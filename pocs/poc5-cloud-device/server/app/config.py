from __future__ import annotations

import json
import os
from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class Settings:
    dashboard_api_key: str
    device_tokens: dict[str, str]
    command_timeout_seconds: float = 3.0
    hello_timeout_seconds: float = 5.0
    max_websocket_message_bytes: int = 2048

    @classmethod
    def from_environment(cls) -> "Settings":
        raw_tokens = os.getenv(
            "POC5_DEVICE_TOKENS_JSON",
            '{"esp32-poc5":"poc-device-token-change-me"}',
        )
        try:
            parsed_tokens = json.loads(raw_tokens)
        except json.JSONDecodeError as error:
            raise RuntimeError("POC5_DEVICE_TOKENS_JSON is not valid JSON") from error

        if not isinstance(parsed_tokens, dict) or not parsed_tokens:
            raise RuntimeError("POC5_DEVICE_TOKENS_JSON must be a non-empty object")

        device_tokens: dict[str, str] = {}
        for device_id, token in parsed_tokens.items():
            if not isinstance(device_id, str) or not device_id:
                raise RuntimeError("Every device ID must be a non-empty string")
            if not isinstance(token, str) or not token:
                raise RuntimeError("Every device token must be a non-empty string")
            device_tokens[device_id] = token

        dashboard_api_key = os.getenv(
            "POC5_DASHBOARD_API_KEY", "poc-dashboard-key-change-me"
        )
        if not dashboard_api_key:
            raise RuntimeError("POC5_DASHBOARD_API_KEY must not be empty")

        return cls(
            dashboard_api_key=dashboard_api_key,
            device_tokens=device_tokens,
        )
