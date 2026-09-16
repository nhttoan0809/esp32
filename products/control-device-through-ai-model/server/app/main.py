from __future__ import annotations

import asyncio
import hmac
import json
import logging
from pathlib import Path
from typing import Any

from fastapi import Depends, FastAPI, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse, RedirectResponse
from fastapi.security import APIKeyHeader
from fastapi.staticfiles import StaticFiles
from pydantic import ValidationError

from .config import Settings
from .models import (
    DeviceStateResponse,
    HelloMessage,
    SetStateRequest,
    SetStateResponse,
    StateReportMessage,
)
from .registry import (
    DeviceAckTimeoutError,
    DeviceDidNotApplyError,
    DeviceDisconnectedError,
    DeviceOfflineError,
    DeviceRegistry,
)

LOGGER = logging.getLogger("lamp.server")
STATIC_DIR = Path(__file__).resolve().parent / "static"


def create_app(settings: Settings | None = None) -> FastAPI:
    active_settings = settings or Settings.from_environment()
    registry = DeviceRegistry(active_settings.command_timeout_seconds)
    app = FastAPI(
        title="Smart Lamp Voice Controller Server",
        version="1.0.0",
        description=(
            "Cloud WebSocket server with Voice Wake-Up dashboard for ESP32 Smart Lamp."
        ),
    )
    app.state.settings = active_settings
    app.state.registry = registry

    dashboard_key_header = APIKeyHeader(name="X-API-Key", auto_error=False)

    async def require_dashboard_key(
        provided: str | None = Depends(dashboard_key_header),
    ) -> None:
        if provided is None or not hmac.compare_digest(
            provided, active_settings.dashboard_api_key
        ):
            raise HTTPException(status_code=401, detail="invalid_dashboard_api_key")

    @app.get("/health")
    async def health() -> dict[str, str]:
        return {"status": "ok"}

    @app.get("/", include_in_schema=False)
    async def root() -> RedirectResponse:
        return RedirectResponse(url="/dashboard")

    @app.get("/login", include_in_schema=False)
    async def login_page() -> FileResponse:
        return FileResponse(STATIC_DIR / "login.html")

    @app.get("/dashboard", include_in_schema=False)
    async def dashboard() -> FileResponse:
        return FileResponse(STATIC_DIR / "dashboard.html")

    @app.get("/dashboard.css", include_in_schema=False)
    async def dashboard_css() -> FileResponse:
        return FileResponse(STATIC_DIR / "dashboard.css")

    @app.get("/dashboard.js", include_in_schema=False)
    async def dashboard_js() -> FileResponse:
        return FileResponse(STATIC_DIR / "dashboard.js")

    app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")

    @app.post(
        "/api/auth/verify",
        dependencies=[Depends(require_dashboard_key)],
    )
    async def verify_auth() -> dict[str, Any]:
        return {"status": "ok", "authenticated": True}

    @app.get(
        "/api/devices",
        response_model=list[DeviceStateResponse],
        dependencies=[Depends(require_dashboard_key)],
    )
    async def list_devices() -> list[DeviceStateResponse]:
        configured_ids = set(active_settings.device_tokens.keys())
        all_ids = sorted(configured_ids.union(await registry.all_device_ids()))
        results: list[DeviceStateResponse] = []
        for dev_id in all_ids:
            snapshot = await registry.snapshot(dev_id)
            results.append(
                DeviceStateResponse(
                    device_id=snapshot.device_id,
                    online=snapshot.online,
                    on=snapshot.on,
                    last_seen=snapshot.last_seen,
                    pending_on=snapshot.pending_on,
                )
            )
        return results

    @app.get(
        "/api/devices/{device_id}",
        response_model=DeviceStateResponse,
        dependencies=[Depends(require_dashboard_key)],
    )
    async def get_device(device_id: str) -> DeviceStateResponse:
        snapshot = await registry.snapshot(device_id)
        return DeviceStateResponse(
            device_id=snapshot.device_id,
            online=snapshot.online,
            on=snapshot.on,
            last_seen=snapshot.last_seen,
            pending_on=snapshot.pending_on,
        )

    @app.put(
        "/api/devices/{device_id}/state",
        response_model=SetStateResponse,
        dependencies=[Depends(require_dashboard_key)],
    )
    async def set_device_state(
        device_id: str, request: SetStateRequest
    ) -> SetStateResponse:
        try:
            command_id, confirmed_on = await registry.send_state_command(
                device_id, request.on
            )
        except DeviceOfflineError:
            await registry.queue_state(device_id, request.on)
            return SetStateResponse(
                device_id=device_id,
                on=request.on,
                synced=False,
                command_id=None,
                warning="device_offline_queued",
            )
        except DeviceDisconnectedError as error:
            raise HTTPException(
                status_code=503, detail="device_disconnected"
            ) from error
        except DeviceAckTimeoutError as error:
            raise HTTPException(
                status_code=504, detail="device_ack_timeout"
            ) from error
        except DeviceDidNotApplyError as error:
            raise HTTPException(
                status_code=502, detail="device_did_not_apply_state"
            ) from error

        return SetStateResponse(
            device_id=device_id,
            on=confirmed_on,
            synced=True,
            command_id=command_id,
        )

    @app.websocket("/ws/devices/{device_id}")
    async def device_websocket(websocket: WebSocket, device_id: str) -> None:
        if not _authorized_device(websocket, device_id, active_settings):
            await websocket.close(code=1008, reason="unauthorized")
            return

        await websocket.accept()
        session = await registry.attach(device_id, websocket)
        try:
            raw_hello = await asyncio.wait_for(
                websocket.receive_text(),
                timeout=active_settings.hello_timeout_seconds,
            )
            hello_payload = _parse_payload(
                raw_hello, active_settings.max_websocket_message_bytes
            )
            hello = HelloMessage.model_validate(hello_payload)
            if hello.device_id != device_id:
                raise ProtocolViolation("hello_device_id_mismatch")
            if not await registry.mark_ready(session, hello.reported.on):
                raise ProtocolViolation("session_replaced")

            await websocket.send_json(
                {"v": 1, "type": "ready", "device_id": device_id}
            )
            LOGGER.info("DEVICE_ONLINE device_id=%s", device_id)

            await registry.reconcile_pending(session)

            while True:
                raw_message = await websocket.receive_text()
                payload = _parse_payload(
                    raw_message, active_settings.max_websocket_message_bytes
                )
                report = StateReportMessage.model_validate(payload)
                if report.device_id != device_id:
                    raise ProtocolViolation("report_device_id_mismatch")
                matched = await registry.record_state_report(
                    session, report.command_id, report.on
                )
                LOGGER.info(
                    "STATE_REPORT device_id=%s command_id=%s on=%s matched=%s",
                    device_id,
                    report.command_id,
                    report.on,
                    matched,
                )
        except TimeoutError:
            await _close_quietly(websocket, 1008, "hello_timeout")
        except (json.JSONDecodeError, ValidationError, ProtocolViolation) as error:
            LOGGER.warning("PROTOCOL_ERROR device_id=%s error=%s", device_id, error)
            await _close_quietly(websocket, 1003, "invalid_message")
        except WebSocketDisconnect:
            pass
        finally:
            await registry.detach(session)
            LOGGER.info("DEVICE_OFFLINE device_id=%s", device_id)

    return app


class ProtocolViolation(ValueError):
    pass


def _authorized_device(
    websocket: WebSocket, device_id: str, settings: Settings
) -> bool:
    expected = settings.device_tokens.get(device_id)
    authorization = websocket.headers.get("authorization")
    if expected is None or authorization is None:
        return False
    prefix = "Bearer "
    if not authorization.startswith(prefix):
        return False
    return hmac.compare_digest(authorization[len(prefix) :], expected)


def _parse_payload(raw: str, max_bytes: int) -> Any:
    if len(raw.encode("utf-8")) > max_bytes:
        raise ProtocolViolation("message_too_large")
    return json.loads(raw)


async def _close_quietly(
    websocket: WebSocket, code: int, reason: str
) -> None:
    try:
        await websocket.close(code=code, reason=reason)
    except RuntimeError:
        pass


app = create_app()
