"""
High-Fidelity Tasmota Node Simulator for ESP32.
Implements the exact MQTT protocol, JSON payload schema, LWT, and Rules behavior
of a real Tasmota v14/v15 device without requiring physical hardware.
Zero external dependencies (uses standard python3 socket & json).
"""

import argparse
import json
import logging
import random
import socket
import struct
import sys
import time

logging.basicConfig(level=logging.INFO, format="%(asctime)s [TASMOTA-SIM] %(message)s")
LOGGER = logging.getLogger("tasmota_sim")


def create_mqtt_connect_packet(client_id: str, will_topic: str, will_msg: str) -> bytes:
    cid_bytes = client_id.encode("utf-8")
    wtopic_bytes = will_topic.encode("utf-8")
    wmsg_bytes = will_msg.encode("utf-8")

    # Var header: Protocol Name (MQTT), Level (4), Connect Flags, KeepAlive (60s)
    # Connect Flags: Clean Session (0x02) | Will Flag (0x04) | Will Retain (0x20) = 0x26
    var_header = b"\x00\x04MQTT\x04\x26\x00\x3c"
    payload = (
        struct.pack("!H", len(cid_bytes)) + cid_bytes +
        struct.pack("!H", len(wtopic_bytes)) + wtopic_bytes +
        struct.pack("!H", len(wmsg_bytes)) + wmsg_bytes
    )
    rem_len = len(var_header) + len(payload)
    return b"\x10" + bytes([rem_len]) + var_header + payload


def create_mqtt_publish_packet(topic: str, payload: str, retain: bool = False) -> bytes:
    topic_bytes = topic.encode("utf-8")
    msg_bytes = payload.encode("utf-8")
    var_header = struct.pack("!H", len(topic_bytes)) + topic_bytes
    rem_len = len(var_header) + len(msg_bytes)
    first_byte = 0x31 if retain else 0x30
    return bytes([first_byte, rem_len]) + var_header + msg_bytes


def create_mqtt_subscribe_packet(topic: str, packet_id: int = 1) -> bytes:
    topic_bytes = topic.encode("utf-8")
    var_header = struct.pack("!H", packet_id)
    payload = struct.pack("!H", len(topic_bytes)) + topic_bytes + b"\x00"  # QoS 0
    rem_len = len(var_header) + len(payload)
    return b"\x82" + bytes([rem_len]) + var_header + payload


class TasmotaSimulator:
    def __init__(self, host: str, port: int, device_topic: str = "edge_tasmota"):
        self.host = host
        self.port = port
        self.topic = device_topic
        self.power = "OFF"
        self.temp = 27.5
        self.hum = 65.0
        self.vcc = 3.32
        self.rssi = 88
        self.uptime_sec = 0
        self.sock: socket.socket | None = None

    def connect(self):
        LOGGER.info(f"Connecting to MQTT Broker at {self.host}:{self.port}...")
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))

        client_id = f"tasmota_{random.randint(100000, 999999)}"
        lwt_topic = f"tele/{self.topic}/LWT"
        connect_pkt = create_mqtt_connect_packet(client_id, lwt_topic, "Offline")
        self.sock.sendall(connect_pkt)

        # Wait for CONNACK
        connack = self.sock.recv(4)
        if connack != b"\x20\x02\x00\x00":
            raise RuntimeError(f"MQTT Connection rejected: {connack.hex()}")

        LOGGER.info(f"Connected as '{client_id}'. Publishing LWT Online...")
        # Send Online LWT
        self.sock.sendall(create_mqtt_publish_packet(lwt_topic, "Online", retain=True))

        # Subscribe to inbound commands: cmnd/edge_tasmota/#
        cmnd_topic = f"cmnd/{self.topic}/#"
        self.sock.sendall(create_mqtt_subscribe_packet(cmnd_topic))
        LOGGER.info(f"Subscribed to command channel: '{cmnd_topic}'")

    def run_step(self):
        # 1. Non-blocking check for inbound MQTT packets
        self.sock.setblocking(False)
        try:
            data = self.sock.recv(1024)
            if data and data[0] >> 4 == 3:  # PUBLISH packet
                self._handle_inbound_publish(data)
        except BlockingIOError:
            pass
        except ConnectionResetError:
            LOGGER.error("Connection reset by broker.")
            sys.exit(1)

        self.uptime_sec += 1

    def _handle_inbound_publish(self, data: bytes):
        try:
            topic_len = struct.unpack("!H", data[2:4])[0]
            topic = data[4:4 + topic_len].decode("utf-8")
            payload = data[4 + topic_len:].decode("utf-8", errors="ignore").strip()
            LOGGER.info(f"[COMMAND RECV] {topic} -> {payload}")

            if topic.endswith("/POWER"):
                if payload.upper() == "ON":
                    self.set_power("ON")
                elif payload.upper() == "OFF":
                    self.set_power("OFF")
                elif payload.upper() in ("TOGGLE", "2"):
                    new_state = "OFF" if self.power == "ON" else "ON"
                    self.set_power(new_state)
        except Exception as e:
            LOGGER.error(f"Error parsing publish: {e}")

    def set_power(self, state: str):
        self.power = state
        LOGGER.info(f"⚡ [EDGE RELAY] Power state changed -> {self.power}")
        # Tasmota publishes both stat/topic/POWER and stat/topic/RESULT
        stat_topic = f"stat/{self.topic}/POWER"
        res_topic = f"stat/{self.topic}/RESULT"
        res_payload = json.dumps({"POWER": self.power})

        self.sock.sendall(create_mqtt_publish_packet(stat_topic, self.power))
        self.sock.sendall(create_mqtt_publish_packet(res_topic, res_payload))

    def publish_telemetry(self):
        # Fluctuate temperature slightly
        self.temp += round(random.uniform(-0.3, 0.4), 1)
        self.temp = max(24.0, min(36.0, self.temp))
        self.hum += round(random.uniform(-0.5, 0.5), 1)

        # 1. tele/topic/STATE
        state_payload = json.dumps({
            "Time": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "Uptime": f"{self.uptime_sec // 3600}T{(self.uptime_sec % 3600) // 60:02d}:{self.uptime_sec % 60:02d}",
            "Vcc": self.vcc,
            "POWER": self.power,
            "Wifi": {"RSSI": self.rssi, "SSID": "HomeNet", "BSSId": "AA:BB:CC:DD:EE:FF"}
        })
        self.sock.sendall(create_mqtt_publish_packet(f"tele/{self.topic}/STATE", state_payload))

        # 2. tele/topic/SENSOR (DHT11 reading)
        sensor_payload = json.dumps({
            "Time": time.strftime("%Y-%m-%dT%H:%M:%S"),
            "DHT11": {
                "Temperature": round(self.temp, 1),
                "Humidity": round(self.hum, 1),
                "DewPoint": round(self.temp - ((100 - self.hum) / 5), 1)
            },
            "TempUnit": "C"
        })
        self.sock.sendall(create_mqtt_publish_packet(f"tele/{self.topic}/SENSOR", sensor_payload))
        LOGGER.info(f"[TELEMETRY] Sent State & Sensor data (Temp={self.temp:.1f}°C, Relay={self.power})")

        # 3. Simulate Tasmota Rule 2: Overheat Protection at edge
        if self.temp > 32.0 and self.power != "ON":
            LOGGER.warning(f"🚨 [RULE2 FIRED] Temperature {self.temp:.1f}°C > 32°C! Auto-activating POWER ON at edge!")
            self.set_power("ON")

    def simulate_button_press(self):
        """Simulates Rule 1: ON Button1#State DO Power1 2 (Toggle) ENDON"""
        LOGGER.info("🔵 [RULE1 TRIGGER] Physical Button Pressed -> Local Toggle")
        new_state = "OFF" if self.power == "ON" else "ON"
        self.set_power(new_state)


def main():
    parser = argparse.ArgumentParser(description="Tasmota Node Simulator for Self-Hosted Server")
    parser.add_argument("--host", default="127.0.0.1", help="MQTT Broker host")
    parser.add_argument("--port", type=int, default=1883, help="MQTT Broker port")
    parser.add_argument("--topic", default="edge_tasmota", help="Device topic prefix")
    parser.add_argument("--interval", type=int, default=5, help="Telemetry interval in seconds")
    args = parser.parse_args()

    sim = TasmotaSimulator(args.host, args.port, args.topic)
    sim.connect()

    last_telemetry = time.time()
    LOGGER.info("Tasmota simulation running. Press Ctrl+C to terminate.")

    try:
        while True:
            sim.run_step()
            if time.time() - last_telemetry >= args.interval:
                sim.publish_telemetry()
                last_telemetry = time.time()
            time.sleep(0.1)
    except KeyboardInterrupt:
        LOGGER.info("Simulator terminated by user.")


if __name__ == "__main__":
    main()
