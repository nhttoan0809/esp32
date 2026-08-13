#!/usr/bin/env python3
import argparse

import serial


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Read ESP32 UART output from a Wokwi RFC2217 server."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=5.0)
    parser.add_argument("--lines", type=int, default=1)
    args = parser.parse_args()

    url = f"rfc2217://{args.host}:{args.port}"
    with serial.serial_for_url(
        url, baudrate=args.baud, timeout=args.timeout
    ) as connection:
        for _ in range(args.lines):
            data = connection.readline()
            if not data:
                raise SystemExit("Timed out waiting for Serial output")
            print(data.decode(errors="replace"), end="")


if __name__ == "__main__":
    main()
