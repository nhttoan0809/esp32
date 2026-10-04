"""
Unit & Integration Test for the Self-Hosted IoT Server.
Validates HTTP endpoints, MQTT socket handshake, and AI Controller rule execution.
"""

import asyncio
import json
import socket
import struct
import time
import urllib.request

MQTT_HOST = "127.0.0.1"
MQTT_PORT = 11883  # Test port
HTTP_PORT = 18080  # Test port


def create_mqtt_connect_packet(client_id: str) -> bytes:
    cid_bytes = client_id.encode("utf-8")
    payload = struct.pack("!H", len(cid_bytes)) + cid_bytes
    var_header = b"\x00\x04MQTT\x04\x02\x00\x3c"  # Clean session, keepalive 60s
    rem_len = len(var_header) + len(payload)
    return b"\x10" + bytes([rem_len]) + var_header + payload


def create_mqtt_publish_packet(topic: str, payload: str) -> bytes:
    topic_bytes = topic.encode("utf-8")
    msg_bytes = payload.encode("utf-8")
    var_header = struct.pack("!H", len(topic_bytes)) + topic_bytes
    rem_len = len(var_header) + len(msg_bytes)
    return b"\x30" + bytes([rem_len]) + var_header + msg_bytes


def run_tests():
    print(">>> 1. Testing HTTP GET /api/state...")
    url = f"http://127.0.0.1:{HTTP_PORT}/api/state"
    req = urllib.request.Request(url)
    with urllib.request.urlopen(req, timeout=5) as resp:
        assert resp.status == 200
        data = json.loads(resp.read().decode("utf-8"))
        assert "esphome" in data
        assert "tasmota" in data
        assert "ai_agent" in data
        print("    [PASS] HTTP State API returned valid device registries.")

    print(">>> 2. Testing MQTT Connection & ESPHome Telemetry...")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((MQTT_HOST, MQTT_PORT))
    s.sendall(create_mqtt_connect_packet("test-esphome-device"))
    connack = s.recv(4)
    assert connack == b"\x20\x02\x00\x00", f"Unexpected connack: {connack}"
    print("    [PASS] MQTT Broker accepted ESPHome client connection.")

    # Send high temperature to trigger AI
    s.sendall(create_mqtt_publish_packet("edge/esphome/sensor/ambient_temperature/state", "31.5"))
    s.sendall(create_mqtt_publish_packet("edge/esphome/sensor/ambient_humidity/state", "75.0"))
    s.sendall(create_mqtt_publish_packet("edge/esphome/switch/living_fan_relay/state", "OFF"))
    time.sleep(0.5)

    # Check that state updated and AI rule fired
    with urllib.request.urlopen(url, timeout=5) as resp:
        data = json.loads(resp.read().decode("utf-8"))
        assert data["esphome"]["temperature"] == 31.5
        assert data["esphome"]["humidity"] == 75.0
        assert "COOLING FAN -> ON" in data["ai_agent"]["last_decision"]
        print("    [PASS] ESPHome telemetry ingested & AI Cooling Decision triggered!")

    print(">>> 3. Testing Tasmota JSON Telemetry Ingest...")
    tasmota_payload = json.dumps({
        "Time": "2026-10-04T13:45:00",
        "DHT11": {"Temperature": 25.2, "Humidity": 60.5},
        "TempUnit": "C"
    })
    s.sendall(create_mqtt_publish_packet("tele/edge_tasmota/SENSOR", tasmota_payload))
    time.sleep(0.5)

    with urllib.request.urlopen(url, timeout=5) as resp:
        data = json.loads(resp.read().decode("utf-8"))
        assert data["tasmota"]["temperature"] == 25.2
        assert data["tasmota"]["humidity"] == 60.5
        print("    [PASS] Tasmota JSON telemetry successfully parsed.")

    s.close()
    print(">>> ALL TESTS PASSED SUCCESSFULLY! ✅")


if __name__ == "__main__":
    run_tests()
