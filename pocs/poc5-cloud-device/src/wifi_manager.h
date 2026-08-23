#pragma once

#include <Arduino.h>

#include "device_config.h"

class WifiManager {
 public:
  enum class State { Idle, Connecting, Connected, RetryWait, Failed };

  void begin();
  void connect(const DeviceConfig &config, bool finiteAttempts);
  void stop();
  void loop();
  bool connected() const;
  bool failed() const;
  uint8_t attempts() const;
  State state() const;
  const String &lastError() const;

 private:
  void startAttempt(uint32_t now);
  void scheduleRetry(uint32_t now, const __FlashStringHelper *reason);
  uint32_t retryDelayMs() const;

  DeviceConfig config_;
  State state_ = State::Idle;
  bool finiteAttempts_ = false;
  uint8_t attempts_ = 0;
  uint32_t attemptStartedAt_ = 0;
  uint32_t nextAttemptAt_ = 0;
  String lastError_;
};
