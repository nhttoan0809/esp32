"""Launch the POC5 dashboard server.

Reads POC5_* variables from ./.env WITHOUT going through a shell. This matters
because POC5_DEVICE_TOKENS_JSON contains double quotes, and `source .env` /
`. ./.env` in bash makes the shell consume those quotes, turning the value into
invalid JSON ("POC5_DEVICE_TOKENS_JSON is not valid JSON"). Parsing here keeps
the values byte-for-byte.

Usage:
    python run_uvicorn.py            # foreground
    nohup python run_uvicorn.py >uvicorn.log 2>&1 &   # detached (survives the shell)
"""
from __future__ import annotations

import json
import os
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
HOST = "127.0.0.1"
PORT = 8000


def load_poc5_env() -> dict[str, str]:
    env_path = HERE / ".env"
    if not env_path.exists():
        return {}
    values: dict[str, str] = {}
    for match in re.finditer(
        r"^(POC5_[A-Z_]+)=(.*)$", env_path.read_text(), re.MULTILINE
    ):
        values[match.group(1)] = match.group(2).strip()
    for key, value in values.items():
        if key.endswith("_JSON"):
            json.loads(value)  # fail fast on a mangled JSON value
    return values


def main() -> None:
    for key, value in load_poc5_env().items():
        os.environ[key] = value

    import uvicorn

    uvicorn.run("app.main:app", host=HOST, port=PORT, log_level="info")


if __name__ == "__main__":
    main()
