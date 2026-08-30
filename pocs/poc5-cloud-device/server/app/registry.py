from __future__ import annotations

import asyncio
import logging
from dataclasses import dataclass
from datetime import datetime, timezone
from uuid import UUID, uuid4

from fastapi import WebSocket, WebSocketDisconnect

LOGGER = logging.getLogger("poc5.registry")


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
    # Desired state requested while offline; pushed automatically on the next
    # connection. None = nothing is queued.
    pending_on: bool | None = None


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

            # The device is now physically in `on`. If that is the state we
            # queued while it was offline, the reconciliation is satisfied and
            # the queue can be cleared so we do not re-push it next reconnect.
            if snapshot.pending_on is not None and snapshot.pending_on == on:
                snapshot.pending_on = None

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
                pending_on=current.pending_on,
            )

    async def queue_state(self, device_id: str, on: bool) -> None:
        """Remember a desired state requested while the device is offline.

        It is pushed automatically by reconcile_pending() the next time the
        device connects. Later calls overwrite it, so the newest request wins.
        """
        await self._set_pending(device_id, on)

    async def _set_pending(self, device_id: str, on: bool | None) -> None:
        async with self._guard:
            snapshot = self._snapshots.setdefault(
                device_id, DeviceSnapshot(device_id=device_id)
            )
            snapshot.pending_on = on

    async def reconcile_pending(self, session: DeviceSession) -> bool:
        """Push a queued desired state to a just-reconnected device.

        Called from the WebSocket read-loop coroutine right after the device
        becomes ready, so this MUST stay fire-and-forget: it sends at most one
        command and returns WITHOUT awaiting the ACK. Awaiting the ACK inline
        would deadlock, because that same coroutine is the one that reads and
        dispatches the ACK. The queue is cleared by record_state_report() when
        the matching state_report arrives; if it never arrives, the next
        reconnect retries it.
        """
        command: dict[str, object] | None = None
        async with self._guard:
            if self._sessions.get(session.device_id) is not session:
                return False
            snapshot = self._snapshots.setdefault(
                session.device_id, DeviceSnapshot(device_id=session.device_id)
            )
            queued = snapshot.pending_on
            if queued is None:
                return False
            reported = snapshot.on
            if reported == queued:
                # Device is already in the desired state; just clear the queue.
                snapshot.pending_on = None
                LOGGER.info(
                    "RECONCILE_NOOP device_id=%s on=%s",
                    session.device_id,
                    queued,
                )
                return False
            command_id = uuid4()
            command = {
                "v": 1,
                "type": "set_state",
                "command_id": str(command_id),
                "device_id": session.device_id,
                "on": queued,
            }
            LOGGER.info(
                "RECONCILE_PUSH device_id=%s command_id=%s on=%s",
                session.device_id,
                command_id,
                queued,
            )
        try:
            await session.websocket.send_json(command)
        except (RuntimeError, OSError, WebSocketDisconnect) as error:
            LOGGER.warning(
                "RECONCILE_SEND_FAILED device_id=%s error=%s",
                session.device_id,
                error,
            )
            return False
        return True

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
                # The socket may already be gone (device just dropped). Keep
                # the desired state queued so reconcile_pending() retries it
                # when the device reconnects.
                await self._set_pending(device_id, on)
                raise DeviceDisconnectedError() from error

            try:
                confirmed_on = await asyncio.wait_for(
                    result, timeout=self._command_timeout_seconds
                )
                # The device confirmed it is physically in the requested
                # state, which is now the newest desired state, so nothing
                # remains pending.
                await self._set_pending(device_id, None)
                return command_id, confirmed_on
            except TimeoutError as error:
                # The command was sent but its confirmation was lost. Keep it
                # queued so the next reconnect can verify/apply it.
                await self._set_pending(device_id, on)
                raise DeviceAckTimeoutError() from error
            except DeviceDisconnectedError as error:
                # The socket dropped while waiting for the ACK. Keep the
                # desired state queued so the next reconnect retries it.
                await self._set_pending(device_id, on)
                raise
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
