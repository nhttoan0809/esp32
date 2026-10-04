"""
Unit tests for SmartIoTController logic without requiring network sockets.
Can run in any isolated environment (including sandbox).
"""

import json
import unittest
from server import SmartIoTController


class MockBroker:
    def __init__(self):
        self.published = []
        self.callbacks = []

    def register_callback(self, cb):
        self.callbacks.append(cb)

    async def publish(self, topic: str, payload: str | bytes, retain: bool = False):
        self.published.append((topic, payload))


class TestSmartIoTController(unittest.IsolatedAsyncioTestCase):
    def setUp(self):
        self.broker = MockBroker()
        self.controller = SmartIoTController(self.broker)

    def test_esphome_telemetry_ingest(self):
        # 1. ESPHome online status
        self.controller.on_mqtt_message("test", "edge/esphome/status", b"online")
        self.assertTrue(self.controller.state["esphome"]["online"])

        # 2. ESPHome temperature & humidity
        self.controller.on_mqtt_message("test", "edge/esphome/sensor/ambient_temperature/state", b"26.4")
        self.controller.on_mqtt_message("test", "edge/esphome/sensor/ambient_humidity/state", b"62.0")
        self.assertEqual(self.controller.state["esphome"]["temperature"], 26.4)
        self.assertEqual(self.controller.state["esphome"]["humidity"], 62.0)

        # 3. ESPHome physical button state
        self.controller.on_mqtt_message("test", "edge/esphome/binary_sensor/physical_button/state", b"ON")
        self.assertEqual(self.controller.state["esphome"]["button"], "ON")

    def test_tasmota_telemetry_ingest(self):
        # 1. Tasmota LWT
        self.controller.on_mqtt_message("test", "tele/edge_tasmota/LWT", b"Online")
        self.assertTrue(self.controller.state["tasmota"]["online"])

        # 2. Tasmota SENSOR JSON payload
        tasmota_json = json.dumps({
            "Time": "2026-10-04T13:45:00",
            "DHT11": {"Temperature": 27.1, "Humidity": 68.3},
            "TempUnit": "C"
        }).encode("utf-8")
        self.controller.on_mqtt_message("test", "tele/edge_tasmota/SENSOR", tasmota_json)
        self.assertEqual(self.controller.state["tasmota"]["temperature"], 27.1)
        self.assertEqual(self.controller.state["tasmota"]["humidity"], 68.3)

    def test_ai_rule_cooling_trigger(self):
        # When temperature exceeds threshold (28.5 C), AI controller should trigger cooling
        self.controller.state["esphome"]["relay"] = "OFF"
        self.controller.on_mqtt_message("test", "edge/esphome/sensor/ambient_temperature/state", b"30.2")

        # Verify decision was logged
        last_decision = self.controller.state["ai_agent"]["last_decision"]
        self.assertIn("High temperature alert on ESPHOME", last_decision)
        self.assertIn("Triggering COOLING FAN -> ON", last_decision)


if __name__ == "__main__":
    unittest.main()
