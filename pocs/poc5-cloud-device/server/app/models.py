from __future__ import annotations

from datetime import datetime
from typing import Literal
from uuid import UUID

from pydantic import BaseModel, ConfigDict, Field, StrictBool


class StrictModel(BaseModel):
    model_config = ConfigDict(extra="forbid")


class DeviceOutput(StrictModel):
    on: StrictBool


class HelloMessage(StrictModel):
    v: Literal[1]
    type: Literal["hello"]
    device_id: str = Field(min_length=1, max_length=64)
    firmware: str = Field(min_length=1, max_length=64)
    reported: DeviceOutput


class StateReportMessage(StrictModel):
    v: Literal[1]
    type: Literal["state_report"]
    command_id: UUID
    device_id: str = Field(min_length=1, max_length=64)
    on: StrictBool


class SetStateRequest(StrictModel):
    on: StrictBool


class DeviceStateResponse(StrictModel):
    device_id: str
    online: bool
    on: bool | None
    last_seen: datetime | None


class SetStateResponse(StrictModel):
    device_id: str
    command_id: UUID
    on: bool
    confirmed: Literal[True] = True
