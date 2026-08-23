#include "cloud_client.h"

#include <WiFiClient.h>
#include <WiFiClientSecure.h>

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
  authorizationHeader_ = F("Authorization: Bearer ");
  authorizationHeader_ += deviceToken_;
  authorizationHeader_ += F("\r\n");
  configured_ = true;
  state_ = State::WaitingForWifi;
  lastError_ = "";
  reconnectAttempt_ = 0;
  Serial.printf("CLOUD_CONFIGURED host=%s port=%u path=%s device_id=%s\r\n",
                config_.serverHost.c_str(), config_.serverPort,
                socketPath_.c_str(), deviceId_.c_str());
}

void CloudClient::stop() {
  configured_ = false;
  if (transportStarted_) {
    webSocket_.disconnect();
  }
  transportStarted_ = false;
  wifiWasConnected_ = false;
  state_ = State::Disabled;
  reconnectAttempt_ = 0;
  clearSensitiveString(deviceToken_);
  authorizationHeader_ = "";
  socketPath_ = "";
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
      if (transportStarted_) {
        webSocket_.disconnect();
        transportStarted_ = false;
      }
      state_ = State::WaitingForWifi;
      lastError_ = F("wifi_disconnected");
      Serial.println("CLOUD_PAUSED reason=wifi_disconnected");
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

  if (state_ == State::RetryWait && !transportStarted_ &&
      deadlineReached(now, timeSyncRetryAt_)) {
    startTimeSync(now);
    return;
  }

  if (transportStarted_) {
    webSocket_.loop();
  }

  if (state_ == State::Authenticating &&
      now - helloStartedAt_ >= APP_HELLO_TIMEOUT_MS) {
    lastError_ = F("hello_timeout");
    Serial.println("WSS_HELLO_TIMEOUT");
    webSocket_.disconnect();
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
  Serial.println("TIME_SYNC_STARTED");
}

void CloudClient::probeTcpConnectivity() {
  // Plain-TCP probe of host:port, run right before the TLS/WebSocket attempt
  // so the Serial trace shows WHICH layer is failing:
  //   TCP_PROBE_OK     -> plain TCP through the gateway works; the WSS
  //                       failure is at the TLS or WebSocket layer.
  //   TCP_PROBE_FAILED -> DNS/routing failed below TLS. 0 = timeout; a
  //                       negative code is an lwIP/esp_wifi error (host not
  //                       found, no route, ...) — read the printed code.
  // connect() blocks, so this runs on the same call path that would block
  // the TLS attempt anyway; on a working link it returns in milliseconds.
  WiFiClient probeClient;
  const int result = probeClient.connect(config_.serverHost.c_str(),
                                         config_.serverPort);
  if (result == 1) {
    Serial.printf("TCP_PROBE_OK host=%s port=%u\r\n",
                  config_.serverHost.c_str(), config_.serverPort);
    probeClient.stop();
  } else {
    Serial.printf("TCP_PROBE_FAILED host=%s port=%u code=%d\r\n",
                  config_.serverHost.c_str(), config_.serverPort, result);
  }
}

void CloudClient::probeTlsConnectivity() {
  // Full TLS handshake against the SAME host/port/CA the WebSocket transport
  // will use, with a generous timeout so a slow gateway can still finish it.
  // Discriminates the two remaining WSS failure modes:
  //   code=1  -> handshake completed; elapsed_ms shows how slow it is.
  //              If it exceeds the WebSockets library's hard 5000ms
  //              WEBSOCKETS_TCP_TIMEOUT budget, that is the whole bug.
  //   code=0  -> stalled until the probe timeout (no TLS data made it back).
  //   code<0  -> actively rejected (CA/cipher/cert validation failure).
  WiFiClientSecure probeSsl;
  probeSsl.setCACert(SERVER_ROOT_CA);
  const uint32_t startedAt = millis();
  const int result = probeSsl.connect(config_.serverHost.c_str(),
                                      config_.serverPort,
                                      TLS_PROBE_TIMEOUT_MS);
  const uint32_t elapsedMs = millis() - startedAt;
  if (result == 1) {
    Serial.printf("TLS_PROBE_OK elapsed_ms=%lu\r\n",
                  static_cast<unsigned long>(elapsedMs));
    probeSsl.stop();
  } else {
    Serial.printf("TLS_PROBE_FAILED code=%d elapsed_ms=%lu\r\n", result,
                  static_cast<unsigned long>(elapsedMs));
  }
}

void CloudClient::startTransport() {
  state_ = State::Connecting;
  probeTcpConnectivity();
  probeTlsConnectivity();
  transportStarted_ = true;
  webSocket_.onEvent([this](WStype_t type, uint8_t *payload, size_t length) {
    handleEvent(type, payload, length);
  });
  webSocket_.setExtraHeaders(authorizationHeader_.c_str());
  webSocket_.setReconnectInterval(nextReconnectDelay());
  webSocket_.enableHeartbeat(WEBSOCKET_PING_INTERVAL_MS,
                             WEBSOCKET_PONG_TIMEOUT_MS,
                             WEBSOCKET_MISSED_PONG_LIMIT);
  webSocket_.beginSslWithCA(config_.serverHost.c_str(), config_.serverPort,
                            socketPath_.c_str(), SERVER_ROOT_CA);
  Serial.printf("WSS_CONNECTING host=%s port=%u path=%s\r\n",
                config_.serverHost.c_str(), config_.serverPort,
                socketPath_.c_str());
}

void CloudClient::handleEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: {
      if (!configured_) {
        return;
      }
      state_ = State::RetryWait;
      const uint32_t delayMs = nextReconnectDelay();
      webSocket_.setReconnectInterval(delayMs);
      lastError_ = F("websocket_disconnected");
      // The WebSockets library sometimes passes a close reason (e.g. "code
      // 1006"); log it when present so the Serial trace shows WHY the
      // connection dropped.
      if (length > 0) {
        Serial.printf("WSS_DISCONNECTED reason=%.*s retry_ms=%lu\r\n",
                      static_cast<int>(length),
                      reinterpret_cast<const char *>(payload),
                      static_cast<unsigned long>(delayMs));
      } else {
        Serial.printf("WSS_DISCONNECTED retry_ms=%lu\r\n",
                      static_cast<unsigned long>(delayMs));
      }
      break;
    }
    case WStype_CONNECTED:
      state_ = State::Authenticating;
      helloStartedAt_ = millis();
      lastError_ = "";
      Serial.println("WSS_UPGRADED");
      sendHello();
      break;
    case WStype_TEXT:
      handleText(payload, length);
      break;
    case WStype_ERROR: {
      // The WebSockets library passes the human-readable failure text here
      // (e.g. "SSL handshake failed", "connect failed"). It is the single
      // most useful fact when a connection attempt dies before WSS_UPGRADED.
      lastError_ = F("websocket_error");
      if (length > 0) {
        Serial.printf("WSS_ERROR reason=%.*s\r\n",
                      static_cast<int>(length),
                      reinterpret_cast<const char *>(payload));
      } else {
        Serial.println("WSS_ERROR reason=unknown");
      }
      break;
    }
    default:
      break;
  }
}

void CloudClient::handleText(uint8_t *payload, size_t length) {
  if (length == 0 || length > MAX_WEBSOCKET_MESSAGE_BYTES) {
    lastError_ = F("invalid_message_size");
    Serial.println("WSS_PROTOCOL_ERROR reason=message_size");
    webSocket_.disconnect();
    return;
  }

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, payload, length);
  if (error || !document.is<JsonObject>()) {
    lastError_ = F("invalid_json");
    Serial.println("WSS_PROTOCOL_ERROR reason=invalid_json");
    webSocket_.disconnect();
    return;
  }

  const JsonObjectConst object = document.as<JsonObjectConst>();
  const char *type = object["type"] | "";
  if (state_ == State::Authenticating && strcmp(type, "ready") == 0) {
    const char *readyDeviceId = object["device_id"] | "";
    if (object.size() != 3 || (object["v"] | 0) != 1 ||
        deviceId_ != readyDeviceId) {
      lastError_ = F("invalid_ready");
      Serial.println("WSS_PROTOCOL_ERROR reason=invalid_ready");
      webSocket_.disconnect();
      return;
    }
    state_ = State::Online;
    reconnectAttempt_ = 0;
    lastError_ = "";
    Serial.println("WSS_AUTHENTICATED");
    return;
  }

  if (state_ != State::Online || strcmp(type, "set_state") != 0 ||
      object.size() != 5 || (object["v"] | 0) != 1 ||
      !object["on"].is<bool>()) {
    lastError_ = F("invalid_command");
    Serial.println("WSS_PROTOCOL_ERROR reason=invalid_command");
    webSocket_.disconnect();
    return;
  }

  const char *commandDeviceId = object["device_id"] | "";
  const String commandId = object["command_id"] | "";
  if (deviceId_ != commandDeviceId || !validCommandId(commandId) ||
      !applyStateHandler_) {
    lastError_ = F("invalid_command_fields");
    Serial.println("WSS_PROTOCOL_ERROR reason=invalid_command_fields");
    webSocket_.disconnect();
    return;
  }

  const bool requestedOn = object["on"].as<bool>();
  Serial.printf("COMMAND_RECEIVED command_id=%s on=%s\r\n",
                commandId.c_str(), requestedOn ? "true" : "false");
  reportedOn_ = applyStateHandler_(requestedOn);
  Serial.printf("DEVICE_STATE_APPLIED command_id=%s on=%s\r\n",
                commandId.c_str(), reportedOn_ ? "true" : "false");
  sendStateReport(commandId, reportedOn_);
}

void CloudClient::sendHello() {
  JsonDocument document;
  document["v"] = 1;
  document["type"] = "hello";
  document["device_id"] = deviceId_;
  document["firmware"] = "poc5-0.1.0";
  document["reported"]["on"] = reportedOn_;
  String message;
  serializeJson(document, message);
  webSocket_.sendTXT(message);
  Serial.println("WSS_HELLO_SENT");
}

void CloudClient::sendStateReport(const String &commandId, bool on) {
  JsonDocument document;
  document["v"] = 1;
  document["type"] = "state_report";
  document["command_id"] = commandId;
  document["device_id"] = deviceId_;
  document["on"] = on;
  String message;
  serializeJson(document, message);
  if (webSocket_.sendTXT(message)) {
    Serial.printf("COMMAND_ACK_SENT command_id=%s on=%s\r\n",
                  commandId.c_str(), on ? "true" : "false");
  } else {
    Serial.printf("COMMAND_ACK_FAILED command_id=%s\r\n", commandId.c_str());
  }
}

uint32_t CloudClient::nextReconnectDelay() {
  static constexpr uint32_t delays[] = {1000, 2000, 4000, 8000,
                                         16000, 30000, 60000};
  const size_t index = min(static_cast<size_t>(reconnectAttempt_),
                           sizeof(delays) / sizeof(delays[0]) - 1);
  if (reconnectAttempt_ < 255) {
    ++reconnectAttempt_;
  }
  return delays[index] + static_cast<uint32_t>(esp_random() % 501);
}

bool CloudClient::validCommandId(const String &value) const {
  if (value.length() != 36) {
    return false;
  }
  for (size_t index = 0; index < value.length(); ++index) {
    const char character = value[index];
    if (index == 8 || index == 13 || index == 18 || index == 23) {
      if (character != '-') {
        return false;
      }
    } else if (!isxdigit(static_cast<unsigned char>(character))) {
      return false;
    }
  }
  return true;
}
