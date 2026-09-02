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

void CloudClient::setApplyStateHandler(ApplyStateHandler handler) {
  applyStateHandler_ = handler;
}

void CloudClient::setReportedState(bool on) {
  reportedOn_ = on;
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

  // Thiết lập các header và cấu hình SSL cho client
  webSocket_.setInsecure();
  webSocket_.addHeader("Authorization", String("Bearer ") + deviceToken_);
  webSocket_.addHeader("ngrok-skip-browser-warning", "69420");
  webSocket_.addHeader("User-Agent", "ESP32-Client");



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
  }

  if (state_ == State::TimeSync) {
    if (systemTimeValid()) {
      Serial.printf("TIME_SYNCED epoch=%lld\r\n",
                    static_cast<long long>(time(nullptr)));
      startTransport();
    } else if (now - timeSyncStartedAt_ >= TIME_SYNC_TIMEOUT_MS) {
      state_ = State::RetryWait;
      lastError_ = F("time_sync_timeout");
      timeSyncRetryAt_ = now + nextReconnectDelay();
      Serial.printf("TIME_SYNC_RETRY_AT_MS %lu\r\n",
                    static_cast<unsigned long>(timeSyncRetryAt_));
    }
    return;
  }

  if (state_ == State::RetryWait && deadlineReached(now, reconnectAt_)) {
    startTransport();
    return;
  }

  if (webSocket_.available()) {
    webSocket_.poll();
  }

  // Ping heartbeat định kỳ để duy trì kết nối
  if (state_ == State::Online && now - lastPingSentAt_ >= WEBSOCKET_PING_INTERVAL_MS) {
    lastPingSentAt_ = now;
    webSocket_.ping();
  }

  if (state_ == State::Authenticating &&
      now - helloStartedAt_ >= APP_HELLO_TIMEOUT_MS) {
    lastError_ = F("hello_timeout");
    Serial.println(F("WSS_HELLO_TIMEOUT"));
    webSocket_.close();
  }
}

bool CloudClient::online() const {
  return state_ == State::Online;
}

CloudClient::State CloudClient::state() const {
  return state_;
}

const char *CloudClient::stateName() const {
  switch (state_) {
    case State::WaitingForWifi:
      return "waiting_for_wifi";
    case State::TimeSync:
      return "time_sync";
    case State::Connecting:
      return "connecting";
    case State::Authenticating:
      return "authenticating";
    case State::Online:
      return "online";
    case State::RetryWait:
      return "retry_wait";
    case State::Disabled:
    default:
      return "disabled";
  }
}

const String &CloudClient::lastError() const {
  return lastError_;
}

void CloudClient::startTimeSync(uint32_t now) {
  state_ = State::TimeSync;
  timeSyncStartedAt_ = now;
  lastError_ = "";
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println(F("TIME_SYNC_STARTED"));
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
      // Handled in connect() return
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

  if (strcmp(type, "set_state") == 0) {
    const String commandId = document["command_id"] | "";
    bool hasOn = false;
    bool desiredOn = false;

    if (document["on"].is<bool>()) {
      hasOn = true;
      desiredOn = document["on"].as<bool>();
    } else if (document["desired"].is<JsonObject>() && document["desired"]["on"].is<bool>()) {
      hasOn = true;
      desiredOn = document["desired"]["on"].as<bool>();
    }

    if (!validCommandId(commandId) || !hasOn) {
      lastError_ = F("invalid_set_state");
      Serial.println(F("WSS_PROTOCOL_ERROR reason=invalid_set_state"));
      webSocket_.close();
      return;
    }

    Serial.printf("COMMAND_RECEIVED id=%s on=%s\r\n",
                  commandId.c_str(), desiredOn ? "true" : "false");

    bool confirmedOn = desiredOn;
    if (applyStateHandler_) {
      confirmedOn = applyStateHandler_(desiredOn);
    }
    reportedOn_ = confirmedOn;
    sendStateReport(commandId, confirmedOn);
    return;
  }

  lastError_ = F("unknown_message_type");
  Serial.printf("WSS_UNKNOWN_TYPE type=%s\r\n", type);
}

void CloudClient::sendHello() {
  JsonDocument document;
  document["v"] = 1;
  document["type"] = "hello";
  document["device_id"] = deviceId_;
  document["firmware"] = "poc5-cloud-device-1.0.0";
  JsonObject reported = document["reported"].to<JsonObject>();
  reported["on"] = reportedOn_;

  String payload;
  serializeJson(document, payload);
  webSocket_.send(payload);
  Serial.printf("WSS_HELLO_SENT reported_on=%s\r\n",
                reportedOn_ ? "true" : "false");
}

void CloudClient::sendStateReport(const String &commandId, bool on) {
  JsonDocument document;
  document["v"] = 1;
  document["type"] = "state_report";
  document["device_id"] = deviceId_;
  document["command_id"] = commandId;
  document["on"] = on;

  String payload;
  serializeJson(document, payload);
  webSocket_.send(payload);
  Serial.printf("STATE_REPORT_SENT id=%s on=%s\r\n",
                commandId.c_str(), on ? "true" : "false");
}

uint32_t CloudClient::nextReconnectDelay() {
  // Backoff: 2s, 4s, 8s, 16s, max 30s
  static const uint32_t delays[] = {2000, 4000, 8000, 16000, 30000};
  const size_t maxIndex = sizeof(delays) / sizeof(delays[0]) - 1;
  const size_t index = (reconnectAttempt_ > maxIndex) ? maxIndex : reconnectAttempt_;
  if (reconnectAttempt_ < 255) {
    ++reconnectAttempt_;
  }
  return delays[index];
}

bool CloudClient::validCommandId(const String &value) const {
  if (value.isEmpty() || value.length() > 64) {
    return false;
  }
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    const bool valid = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                       (c >= '0' && c <= '9') || c == '-' || c == '_';
    if (!valid) {
      return false;
    }
  }
  return true;
}
