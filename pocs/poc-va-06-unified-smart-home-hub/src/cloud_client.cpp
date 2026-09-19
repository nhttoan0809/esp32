#include "cloud_client.h"

#include <ArduinoJson.h>
#include <time.h>

#include "app_config.h"
#include "tls_ca.h"

using namespace app_config;

namespace {

bool deadlineReached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

bool systemTimeValid() {
  return time(nullptr) >= 1700000000;
}

}  // namespace

void CloudClient::setApplyRelayHandler(ApplyRelayHandler handler) {
  applyRelayHandler_ = handler;
}

void CloudClient::setApplyBrightnessHandler(ApplyBrightnessHandler handler) {
  applyBrightnessHandler_ = handler;
}

void CloudClient::setApplySecurityModeHandler(ApplySecurityModeHandler handler) {
  applySecurityModeHandler_ = handler;
}

void CloudClient::begin(const DeviceConfig &config, const char *deviceId,
                        const char *deviceToken) {
  stop();
  config_ = config;
  deviceId_ = deviceId;
  deviceToken_ = deviceToken;

  socketPath_ = config_.serverPath;
  if (socketPath_.endsWith("/")) {
    socketPath_.remove(socketPath_.length() - 1);
  }
  socketPath_ += '/';
  socketPath_ += deviceId_;

  const String scheme = (config_.serverPort == 443) ? "wss://" : "ws://";
  String portPart = "";
  if (config_.serverPort != 80 && config_.serverPort != 443 && config_.serverPort != 0) {
    portPart = ":" + String(config_.serverPort);
  }
  wsUrl_ = scheme + config_.serverHost + portPart + socketPath_;

  if (config_.serverPort == 443) {
    webSocket_.setCACert(SERVER_ROOT_CA);
  }
  webSocket_.addHeader("Authorization", String("Bearer ") + deviceToken_);
  webSocket_.addHeader("ngrok-skip-browser-warning", "69420");
  webSocket_.addHeader("User-Agent", "ESP32-Smart-Home-Hub");

  webSocket_.onMessage([this](websockets::WebsocketsMessage message) {
    handleMessage(message.data());
  });

  webSocket_.onEvent([this](websockets::WebsocketsEvent event, String data) {
    handleEvent(event, data);
  });

  configured_ = true;
  state_ = State::WaitingForWifi;
  lastError_ = "";
  reconnectAttempt_ = 0;

  Serial.printf("CLOUD_CONFIGURED url=%s device_id=%s\r\n",
                wsUrl_.c_str(), deviceId_.c_str());
}

void CloudClient::stop() {
  configured_ = false;
  if (webSocket_.available()) {
    webSocket_.close();
  }
  wifiWasConnected_ = false;
  state_ = State::Disabled;
  reconnectAttempt_ = 0;
  clearSensitiveString(deviceToken_);
  socketPath_ = "";
  wsUrl_ = "";
  deviceId_ = "";
  clearDeviceConfig(config_);
}

void CloudClient::loop(bool wifiConnected) {
  if (!configured_) return;

  const uint32_t now = millis();
  if (!wifiConnected) {
    if (wifiWasConnected_) {
      wifiWasConnected_ = false;
      if (webSocket_.available()) {
        webSocket_.close();
      }
      state_ = State::WaitingForWifi;
      lastError_ = F("wifi_disconnected");
      Serial.println(F("CLOUD_PAUSED reason=wifi_disconnected"));
    }
    return;
  }

  if (!wifiWasConnected_) {
    wifiWasConnected_ = true;
    startTimeSync(now);
    return;
  }

  webSocket_.poll();

  switch (state_) {
    case State::Disabled:
    case State::WaitingForWifi:
      break;

    case State::TimeSync:
      if (systemTimeValid()) {
        Serial.println(F("NTP_SYNC_OK"));
        startTransport();
      } else if (deadlineReached(now, timeSyncStartedAt_ + 10000)) {
        if (deadlineReached(now, timeSyncRetryAt_)) {
          timeSyncRetryAt_ = now + 5000;
          configTime(0, 0, "pool.ntp.org", "time.google.com");
        }
      }
      break;

    case State::Connecting:
      break;

    case State::Authenticating:
      if (deadlineReached(now, helloStartedAt_ + 5000)) {
        lastError_ = F("hello_timeout");
        Serial.println(F("WSS_AUTH_TIMEOUT"));
        webSocket_.close();
      }
      break;

    case State::Online:
      if (deadlineReached(now, lastPingSentAt_ + 15000)) {
        lastPingSentAt_ = now;
        webSocket_.ping();
      }
      break;

    case State::RetryWait:
      if (deadlineReached(now, reconnectAt_)) {
        startTransport();
      }
      break;
  }
}

void CloudClient::startTimeSync(uint32_t now) {
  state_ = State::TimeSync;
  timeSyncStartedAt_ = now;
  timeSyncRetryAt_ = now + 5000;

  if (systemTimeValid()) {
    startTransport();
    return;
  }

  Serial.println(F("NTP_SYNC_STARTING"));
  configTime(0, 0, "pool.ntp.org", "time.google.com");
}

void CloudClient::startTransport() {
  state_ = State::Connecting;
  Serial.printf("WSS_CONNECTING url=%s\r\n", wsUrl_.c_str());

  const bool ok = webSocket_.connect(wsUrl_);
  if (ok) {
    Serial.println(F("WSS_UPGRADED"));
    state_ = State::Authenticating;
    helloStartedAt_ = millis();
    lastPingSentAt_ = millis();
    sendHello();
  } else {
    state_ = State::RetryWait;
    const uint32_t delayMs = nextReconnectDelay();
    reconnectAt_ = millis() + delayMs;
    Serial.printf("WSS_CONNECT_FAILED retry_ms=%lu\r\n",
                  static_cast<unsigned long>(delayMs));
  }
}

void CloudClient::handleEvent(websockets::WebsocketsEvent event, const String &data) {
  switch (event) {
    case websockets::WebsocketsEvent::ConnectionOpened:
      break;
    case websockets::WebsocketsEvent::ConnectionClosed: {
      if (!configured_) return;
      state_ = State::RetryWait;
      const uint32_t delayMs = nextReconnectDelay();
      reconnectAt_ = millis() + delayMs;
      lastError_ = F("websocket_disconnected");
      Serial.printf("WSS_DISCONNECTED retry_ms=%lu\r\n",
                    static_cast<unsigned long>(delayMs));
      break;
    }
    case websockets::WebsocketsEvent::GotPing:
      webSocket_.pong();
      break;
    case websockets::WebsocketsEvent::GotPong:
      break;
  }
}

void CloudClient::handleMessage(const String &payload) {
  if (payload.length() == 0 || payload.length() > MAX_WEBSOCKET_MESSAGE_BYTES) {
    webSocket_.close();
    return;
  }

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, payload);
  if (error || !document.is<JsonObject>()) {
    webSocket_.close();
    return;
  }

  const int version = document["v"] | 0;
  const char *type = document["type"] | "";
  const char *incomingDeviceId = document["device_id"] | "";

  if (version != 1 || strcmp(incomingDeviceId, deviceId_.c_str()) != 0) {
    webSocket_.close();
    return;
  }

  if (strcmp(type, "ready") == 0) {
    state_ = State::Online;
    reconnectAttempt_ = 0;
    lastError_ = "";
    Serial.println(F("WSS_AUTHENTICATED"));
    return;
  }

  // 1. Relay Lamp Control: set_state
  if (strcmp(type, "set_state") == 0) {
    const String commandId = document["command_id"] | "";
    bool desiredOn = false;
    if (document["on"].is<bool>()) {
      desiredOn = document["on"].as<bool>();
    } else if (document["desired"]["on"].is<bool>()) {
      desiredOn = document["desired"]["on"].as<bool>();
    }

    Serial.printf("[CMD] Relay Lamp set_state=%s\r\n", desiredOn ? "true" : "false");
    bool confirmedOn = desiredOn;
    if (applyRelayHandler_) {
      confirmedOn = applyRelayHandler_(desiredOn);
    }
    sendStateReport(commandId, confirmedOn);
    return;
  }

  // 2. Dimmer Brightness Control: set_brightness
  if (strcmp(type, "set_brightness") == 0) {
    const String commandId = document["command_id"] | "";
    const uint8_t brightness = document["brightness"] | 0;

    Serial.printf("[CMD] Dimmer set_brightness=%u%%\r\n", brightness);
    uint8_t confirmedBrightness = brightness;
    if (applyBrightnessHandler_) {
      confirmedBrightness = applyBrightnessHandler_(brightness);
    }
    sendBrightnessReport(commandId, confirmedBrightness);
    return;
  }

  // 3. Security Mode Control: set_security_mode
  if (strcmp(type, "set_security_mode") == 0) {
    const String commandId = document["command_id"] | "";
    const char *mode = document["mode"] | "guard";
    const bool isGuard = (strcmp(mode, "guard") == 0);

    Serial.printf("[CMD] Security mode set=%s\r\n", mode);
    if (applySecurityModeHandler_) {
      applySecurityModeHandler_(isGuard);
    }
    return;
  }
}

void CloudClient::sendHello() {
  JsonDocument document;
  document["v"] = 1;
  document["type"] = "hello";
  document["device_id"] = deviceId_;
  document["firmware"] = "poc-va-06-hub-1.0.0";
  JsonObject reported = document["reported"].to<JsonObject>();
  reported["on"] = false;

  String payload;
  serializeJson(document, payload);
  webSocket_.send(payload);
  Serial.printf("WSS_HELLO_SENT device_id=%s\r\n", deviceId_.c_str());
}

void CloudClient::sendStateReport(const String &commandId, bool on) {
  JsonDocument document;
  document["v"] = 1;
  document["type"] = "state_report";
  document["device_id"] = deviceId_;
  document["command_id"] = commandId.length() > 0 ? commandId : "00000000-0000-0000-0000-000000000000";
  document["on"] = on;

  String payload;
  serializeJson(document, payload);
  webSocket_.send(payload);
}

void CloudClient::sendBrightnessReport(const String &commandId, uint8_t brightness) {
  JsonDocument document;
  document["v"] = 1;
  document["type"] = "brightness_report";
  document["device_id"] = deviceId_;
  document["command_id"] = commandId.length() > 0 ? commandId : "00000000-0000-0000-0000-000000000000";
  document["brightness"] = brightness;

  String payload;
  serializeJson(document, payload);
  webSocket_.send(payload);
}

void CloudClient::reportTelemetry(const SystemState &state) {
  if (!online()) return;

  JsonDocument document;
  document["v"] = 1;
  document["type"] = "sensor_data";
  document["device_id"] = deviceId_;

  JsonObject sensors = document["sensors"].to<JsonObject>();
  if (!isnan(state.temperature)) {
    sensors["temperature"] = round(state.temperature * 10.0f) / 10.0f;
  }
  if (!isnan(state.humidity)) {
    sensors["humidity"] = round(state.humidity * 10.0f) / 10.0f;
  }
  sensors["lamp_on"] = state.lampOn;
  sensors["led_brightness"] = state.ledBrightness;
  sensors["light_level"] = state.ldrDark ? "dark" : "bright";
  sensors["motion_detected"] = state.motionDetected;
  sensors["security_mode"] = state.guardMode ? "guard" : "eco";
  sensors["motion_count"] = state.motionCount;

  String payload;
  serializeJson(document, payload);
  webSocket_.send(payload);
  Serial.printf("TELEMETRY_SENT T=%.1f H=%.1f LDR=%s PIR=%s\r\n",
                state.temperature, state.humidity,
                state.ldrDark ? "dark" : "bright",
                state.motionDetected ? "motion" : "clear");
}

void CloudClient::reportRelayState(bool on) {
  if (!online()) return;
  sendStateReport("00000000-0000-0000-0000-000000000000", on);
}

void CloudClient::reportMotionEvent(bool detected) {
  if (!online()) return;
  JsonDocument document;
  document["v"] = 1;
  document["type"] = "sensor_data";
  document["device_id"] = deviceId_;
  JsonObject sensors = document["sensors"].to<JsonObject>();
  sensors["motion_detected"] = detected;

  String payload;
  serializeJson(document, payload);
  webSocket_.send(payload);
}

bool CloudClient::online() const {
  return state_ == State::Online;
}

CloudClient::State CloudClient::state() const {
  return state_;
}

const char *CloudClient::stateName() const {
  switch (state_) {
    case State::Disabled: return "Disabled";
    case State::WaitingForWifi: return "WaitingForWifi";
    case State::TimeSync: return "TimeSync";
    case State::Connecting: return "Connecting";
    case State::Authenticating: return "Authenticating";
    case State::Online: return "Online";
    case State::RetryWait: return "RetryWait";
    default: return "Unknown";
  }
}

const String &CloudClient::lastError() const {
  return lastError_;
}

uint32_t CloudClient::nextReconnectDelay() {
  if (reconnectAttempt_ < 5) {
    reconnectAttempt_++;
  }
  return 1000UL * (1UL << reconnectAttempt_);
}
