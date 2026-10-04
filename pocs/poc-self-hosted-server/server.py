"""
Lightweight Self-Hosted IoT Server with Embedded MQTT Broker, REST API, Web Dashboard, and AI Controller.
Zero external dependencies required (uses Python standard library asyncio & http).
Supports both ESPHome and Tasmota MQTT dialects simultaneously.
"""

from __future__ import annotations

import asyncio
import json
import logging
import os
import struct
import sys
import time
from typing import Dict, Set, Tuple, Any

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
LOGGER = logging.getLogger("self_hosted_server")


# ============================================================================
# Minimal Embedded MQTT 3.1.1 Broker (RFC Compliant Subset)
# ============================================================================

class MQTTBroker:
    """A lightweight in-memory MQTT 3.1.1 broker for local edge device communication."""

    def __init__(self, host: str = "0.0.0.0", port: int = 1883):
        self.host = host
        self.port = port
        self.clients: Dict[str, asyncio.StreamWriter] = {}
        # topic -> set of (client_id, writer)
        self.subscriptions: Dict[str, Set[Tuple[str, asyncio.StreamWriter]]] = {}
        self.retained: Dict[str, bytes] = {}
        self.server: asyncio.Server | None = None
        self.message_callbacks = []

    def register_callback(self, callback):
        self.message_callbacks.append(callback)

    async def start(self):
        self.server = await asyncio.start_server(self.handle_client, self.host, self.port)
        LOGGER.info(f"MQTT Broker listening on {self.host}:{self.port}")

    async def handle_client(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        client_id = f"client-{id(writer)}"
        try:
            while True:
                # Read fixed header: Byte 1 = Packet type & flags
                byte1 = await reader.read(1)
                if not byte1:
                    break
                packet_type = byte1[0] >> 4
                flags = byte1[0] & 0x0F

                # Read remaining length (variable length encoding)
                multiplier = 1
                rem_len = 0
                while True:
                    encoded_byte = await reader.read(1)
                    if not encoded_byte:
                        break
                    digit = encoded_byte[0]
                    rem_len += (digit & 127) * multiplier
                    multiplier *= 128
                    if (digit & 128) == 0:
                        break

                payload = await reader.readexactly(rem_len) if rem_len > 0 else b""

                # Dispatch packet type
                if packet_type == 1:  # CONNECT
                    client_id = self._parse_connect(payload, client_id)
                    self.clients[client_id] = writer
                    LOGGER.info(f"MQTT Client connected: {client_id}")
                    # Send CONNACK (0x20, len=2, flags=0, return_code=0)
                    writer.write(b"\x20\x02\x00\x00")
                    await writer.drain()

                elif packet_type == 3:  # PUBLISH
                    topic, message = self._parse_publish(flags, payload)
                    # Notify internal callbacks
                    for cb in self.message_callbacks:
                        try:
                            cb(client_id, topic, message)
                        except Exception as e:
                            LOGGER.error(f"Error in callback: {e}")
                    # Broadcast to matching subscribers
                    await self._route_publish(topic, message, flags & 0x01)

                elif packet_type == 8:  # SUBSCRIBE
                    packet_id, topics = self._parse_subscribe(payload)
                    for t in topics:
                        if t not in self.subscriptions:
                            self.subscriptions[t] = set()
                        self.subscriptions[t].add((client_id, writer))
                        LOGGER.info(f"Client '{client_id}' subscribed to: '{t}'")
                    # Send SUBACK (0x90, len=2+len(topics), packet_id, qos 0 per topic)
                    resp = bytearray([0x90, 2 + len(topics), packet_id >> 8, packet_id & 0xFF])
                    resp.extend([0x00] * len(topics))
                    writer.write(resp)
                    await writer.drain()

                elif packet_type == 12:  # PINGREQ
                    # Send PINGRESP (0xD0, 0x00)
                    writer.write(b"\xD0\x00")
                    await writer.drain()

                elif packet_type == 14:  # DISCONNECT
                    LOGGER.info(f"MQTT Client disconnected: {client_id}")
                    break

        except (asyncio.IncompleteReadError, ConnectionResetError):
            pass
        except Exception as e:
            LOGGER.debug(f"Client exception ({client_id}): {e}")
        finally:
            if client_id in self.clients:
                del self.clients[client_id]
            # Clean up subscriptions
            for t in list(self.subscriptions.keys()):
                self.subscriptions[t] = {
                    (cid, w) for cid, w in self.subscriptions[t] if cid != client_id
                }
            try:
                writer.close()
                await writer.wait_closed()
            except Exception:
                pass

    def _parse_connect(self, payload: bytes, default_id: str) -> str:
        if len(payload) < 10:
            return default_id
        proto_len = struct.unpack("!H", payload[0:2])[0]
        offset = 2 + proto_len + 4  # skip proto name, level, flags, keepalive
        if offset + 2 <= len(payload):
            cid_len = struct.unpack("!H", payload[offset:offset + 2])[0]
            cid = payload[offset + 2: offset + 2 + cid_len].decode("utf-8", errors="ignore")
            if cid:
                return cid
        return default_id

    def _parse_publish(self, flags: int, payload: bytes) -> Tuple[str, bytes]:
        topic_len = struct.unpack("!H", payload[0:2])[0]
        topic = payload[2:2 + topic_len].decode("utf-8", errors="ignore")
        offset = 2 + topic_len
        qos = (flags >> 1) & 0x03
        if qos > 0:
            offset += 2  # Skip packet identifier
        message = payload[offset:]
        return topic, message

    def _parse_subscribe(self, payload: bytes) -> Tuple[int, list[str]]:
        packet_id = struct.unpack("!H", payload[0:2])[0]
        offset = 2
        topics = []
        while offset < len(payload):
            t_len = struct.unpack("!H", payload[offset:offset + 2])[0]
            t = payload[offset + 2: offset + 2 + t_len].decode("utf-8", errors="ignore")
            offset += 2 + t_len + 1  # Skip topic + requested QoS
            topics.append(t)
        return packet_id, topics

    async def _route_publish(self, topic: str, message: bytes, retain: bool):
        if retain:
            self.retained[topic] = message

        # Encode standard QoS 0 PUBLISH packet
        topic_bytes = topic.encode("utf-8")
        packet_len = 2 + len(topic_bytes) + len(message)
        header = bytearray([0x30])
        # encode remaining length
        val = packet_len
        while True:
            encoded_byte = val % 128
            val = val // 128
            if val > 0:
                encoded_byte |= 128
            header.append(encoded_byte)
            if val == 0:
                break
        header.extend(struct.pack("!H", len(topic_bytes)))
        header.extend(topic_bytes)
        header.extend(message)

        # Match wildcard and exact subscriptions
        delivered_writers = set()
        for sub_topic, clients in self.subscriptions.items():
            if self._topic_matches(sub_topic, topic):
                for cid, writer in clients:
                    if writer not in delivered_writers:
                        try:
                            writer.write(header)
                            await writer.drain()
                            delivered_writers.add(writer)
                        except Exception:
                            pass

    def _topic_matches(self, pattern: str, topic: str) -> bool:
        if pattern == "#" or pattern == topic:
            return True
        p_parts = pattern.split("/")
        t_parts = topic.split("/")
        for i, p in enumerate(p_parts):
            if p == "#":
                return True
            if i >= len(t_parts):
                return False
            if p != "+" and p != t_parts[i]:
                return False
        return len(p_parts) == len(t_parts)

    async def publish(self, topic: str, payload: str | bytes, retain: bool = False):
        """Internal helper to publish a message from the server itself."""
        msg_bytes = payload.encode("utf-8") if isinstance(payload, str) else payload
        # Notify local callbacks
        for cb in self.message_callbacks:
            try:
                cb("SERVER", topic, msg_bytes)
            except Exception:
                pass
        await self._route_publish(topic, msg_bytes, retain)


# ============================================================================
# Device Registry, State Tracker & AI Decision Controller
# ============================================================================

class SmartIoTController:
    """
    Maintains real-time state for both ESPHome and Tasmota devices.
    Executes AI / Heuristic rules to automate edge actuators.
    """

    def __init__(self, broker: MQTTBroker):
        self.broker = broker
        self.broker.register_callback(self.on_mqtt_message)
        self.state: Dict[str, Any] = {
            "esphome": {
                "online": False,
                "ip": "Unknown",
                "temperature": 0.0,
                "humidity": 0.0,
                "button": "RELEASED",
                "relay": "OFF",
                "analog": 0,
                "last_seen": 0,
            },
            "tasmota": {
                "online": False,
                "ip": "Unknown",
                "temperature": 0.0,
                "humidity": 0.0,
                "button": "RELEASED",
                "relay": "OFF",
                "vcc": 0.0,
                "wifi_rssi": 0,
                "last_seen": 0,
            },
            "ai_agent": {
                "auto_cooling_enabled": True,
                "threshold_temp_c": 28.5,
                "last_decision": "System initialized. Waiting for telemetry.",
                "decision_history": [],
            },
        }

    def on_mqtt_message(self, client_id: str, topic: str, payload_bytes: bytes):
        now = time.time()
        payload_str = payload_bytes.decode("utf-8", errors="ignore").strip()

        # -------------------------------------------------------------
        # 1. ESPHome Ingest Dialect
        # -------------------------------------------------------------
        if topic.startswith("edge/esphome/"):
            subtopic = topic[len("edge/esphome/"):]
            self.state["esphome"]["online"] = True
            self.state["esphome"]["last_seen"] = now

            if subtopic == "status":
                self.state["esphome"]["online"] = (payload_str.lower() == "online")

            elif subtopic == "sensor/ambient_temperature/state":
                try:
                    val = float(payload_str)
                    self.state["esphome"]["temperature"] = val
                    self._run_ai_rules("esphome", val)
                except ValueError:
                    pass

            elif subtopic == "sensor/ambient_humidity/state":
                try:
                    self.state["esphome"]["humidity"] = float(payload_str)
                except ValueError:
                    pass

            elif subtopic == "binary_sensor/physical_button/state":
                self.state["esphome"]["button"] = payload_str.upper()

            elif subtopic == "switch/living_fan_relay/state":
                self.state["esphome"]["relay"] = payload_str.upper()

            elif subtopic == "sensor/analog_input/state":
                try:
                    self.state["esphome"]["analog"] = int(float(payload_str))
                except ValueError:
                    pass

        # -------------------------------------------------------------
        # 2. Tasmota Ingest Dialect
        # -------------------------------------------------------------
        elif "edge_tasmota" in topic:
            self.state["tasmota"]["online"] = True
            self.state["tasmota"]["last_seen"] = now

            if topic.endswith("/LWT"):
                self.state["tasmota"]["online"] = (payload_str.lower() == "online")

            elif topic.endswith("/STATE"):
                try:
                    data = json.loads(payload_str)
                    self.state["tasmota"]["relay"] = data.get("POWER", "OFF").upper()
                    self.state["tasmota"]["wifi_rssi"] = data.get("Wifi", {}).get("RSSI", 0)
                    self.state["tasmota"]["vcc"] = data.get("Vcc", 0.0)
                except json.JSONDecodeError:
                    pass

            elif topic.endswith("/SENSOR"):
                try:
                    data = json.loads(payload_str)
                    # Extract DHT11 or environmental readings
                    dht = data.get("DHT11", {}) or data.get("AM2301", {}) or data.get("SI7021", {})
                    if dht:
                        temp = float(dht.get("Temperature", 0.0))
                        hum = float(dht.get("Humidity", 0.0))
                        self.state["tasmota"]["temperature"] = temp
                        self.state["tasmota"]["humidity"] = hum
                        self._run_ai_rules("tasmota", temp)
                except (json.JSONDecodeError, ValueError):
                    pass

            elif topic.endswith("/stat/POWER") or topic.endswith("/POWER"):
                self.state["tasmota"]["relay"] = payload_str.upper()

    def _run_ai_rules(self, source_device: str, current_temp: float):
        """
        AI Decision Controller:
        Monitors incoming environmental metrics and triggers edge actuators
        with full audit trail logging.
        """
        cfg = self.state["ai_agent"]
        if not cfg["auto_cooling_enabled"]:
            return

        threshold = cfg["threshold_temp_c"]
        current_relay = self.state[source_device]["relay"]

        if current_temp > threshold and current_relay != "ON":
            decision = (
                f"[AI AGENT] High temperature alert on {source_device.upper()}: "
                f"{current_temp:.1f}°C > {threshold}°C threshold. Triggering COOLING FAN -> ON."
            )
            LOGGER.info(decision)
            self._record_decision(decision)
            try:
                loop = asyncio.get_running_loop()
                loop.create_task(self.set_device_relay(source_device, "ON"))
            except RuntimeError:
                pass

        elif current_temp <= (threshold - 1.5) and current_relay == "ON":
            decision = (
                f"[AI AGENT] Temperature normalized on {source_device.upper()}: "
                f"{current_temp:.1f}°C <= {threshold - 1.5:.1f}°C. Restoring COOLING FAN -> OFF."
            )
            LOGGER.info(decision)
            self._record_decision(decision)
            try:
                loop = asyncio.get_running_loop()
                loop.create_task(self.set_device_relay(source_device, "OFF"))
            except RuntimeError:
                pass

    def _record_decision(self, text: str):
        self.state["ai_agent"]["last_decision"] = text
        history = self.state["ai_agent"]["decision_history"]
        history.insert(0, f"[{time.strftime('%H:%M:%S')}] {text}")
        if len(history) > 20:
            history.pop()

    async def set_device_relay(self, device: str, target_state: str):
        target_state = target_state.upper()
        if device == "esphome":
            # ESPHome MQTT switch topic
            topic = "edge/esphome/switch/living_fan_relay/command"
            await self.broker.publish(topic, target_state)
            LOGGER.info(f"Published to ESPHome: {topic} -> {target_state}")
        elif device == "tasmota":
            # Tasmota standard cmnd topic
            topic = "cmnd/edge_tasmota/POWER"
            await self.broker.publish(topic, target_state)
            LOGGER.info(f"Published to Tasmota: {topic} -> {target_state}")


# ============================================================================
# Lightweight HTTP & Web Dashboard Server
# ============================================================================

HTML_DASHBOARD = """<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Self-Hosted IoT Server | ESPHome vs Tasmota</title>
  <script src="https://cdn.tailwindcss.com"></script>
  <style>
    @keyframes pulse-slow { 0%, 100% { opacity: 1; } 50% { opacity: 0.4; } }
    .pulse-dot { animation: pulse-slow 2s infinite ease-in-out; }
  </style>
</head>
<body class="bg-slate-950 text-slate-100 min-h-screen p-4 md:p-8 font-sans">
  <div class="max-w-6xl mx-auto space-y-8">
    
    <!-- Header -->
    <header class="flex flex-col md:flex-row justify-between items-start md:items-center border-b border-slate-800 pb-6 gap-4">
      <div>
        <div class="flex items-center gap-3">
          <span class="inline-block w-3 h-3 rounded-full bg-emerald-500 pulse-dot"></span>
          <h1 class="text-2xl font-bold tracking-tight text-white">Self-Hosted IoT Edge Server</h1>
          <span class="text-xs bg-slate-800 text-slate-400 px-2 py-1 rounded">No Home Assistant Required</span>
        </div>
        <p class="text-sm text-slate-400 mt-1">Dual-Approach Benchmark: Compile-Time (ESPHome) vs Runtime Monolith (Tasmota)</p>
      </div>
      <div class="flex items-center gap-4 text-xs">
        <div class="bg-slate-900 border border-slate-800 px-3 py-2 rounded-lg">
          <span class="text-slate-500">MQTT Port:</span> <span class="font-mono text-emerald-400 font-bold">1883</span>
        </div>
        <div class="bg-slate-900 border border-slate-800 px-3 py-2 rounded-lg">
          <span class="text-slate-500">HTTP Port:</span> <span class="font-mono text-cyan-400 font-bold">8080</span>
        </div>
      </div>
    </header>

    <!-- Dual Device Comparative Grid -->
    <div class="grid grid-cols-1 md:grid-cols-2 gap-6">

      <!-- ESPHome Card -->
      <div class="bg-slate-900 border border-emerald-900/40 rounded-xl p-6 shadow-xl relative overflow-hidden">
        <div class="absolute top-0 right-0 w-32 h-32 bg-emerald-500/5 rounded-full blur-2xl"></div>
        <div class="flex justify-between items-center mb-4">
          <div class="flex items-center gap-2">
            <span id="esphome-online-badge" class="w-2.5 h-2.5 rounded-full bg-slate-600"></span>
            <h2 class="text-lg font-semibold text-emerald-400">POC-A: ESPHome Edge</h2>
          </div>
          <span class="text-xs bg-emerald-950 text-emerald-300 border border-emerald-800/50 px-2 py-0.5 rounded font-mono">Compile-Time CaC</span>
        </div>
        <p class="text-xs text-slate-400 mb-6">Topic: <code class="text-emerald-300">edge/esphome/#</code> | Framework: ESP-IDF Native</p>

        <div class="grid grid-cols-2 gap-4 mb-6">
          <div class="bg-slate-950 p-4 rounded-lg border border-slate-800/80">
            <span class="text-xs text-slate-500 block mb-1">Temperature</span>
            <span id="esphome-temp" class="text-3xl font-bold font-mono text-slate-200">--.-</span>
            <span class="text-xs text-slate-500 ml-1">°C</span>
          </div>
          <div class="bg-slate-950 p-4 rounded-lg border border-slate-800/80">
            <span class="text-xs text-slate-500 block mb-1">Humidity</span>
            <span id="esphome-hum" class="text-3xl font-bold font-mono text-slate-200">--.-</span>
            <span class="text-xs text-slate-500 ml-1">%</span>
          </div>
          <div class="bg-slate-950 p-3 rounded-lg border border-slate-800/80">
            <span class="text-xs text-slate-500 block">Physical Button (GPIO18)</span>
            <span id="esphome-button" class="text-sm font-semibold font-mono text-slate-400">IDLE</span>
          </div>
          <div class="bg-slate-950 p-3 rounded-lg border border-slate-800/80">
            <span class="text-xs text-slate-500 block">Analog ADC1 (GPIO32)</span>
            <span id="esphome-analog" class="text-sm font-semibold font-mono text-slate-400">--</span>
          </div>
        </div>

        <div class="flex items-center justify-between pt-4 border-t border-slate-800">
          <div>
            <span class="text-xs text-slate-500 block">Relay Actuator (GPIO23)</span>
            <span id="esphome-relay" class="text-base font-bold font-mono text-slate-400">OFF</span>
          </div>
          <div class="flex gap-2">
            <button onclick="controlRelay('esphome', 'ON')" class="px-3 py-1.5 bg-emerald-600 hover:bg-emerald-500 text-white rounded text-xs font-semibold transition">Turn ON</button>
            <button onclick="controlRelay('esphome', 'OFF')" class="px-3 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded text-xs font-semibold transition">Turn OFF</button>
          </div>
        </div>
      </div>

      <!-- Tasmota Card -->
      <div class="bg-slate-900 border border-cyan-900/40 rounded-xl p-6 shadow-xl relative overflow-hidden">
        <div class="absolute top-0 right-0 w-32 h-32 bg-cyan-500/5 rounded-full blur-2xl"></div>
        <div class="flex justify-between items-center mb-4">
          <div class="flex items-center gap-2">
            <span id="tasmota-online-badge" class="w-2.5 h-2.5 rounded-full bg-slate-600"></span>
            <h2 class="text-lg font-semibold text-cyan-400">POC-B: Tasmota Edge</h2>
          </div>
          <span class="text-xs bg-cyan-950 text-cyan-300 border border-cyan-800/50 px-2 py-0.5 rounded font-mono">Runtime Monolith</span>
        </div>
        <p class="text-xs text-slate-400 mb-6">Topic: <code class="text-cyan-300">tele/edge_tasmota/#</code> | Framework: Arduino Core</p>

        <div class="grid grid-cols-2 gap-4 mb-6">
          <div class="bg-slate-950 p-4 rounded-lg border border-slate-800/80">
            <span class="text-xs text-slate-500 block mb-1">Temperature</span>
            <span id="tasmota-temp" class="text-3xl font-bold font-mono text-slate-200">--.-</span>
            <span class="text-xs text-slate-500 ml-1">°C</span>
          </div>
          <div class="bg-slate-950 p-4 rounded-lg border border-slate-800/80">
            <span class="text-xs text-slate-500 block mb-1">Humidity</span>
            <span id="tasmota-hum" class="text-3xl font-bold font-mono text-slate-200">--.-</span>
            <span class="text-xs text-slate-500 ml-1">%</span>
          </div>
          <div class="bg-slate-950 p-3 rounded-lg border border-slate-800/80">
            <span class="text-xs text-slate-500 block">Wi-Fi RSSI</span>
            <span id="tasmota-rssi" class="text-sm font-semibold font-mono text-slate-400">-- %</span>
          </div>
          <div class="bg-slate-950 p-3 rounded-lg border border-slate-800/80">
            <span class="text-xs text-slate-500 block">Supply Vcc</span>
            <span id="tasmota-vcc" class="text-sm font-semibold font-mono text-slate-400">-- V</span>
          </div>
        </div>

        <div class="flex items-center justify-between pt-4 border-t border-slate-800">
          <div>
            <span class="text-xs text-slate-500 block">Relay Actuator (GPIO23)</span>
            <span id="tasmota-relay" class="text-base font-bold font-mono text-slate-400">OFF</span>
          </div>
          <div class="flex gap-2">
            <button onclick="controlRelay('tasmota', 'ON')" class="px-3 py-1.5 bg-cyan-600 hover:bg-cyan-500 text-white rounded text-xs font-semibold transition">Turn ON</button>
            <button onclick="controlRelay('tasmota', 'OFF')" class="px-3 py-1.5 bg-slate-800 hover:bg-slate-700 text-slate-300 rounded text-xs font-semibold transition">Turn OFF</button>
          </div>
        </div>
      </div>

    </div>

    <!-- AI Controller & Decision Audit Log -->
    <div class="bg-slate-900 border border-slate-800 rounded-xl p-6 shadow-xl">
      <div class="flex flex-col md:flex-row justify-between items-start md:items-center mb-4 gap-2">
        <div class="flex items-center gap-2">
          <span class="text-base">🤖</span>
          <h3 class="text-base font-semibold text-white">AI Decision Engine & Automation Audit Trail</h3>
        </div>
        <div class="flex items-center gap-3 text-xs">
          <label class="flex items-center gap-2 cursor-pointer">
            <input type="checkbox" id="ai-toggle" checked onchange="toggleAI(this.checked)" class="rounded bg-slate-800 border-slate-700 text-emerald-500">
            <span class="text-slate-300">Auto-Cooling Heuristic Active (> 28.5°C)</span>
          </label>
        </div>
      </div>

      <div class="bg-slate-950 p-4 rounded-lg border border-slate-800/80 mb-4">
        <span class="text-xs text-slate-500 block mb-1">Latest Intelligent Decision:</span>
        <p id="ai-decision-text" class="text-sm font-mono text-emerald-400">Awaiting telemetry...</p>
      </div>

      <div>
        <span class="text-xs text-slate-500 block mb-2">Decision Log History:</span>
        <div id="ai-history" class="space-y-1 max-h-36 overflow-y-auto text-xs font-mono text-slate-400">
          <p class="text-slate-600 italic">No historical events yet.</p>
        </div>
      </div>
    </div>

  </div>

  <script>
    async function updateDashboard() {
      try {
        const res = await fetch('/api/state');
        if (!res.ok) return;
        const data = await res.json();

        // Update ESPHome
        const esp = data.esphome;
        document.getElementById('esphome-online-badge').className = `w-2.5 h-2.5 rounded-full ${esp.online ? 'bg-emerald-500' : 'bg-rose-500'}`;
        document.getElementById('esphome-temp').innerText = esp.temperature > 0 ? esp.temperature.toFixed(1) : '--.-';
        document.getElementById('esphome-hum').innerText = esp.humidity > 0 ? esp.humidity.toFixed(1) : '--.-';
        document.getElementById('esphome-button').innerText = esp.button;
        document.getElementById('esphome-analog').innerText = esp.analog;
        const espRelay = document.getElementById('esphome-relay');
        espRelay.innerText = esp.relay;
        espRelay.className = `text-base font-bold font-mono ${esp.relay === 'ON' ? 'text-emerald-400' : 'text-slate-400'}`;

        // Update Tasmota
        const tas = data.tasmota;
        document.getElementById('tasmota-online-badge').className = `w-2.5 h-2.5 rounded-full ${tas.online ? 'bg-cyan-500' : 'bg-rose-500'}`;
        document.getElementById('tasmota-temp').innerText = tas.temperature > 0 ? tas.temperature.toFixed(1) : '--.-';
        document.getElementById('tasmota-hum').innerText = tas.humidity > 0 ? tas.humidity.toFixed(1) : '--.-';
        document.getElementById('tasmota-rssi').innerText = tas.wifi_rssi ? `${tas.wifi_rssi} %` : '-- %';
        document.getElementById('tasmota-vcc').innerText = tas.vcc ? `${tas.vcc} V` : '-- V';
        const tasRelay = document.getElementById('tasmota-relay');
        tasRelay.innerText = tas.relay;
        tasRelay.className = `text-base font-bold font-mono ${tas.relay === 'ON' ? 'text-cyan-400' : 'text-slate-400'}`;

        // Update AI
        const ai = data.ai_agent;
        document.getElementById('ai-decision-text').innerText = ai.last_decision;
        if (ai.decision_history && ai.decision_history.length > 0) {
          document.getElementById('ai-history').innerHTML = ai.decision_history.map(item => `<div>${item}</div>`).join('');
        }
      } catch (err) {
        console.error(err);
      }
    }

    async function controlRelay(device, state) {
      await fetch(`/api/control/${device}?state=${state}`, { method: 'POST' });
      setTimeout(updateDashboard, 150);
    }

    async function toggleAI(enabled) {
      await fetch(`/api/ai/toggle?enabled=${enabled}`, { method: 'POST' });
    }

    setInterval(updateDashboard, 1000);
    updateDashboard();
  </script>
</body>
</html>
"""


class HttpServer:
    def __init__(self, controller: SmartIoTController, host: str = "0.0.0.0", port: int = 8080):
        self.controller = controller
        self.host = host
        self.port = port
        self.server: asyncio.Server | None = None

    async def start(self):
        self.server = await asyncio.start_server(self.handle_http, self.host, self.port)
        LOGGER.info(f"HTTP Dashboard listening on http://{self.host}:{self.port}")

    async def handle_http(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        try:
            line = await reader.readline()
            if not line:
                return
            request_line = line.decode("utf-8", errors="ignore").strip()
            parts = request_line.split(" ")
            if len(parts) < 2:
                return
            method, full_path = parts[0], parts[1]

            # Read headers until empty line
            while True:
                h_line = await reader.readline()
                if not h_line or h_line == b"\r\n":
                    break

            path = full_path.split("?")[0]
            query = full_path.split("?")[1] if "?" in full_path else ""

            # Route 1: Dashboard UI
            if path in ("/", "/index.html"):
                body = HTML_DASHBOARD.encode("utf-8")
                headers = (
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html; charset=utf-8\r\n"
                    f"Content-Length: {len(body)}\r\n"
                    "Connection: close\r\n\r\n"
                )
                writer.write(headers.encode("utf-8") + body)

            # Route 2: GET State API
            elif path == "/api/state":
                body = json.dumps(self.controller.state).encode("utf-8")
                headers = (
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    f"Content-Length: {len(body)}\r\n"
                    "Connection: close\r\n\r\n"
                )
                writer.write(headers.encode("utf-8") + body)

            # Route 3: POST Control Relay
            elif path.startswith("/api/control/"):
                device = path[len("/api/control/"):]
                state = "ON"
                if "state=" in query:
                    state = query.split("state=")[1].split("&")[0].upper()
                await self.controller.set_device_relay(device, state)
                body = json.dumps({"status": "ok", "device": device, "applied_state": state}).encode("utf-8")
                headers = (
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    f"Content-Length: {len(body)}\r\n"
                    "Connection: close\r\n\r\n"
                )
                writer.write(headers.encode("utf-8") + body)

            # Route 4: POST Toggle AI
            elif path == "/api/ai/toggle":
                enabled = "true" in query.lower()
                self.controller.state["ai_agent"]["auto_cooling_enabled"] = enabled
                body = json.dumps({"status": "ok", "auto_cooling": enabled}).encode("utf-8")
                headers = (
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    f"Content-Length: {len(body)}\r\n"
                    "Connection: close\r\n\r\n"
                )
                writer.write(headers.encode("utf-8") + body)

            else:
                body = b"404 Not Found"
                headers = (
                    "HTTP/1.1 404 Not Found\r\n"
                    f"Content-Length: {len(body)}\r\n"
                    "Connection: close\r\n\r\n"
                )
                writer.write(headers.encode("utf-8") + body)

            await writer.drain()
        except Exception as e:
            LOGGER.debug(f"HTTP handler error: {e}")
        finally:
            try:
                writer.close()
                await writer.wait_closed()
            except Exception:
                pass


# ============================================================================
# Main Entry Point
# ============================================================================

async def main():
    mqtt_port = int(os.environ.get("MQTT_PORT", "1883"))
    http_port = int(os.environ.get("HTTP_PORT", "8080"))

    broker = MQTTBroker(host="0.0.0.0", port=mqtt_port)
    await broker.start()

    controller = SmartIoTController(broker)
    http_server = HttpServer(controller, host="0.0.0.0", port=http_port)
    await http_server.start()

    LOGGER.info("=================================================================")
    LOGGER.info("🚀 SELF-HOSTED EDGE IOT SERVER STARTED SUCCESSFULLY")
    LOGGER.info(f"📡 MQTT Broker on tcp://0.0.0.0:{mqtt_port}")
    LOGGER.info(f"🌐 Web Dashboard on http://localhost:{http_port}")
    LOGGER.info("💡 Ready to ingest ESPHome (edge/esphome/#) & Tasmota (tele/#)")
    LOGGER.info("=================================================================")

    # Keep server running
    while True:
        await asyncio.sleep(3600)


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        LOGGER.info("Server terminated by user.")
