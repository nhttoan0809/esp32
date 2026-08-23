#include "wifi_manager.h"

#include <WiFi.h>

#include "app_config.h"

using namespace app_config;

namespace {

bool deadlineReached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

}  // namespace

void WifiManager::begin() {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_AP_STA);
}

void WifiManager::connect(const DeviceConfig &config, bool finiteAttempts) {
  config_ = config;
  finiteAttempts_ = finiteAttempts;
  attempts_ = 0;
  lastError_ = "";
  WiFi.disconnect(false, false);
  startAttempt(millis());
}

void WifiManager::stop() {
  WiFi.disconnect(false, false);
  state_ = State::Idle;
  attempts_ = 0;
  clearDeviceConfig(config_);
}

void WifiManager::loop() {
  const uint32_t now = millis();
  const wl_status_t status = WiFi.status();

  if (state_ == State::Connected) {
    if (status != WL_CONNECTED) {
      Serial.printf("WIFI_DISCONNECTED status=%d\r\n", status);
      scheduleRetry(now, F("disconnected"));
    }
    return;
  }

  if (state_ == State::Connecting) {
    if (status == WL_CONNECTED &&
        WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
      state_ = State::Connected;
      lastError_ = "";
      Serial.printf("WIFI_CONNECTED ip=%s rssi=%d\r\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
      return;
    }
    if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
      scheduleRetry(now, F("connect_error"));
      return;
    }
    if (now - attemptStartedAt_ >= WIFI_CONNECT_TIMEOUT_MS) {
      scheduleRetry(now, F("connect_timeout"));
    }
    return;
  }

  if (state_ == State::RetryWait && deadlineReached(now, nextAttemptAt_)) {
    startAttempt(now);
  }
}

bool WifiManager::connected() const {
  return state_ == State::Connected && WiFi.status() == WL_CONNECTED;
}

bool WifiManager::failed() const {
  return state_ == State::Failed;
}

uint8_t WifiManager::attempts() const {
  return attempts_;
}

WifiManager::State WifiManager::state() const {
  return state_;
}

const String &WifiManager::lastError() const {
  return lastError_;
}

void WifiManager::startAttempt(uint32_t now) {
  ++attempts_;
  state_ = State::Connecting;
  attemptStartedAt_ = now;
  Serial.printf("WIFI_CONNECTING ssid=%s attempt=%u\r\n",
                config_.wifiSsid.c_str(), attempts_);
  const wl_status_t status =
      WiFi.begin(config_.wifiSsid.c_str(), config_.wifiPassword.c_str());
  if (status == WL_CONNECT_FAILED) {
    scheduleRetry(now, F("connect_start_failed"));
  }
}

void WifiManager::scheduleRetry(uint32_t now,
                                const __FlashStringHelper *reason) {
  lastError_ = String(reason);
  WiFi.disconnect(false, false);
  if (finiteAttempts_ && attempts_ >= 2) {
    state_ = State::Failed;
    Serial.printf("WIFI_FAILED reason=%s attempts=%u\r\n",
                  lastError_.c_str(), attempts_);
    return;
  }
  const uint32_t delayMs = retryDelayMs();
  nextAttemptAt_ = now + delayMs;
  state_ = State::RetryWait;
  Serial.printf("WIFI_RETRY_IN_MS %lu reason=%s\r\n",
                static_cast<unsigned long>(delayMs), lastError_.c_str());
}

uint32_t WifiManager::retryDelayMs() const {
  static constexpr uint32_t delays[] = {1000, 2000, 4000, 8000,
                                         16000, 30000, 60000};
  const size_t index =
      min(static_cast<size_t>(attempts_ == 0 ? 0 : attempts_ - 1),
          sizeof(delays) / sizeof(delays[0]) - 1);
  return delays[index] + static_cast<uint32_t>(esp_random() % 501);
}
