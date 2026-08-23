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


def test_offline_command_is_rejected_and_not_queued() -> None:
    with make_client() as client:
        response = client.put(
            f"/api/devices/{DEVICE_ID}/state",
            headers=dashboard_headers(),
            json={"on": True},
        )
        assert response.status_code == 503
        assert response.json()["detail"] == "device_offline"


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
            assert body["confirmed"] is True


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
