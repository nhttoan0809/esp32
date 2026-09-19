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

  // SSL CA cert setup
  if (config_.serverPort == 443) {
    webSocket_.setCACert(SERVER_ROOT_CA);
  }
  webSocket_.addHeader("Authorization", String("Bearer ") + deviceToken_);
  webSocket_.addHeader("ngrok-skip-browser-warning", "69420");
  webSocket_.addHeader("User-Agent", "ESP32-Climate-Voice-Monitor");

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
  if (!configured_) {
    return;
  }

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
          Serial.println(F("NTP_SYNC_RETRY"));
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
    Serial.println(F("NTP_ALREADY_VALID"));
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
      if (!configured_) {
        return;
      }
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
    lastError_ = F("invalid_message_size");
    Serial.println(F("WSS_PROTOCOL_ERROR reason=message_size"));
    webSocket_.close();
    return;
  }

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, payload);
  if (error || !document.is<JsonObject>()) {
    lastError_ = F("invalid_json");
    Serial.println(F("WSS_PROTOCOL_ERROR reason=invalid_json"));
    webSocket_.close();
    return;
  }

  const int version = document["v"] | 0;
  const char *type = document["type"] | "";
  const char *incomingDeviceId = document["device_id"] | "";

  if (version != 1 || strcmp(incomingDeviceId, deviceId_.c_str()) != 0) {
    lastError_ = F("protocol_mismatch");
    Serial.println(F("WSS_PROTOCOL_ERROR reason=version_or_device_id"));
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

  Serial.printf("WSS_MESSAGE type=%s\r\n", type);
}

void CloudClient::sendHello() {
  JsonDocument document;
  document["v"] = 1;
  document["type"] = "hello";
  document["device_id"] = deviceId_;
  document["firmware"] = "poc-va-01-climate-1.0.0";
  JsonObject reported = document["reported"].to<JsonObject>();
  reported["on"] = false;

  String payload;
  serializeJson(document, payload);
  webSocket_.send(payload);
  Serial.printf("WSS_HELLO_SENT device_id=%s\r\n", deviceId_.c_str());
}

void CloudClient::reportSensorData(float temperature, float humidity) {
  if (!online() || isnan(temperature) || isnan(humidity)) {
    return;
  }

  JsonDocument document;
  document["v"] = 1;
  document["type"] = "sensor_data";
  document["device_id"] = deviceId_;
  JsonObject sensors = document["sensors"].to<JsonObject>();
  sensors["temperature"] = round(temperature * 10.0f) / 10.0f;
  sensors["humidity"] = round(humidity * 10.0f) / 10.0f;

  String payload;
  serializeJson(document, payload);
  webSocket_.send(payload);

  lastReportedTemp_ = temperature;
  lastReportedHumid_ = humidity;
  Serial.printf("SENSOR_REPORT temp=%.1f humid=%.1f\r\n", temperature, humidity);
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
