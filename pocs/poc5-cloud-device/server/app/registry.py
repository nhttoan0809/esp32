from __future__ import annotations

import asyncio
from dataclasses import dataclass
from datetime import datetime, timezone
from uuid import UUID, uuid4

from fastapi import WebSocket, WebSocketDisconnect


class DeviceOfflineError(RuntimeError):
    pass


class DeviceDisconnectedError(RuntimeError):
    pass


class DeviceAckTimeoutError(RuntimeError):
    pass


class DeviceDidNotApplyError(RuntimeError):
    pass


@dataclass(slots=True)
class DeviceSnapshot:
    device_id: str
    online: bool = False
    on: bool | None = None
    last_seen: datetime | None = None


@dataclass(slots=True)
class PendingCommand:
    command_id: UUID
    requested_on: bool
    result: asyncio.Future[bool]


@dataclass(slots=True)
class DeviceSession:
    device_id: str
    websocket: WebSocket
    generation: int
    online: bool = False
    pending: PendingCommand | None = None
    command_lock: asyncio.Lock | None = None

    def __post_init__(self) -> None:
        self.command_lock = asyncio.Lock()


class DeviceRegistry:
    def __init__(self, command_timeout_seconds: float) -> None:
        self._command_timeout_seconds = command_timeout_seconds
        self._sessions: dict[str, DeviceSession] = {}
        self._snapshots: dict[str, DeviceSnapshot] = {}
        self._generation = 0
        self._guard = asyncio.Lock()

    async def attach(self, device_id: str, websocket: WebSocket) -> DeviceSession:
        old_session: DeviceSession | None
        async with self._guard:
            old_session = self._sessions.get(device_id)
            self._generation += 1
            session = DeviceSession(device_id, websocket, self._generation)
            self._sessions[device_id] = session
            snapshot = self._snapshots.setdefault(
                device_id, DeviceSnapshot(device_id=device_id)
            )
            snapshot.online = False

        if old_session is not None:
            self._fail_pending(old_session, DeviceDisconnectedError())
            try:
                await old_session.websocket.close(
                    code=1012, reason="replaced_by_new_connection"
                )
            except RuntimeError:
                pass
        return session

    async def mark_ready(self, session: DeviceSession, on: bool) -> bool:
        async with self._guard:
            if self._sessions.get(session.device_id) is not session:
                return False
            session.online = True
            snapshot = self._snapshots.setdefault(
                session.device_id, DeviceSnapshot(device_id=session.device_id)
            )
            snapshot.online = True
            snapshot.on = on
            snapshot.last_seen = self._now()
            return True

    async def detach(self, session: DeviceSession) -> None:
        async with self._guard:
            if self._sessions.get(session.device_id) is not session:
                return
            self._sessions.pop(session.device_id, None)
            snapshot = self._snapshots.setdefault(
                session.device_id, DeviceSnapshot(device_id=session.device_id)
            )
            snapshot.online = False
            snapshot.last_seen = self._now()
        self._fail_pending(session, DeviceDisconnectedError())

    async def record_state_report(
        self, session: DeviceSession, command_id: UUID, on: bool
    ) -> bool:
        async with self._guard:
            if self._sessions.get(session.device_id) is not session:
                return False
            snapshot = self._snapshots.setdefault(
                session.device_id, DeviceSnapshot(device_id=session.device_id)
            )
            snapshot.online = session.online
            snapshot.on = on
            snapshot.last_seen = self._now()

            pending = session.pending
            if pending is None or pending.command_id != command_id:
                return False
            if pending.result.done():
                return False
            if pending.requested_on != on:
                pending.result.set_exception(DeviceDidNotApplyError())
            else:
                pending.result.set_result(on)
            return True

    async def snapshot(self, device_id: str) -> DeviceSnapshot:
        async with self._guard:
            current = self._snapshots.get(device_id)
            if current is None:
                return DeviceSnapshot(device_id=device_id)
            return DeviceSnapshot(
                device_id=current.device_id,
                online=current.online,
                on=current.on,
                last_seen=current.last_seen,
            )

    async def send_state_command(self, device_id: str, on: bool) -> tuple[UUID, bool]:
        session = await self._online_session(device_id)
        assert session.command_lock is not None

        async with session.command_lock:
            session = await self._online_session(device_id, expected=session)
            command_id = uuid4()
            loop = asyncio.get_running_loop()
            result: asyncio.Future[bool] = loop.create_future()
            pending = PendingCommand(command_id, on, result)
            session.pending = pending

            try:
                await session.websocket.send_json(
                    {
                        "v": 1,
                        "type": "set_state",
                        "command_id": str(command_id),
                        "device_id": device_id,
                        "on": on,
                    }
                )
            except (RuntimeError, OSError, WebSocketDisconnect) as error:
                raise DeviceDisconnectedError() from error

            try:
                confirmed_on = await asyncio.wait_for(
                    result, timeout=self._command_timeout_seconds
                )
                return command_id, confirmed_on
            except TimeoutError as error:
                raise DeviceAckTimeoutError() from error
            finally:
                if session.pending is pending:
                    session.pending = None

    async def _online_session(
        self, device_id: str, expected: DeviceSession | None = None
    ) -> DeviceSession:
        async with self._guard:
            session = self._sessions.get(device_id)
            if session is None or not session.online:
                raise DeviceOfflineError()
            if expected is not None and session is not expected:
                raise DeviceDisconnectedError()
            return session

    @staticmethod
    def _fail_pending(session: DeviceSession, error: Exception) -> None:
        if session.pending is not None and not session.pending.result.done():
            session.pending.result.set_exception(error)

    @staticmethod
    def _now() -> datetime:
        return datetime.now(timezone.utc)
