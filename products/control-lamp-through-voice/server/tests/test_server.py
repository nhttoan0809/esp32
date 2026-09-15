from __future__ import annotations

import time
from concurrent.futures import ThreadPoolExecutor

import pytest
from fastapi.testclient import TestClient

from app.config import Settings
from app.main import create_app

DEVICE_ID = "esp32-smart-lamp"
DEVICE_TOKEN = "lamp-secret-token"
DASHBOARD_KEY = "lamp-dashboard-secret"


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
        "firmware": "smart-lamp-voice-1.0.0",
        "reported": {"on": on},
    }


def test_health_and_dashboard_and_login_are_served() -> None:
    with make_client() as client:
        assert client.get("/health").json() == {"status": "ok"}
        login = client.get("/login")
        assert login.status_code == 200
        assert "Smart Lamp" in login.text
        dashboard = client.get("/dashboard")
        assert dashboard.status_code == 200
        assert "Smart Lamp Controller" in dashboard.text
        root = client.get("/", follow_redirects=False)
        assert root.status_code == 307
        assert root.headers["location"] == "/dashboard"


def test_auth_verify_and_device_list_endpoints() -> None:
    with make_client() as client:
        # Auth verify with invalid key
        bad_auth = client.post("/api/auth/verify", headers={"X-API-Key": "wrong"})
        assert bad_auth.status_code == 401

        # Auth verify with valid key
        good_auth = client.post("/api/auth/verify", headers=dashboard_headers())
        assert good_auth.status_code == 200
        assert good_auth.json()["authenticated"] is True

        # List devices with invalid key
        bad_list = client.get("/api/devices")
        assert bad_list.status_code == 401

        # List devices with valid key
        good_list = client.get("/api/devices", headers=dashboard_headers())
        assert good_list.status_code == 200
        devices = good_list.json()
        assert isinstance(devices, list)
        assert len(devices) >= 1
        assert devices[0]["device_id"] == DEVICE_ID


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
        queued = client.put(
            f"/api/devices/{DEVICE_ID}/state",
            headers=dashboard_headers(),
            json={"on": True},
        )
        assert queued.status_code == 200
        assert queued.json()["synced"] is False

        with client.websocket_connect(
            f"/ws/devices/{DEVICE_ID}", headers=websocket_headers()
        ) as websocket:
            websocket.send_json(hello(on=False))
            assert websocket.receive_json()["type"] == "ready"

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

        with client.websocket_connect(
            f"/ws/devices/{DEVICE_ID}", headers=websocket_headers()
        ) as websocket:
            websocket.send_json(hello(on=True))
            assert websocket.receive_json()["type"] == "ready"

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


def test_unsolicited_button_state_report_updates_snapshot() -> None:
    with make_client() as client:
        with client.websocket_connect(
            f"/ws/devices/{DEVICE_ID}", headers=websocket_headers()
        ) as websocket:
            websocket.send_json(hello(on=False))
            assert websocket.receive_json()["type"] == "ready"

            # Physical button pressed on ESP32 -> sends state_report with null UUID
            websocket.send_json(
                {
                    "v": 1,
                    "type": "state_report",
                    "command_id": "00000000-0000-0000-0000-000000000000",
                    "device_id": DEVICE_ID,
                    "on": True,
                }
            )
            time.sleep(0.05)

            state = client.get(
                f"/api/devices/{DEVICE_ID}", headers=dashboard_headers()
            )
            assert state.json()["on"] is True
