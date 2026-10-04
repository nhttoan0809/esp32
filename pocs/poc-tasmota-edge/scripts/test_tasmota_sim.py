"""
Unit test for Tasmota Simulator packets and payload formatting.
Runs without requiring network sockets.
"""

import json
import unittest
from simulate_tasmota_node import (
    create_mqtt_connect_packet,
    create_mqtt_publish_packet,
    create_mqtt_subscribe_packet,
)


class TestTasmotaSim(unittest.TestCase):
    def test_mqtt_connect_packet(self):
        pkt = create_mqtt_connect_packet("test_client", "tele/edge_tasmota/LWT", "Offline")
        self.assertEqual(pkt[0], 0x10)  # CONNECT packet type
        self.assertIn(b"test_client", pkt)
        self.assertIn(b"tele/edge_tasmota/LWT", pkt)
        self.assertIn(b"Offline", pkt)

    def test_mqtt_publish_packet(self):
        topic = "tele/edge_tasmota/SENSOR"
        payload = json.dumps({"DHT11": {"Temperature": 28.0, "Humidity": 65.0}})
        pkt = create_mqtt_publish_packet(topic, payload)
        self.assertEqual(pkt[0], 0x30)  # PUBLISH packet type
        self.assertIn(topic.encode("utf-8"), pkt)
        self.assertIn(b"28.0", pkt)

    def test_mqtt_subscribe_packet(self):
        topic = "cmnd/edge_tasmota/#"
        pkt = create_mqtt_subscribe_packet(topic)
        self.assertEqual(pkt[0], 0x82)  # SUBSCRIBE packet type
        self.assertIn(topic.encode("utf-8"), pkt)


if __name__ == "__main__":
    unittest.main()
