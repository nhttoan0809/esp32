from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor

import pytest
from fastapi.testclient import TestClient

from app.config import Settings
from app.main import create_app

DEVICE_ID = "esp32-poc5"
DEVICE_TOKEN = "test-device-token"
DASHBOARD_KEY = "test-dashboard-key"


def make_client(command_timeout: float = 0.5) -> TestClient:
    settings = Settings(
        dashboard_api_key=DASHBOARD_KEY,
        device_tokens={DEVICE_ID: DEVICE_TOKEN},
        command_timeout_seconds=command_timeout,
        hello_timeout_seconds=0.5,
    )
    return TestClient(create_app(settings))


def dashboard_headers() -> dict[str, str]:
    return {"X-API-Key": DASHBOARD_KEY}


def websocket_headers() -> dict[str, str]:
    return {"Authorization": f"Bearer {DEVICE_TOKEN}"}


def hello(on: bool = False) -> dict[str, object]:
    return {
        "v": 1,
        "type": "hello",
        "device_id": DEVICE_ID,
        "firmware": "test-0.1.0",
        "reported": {"on": on},
    }


def test_health_and_dashboard_are_served() -> None:
    with make_client() as client:
        assert client.get("/health").json() == {"status": "ok"}
        dashboard = client.get("/dashboard")
        assert dashboard.status_code == 200
        assert "ESP32 Real Device" in dashboard.text


def test_dashboard_api_requires_key() -> None:
    with make_client() as client:
        response = client.get(f"/api/devices/{DEVICE_ID}")
        assert response.status_code == 401
        assert response.json()["detail"] == "invalid_dashboard_api_key"


def test_offline_command_is_queued_and_warned() -> None:
    with make_client() as client:
        response = client.put(
            f"/api/devices/{DEVICE_ID}/state",
            headers=dashboard_headers(),
            json={"on": True},
        )
        assert response.status_code == 200
        body = response.json()
        assert body["synced"] is False
        assert body["command_id"] is None
        assert body["warning"] == "device_offline_queued"

        # The desired state is stored and visible while the device is offline.
        state = client.get(
            f"/api/devices/{DEVICE_ID}", headers=dashboard_headers()
        )
        assert state.status_code == 200
        assert state.json()["online"] is False
        assert state.json()["pending_on"] is True


def test_websocket_rejects_invalid_device_token() -> None:
    with make_client() as client:
        with pytest.raises(Exception) as captured:
            with client.websocket_connect(
                f"/ws/devices/{DEVICE_ID}",
                headers={"Authorization": "Bearer wrong"},
            ):
                pass
        assert getattr(captured.value, "status_code", 403) == 403


def test_hello_sets_reported_state_and_online_status() -> None:
    with make_client() as client:
        with client.websocket_connect(
            f"/ws/devices/{DEVICE_ID}", headers=websocket_headers()
        ) as websocket:
            websocket.send_json(hello(on=True))
            assert websocket.receive_json() == {
                "v": 1,
                "type": "ready",
                "device_id": DEVICE_ID,
            }
            response = client.get(
                f"/api/devices/{DEVICE_ID}", headers=dashboard_headers()
            )
            assert response.status_code == 200
            assert response.json()["online"] is True
            assert response.json()["on"] is True


def test_direct_command_returns_only_after_matching_ack() -> None:
    with make_client() as client:
        with client.websocket_connect(
            f"/ws/devices/{DEVICE_ID}", headers=websocket_headers()
        ) as websocket:
            websocket.send_json(hello())
            websocket.receive_json()

            with ThreadPoolExecutor(max_workers=1) as executor:
                pending_response = executor.submit(
                    client.put,
                    f"/api/devices/{DEVICE_ID}/state",
                    headers=dashboard_headers(),
                    json={"on": True},
                )
                command = websocket.receive_json()
                assert command["type"] == "set_state"
                assert command["on"] is True
                websocket.send_json(
                    {
                        "v": 1,
                        "type": "state_report",
                        "command_id": command["command_id"],
                        "device_id": DEVICE_ID,
                        "on": True,
                    }
                )
                response = pending_response.result(timeout=2)

            assert response.status_code == 200
            body = response.json()
            assert body["command_id"] == command["command_id"]
            assert body["on"] is True
            assert body["synced"] is True


def test_offline_desired_state_is_pushed_on_reconnect() -> None:
    with make_client() as client:
        # 1) Device is offline; a PUT is queued and warns.
        queued = client.put(
            f"/api/devices/{DEVICE_ID}/state",
            headers=dashboard_headers(),
            json={"on": True},
        )
        assert queued.status_code == 200
        assert queued.json()["synced"] is False

        # 2) The device (re)connects and reports its *physical* state is off.
        with client.websocket_connect(
            f"/ws/devices/{DEVICE_ID}", headers=websocket_headers()
        ) as websocket:
            websocket.send_json(hello(on=False))
            assert websocket.receive_json()["type"] == "ready"

            # 3) The server automatically pushes the queued desired state so
            #    the physical device catches up on reconnect.
            command = websocket.receive_json()
            assert command["type"] == "set_state"
            assert command["on"] is True

            # 4) The device confirms the applied state.
            websocket.send_json(
                {
                    "v": 1,
                    "type": "state_report",
                    "command_id": command["command_id"],
                    "device_id": DEVICE_ID,
                    "on": True,
                }
            )

            # 5) The queue is now satisfied, so a reconnect is a no-op.
            state = client.get(
                f"/api/devices/{DEVICE_ID}", headers=dashboard_headers()
            )
            assert state.json()["online"] is True
            assert state.json()["on"] is True
            assert state.json()["pending_on"] is None


def test_already_in_desired_state_is_not_repushed_on_reconnect() -> None:
    with make_client() as client:
        client.put(
            f"/api/devices/{DEVICE_ID}/state",
            headers=dashboard_headers(),
            json={"on": True},
        )

        # Device reconnects already in the desired state (physical is ON).
        with client.websocket_connect(
            f"/ws/devices/{DEVICE_ID}", headers=websocket_headers()
        ) as websocket:
            websocket.send_json(hello(on=True))
            assert websocket.receive_json()["type"] == "ready"

            # No set_state should be pushed; the next GET shows it settled.
            state = client.get(
                f"/api/devices/{DEVICE_ID}", headers=dashboard_headers()
            )
            assert state.json()["online"] is True
            assert state.json()["on"] is True
            assert state.json()["pending_on"] is None


def test_command_times_out_without_ack() -> None:
    with make_client(command_timeout=0.05) as client:
        with client.websocket_connect(
            f"/ws/devices/{DEVICE_ID}", headers=websocket_headers()
        ) as websocket:
            websocket.send_json(hello())
            websocket.receive_json()

            with ThreadPoolExecutor(max_workers=1) as executor:
                pending_response = executor.submit(
                    client.put,
                    f"/api/devices/{DEVICE_ID}/state",
                    headers=dashboard_headers(),
                    json={"on": True},
                )
                assert websocket.receive_json()["type"] == "set_state"
                response = pending_response.result(timeout=2)

            assert response.status_code == 504
            assert response.json()["detail"] == "device_ack_timeout"


def test_disconnect_while_waiting_for_ack_returns_503() -> None:
    with make_client() as client:
        websocket = client.websocket_connect(
            f"/ws/devices/{DEVICE_ID}", headers=websocket_headers()
        )
        websocket.__enter__()
        try:
            websocket.send_json(hello())
            websocket.receive_json()

            with ThreadPoolExecutor(max_workers=1) as executor:
                pending_response = executor.submit(
                    client.put,
                    f"/api/devices/{DEVICE_ID}/state",
                    headers=dashboard_headers(),
                    json={"on": True},
                )
                assert websocket.receive_json()["type"] == "set_state"
                websocket.close()
                response = pending_response.result(timeout=2)

            assert response.status_code == 503
            assert response.json()["detail"] == "device_disconnected"
        finally:
            websocket.__exit__(None, None, None)


def test_new_connection_replaces_old_session_and_receives_commands() -> None:
    with make_client() as client:
        with client.websocket_connect(
            f"/ws/devices/{DEVICE_ID}", headers=websocket_headers()
        ) as first_socket:
            first_socket.send_json(hello())
            first_socket.receive_json()

            with client.websocket_connect(
                f"/ws/devices/{DEVICE_ID}", headers=websocket_headers()
            ) as second_socket:
                second_socket.send_json(hello(on=True))
                second_socket.receive_json()

                with ThreadPoolExecutor(max_workers=1) as executor:
                    pending_response = executor.submit(
                        client.put,
                        f"/api/devices/{DEVICE_ID}/state",
                        headers=dashboard_headers(),
                        json={"on": False},
                    )
                    command = second_socket.receive_json()
                    second_socket.send_json(
                        {
                            "v": 1,
                            "type": "state_report",
                            "command_id": command["command_id"],
                            "device_id": DEVICE_ID,
                            "on": False,
                        }
                    )
                    response = pending_response.result(timeout=2)

                assert response.status_code == 200
                assert response.json()["on"] is False


def test_mismatched_applied_state_is_not_reported_as_success() -> None:
    with make_client() as client:
        with client.websocket_connect(
            f"/ws/devices/{DEVICE_ID}", headers=websocket_headers()
        ) as websocket:
            websocket.send_json(hello())
            websocket.receive_json()

            with ThreadPoolExecutor(max_workers=1) as executor:
                pending_response = executor.submit(
                    client.put,
                    f"/api/devices/{DEVICE_ID}/state",
                    headers=dashboard_headers(),
                    json={"on": True},
                )
                command = websocket.receive_json()
                websocket.send_json(
                    {
                        "v": 1,
                        "type": "state_report",
                        "command_id": command["command_id"],
                        "device_id": DEVICE_ID,
                        "on": False,
                    }
                )
                response = pending_response.result(timeout=2)

            assert response.status_code == 502
            assert response.json()["detail"] == "device_did_not_apply_state"
