#pragma once

#include <Arduino.h>
#include <ArduinoWebsockets.h>
#include <functional>

#include "device_config.h"

class CloudClient {
 public:
  enum class State {
    Disabled,
    WaitingForWifi,
    TimeSync,
    Connecting,
    Authenticating,
    Online,
    RetryWait,
  };

  void begin(const DeviceConfig &config, const char *deviceId,
             const char *deviceToken);
  void stop();
  void loop(bool wifiConnected);
  void reportSensorData(float temperature, float humidity);
  bool online() const;
  State state() const;
  const char *stateName() const;
  const String &lastError() const;

 private:
  void startTimeSync(uint32_t now);
  void startTransport();
  void handleMessage(const String &payload);
  void handleEvent(websockets::WebsocketsEvent event, const String &data);
  void sendHello();
  uint32_t nextReconnectDelay();

  websockets::WebsocketsClient webSocket_;
  DeviceConfig config_;
  String deviceId_;
  String deviceToken_;
  String socketPath_;
  String wsUrl_;
  String lastError_;

  State state_ = State::Disabled;
  bool configured_ = false;
  bool wifiWasConnected_ = false;
  uint8_t reconnectAttempt_ = 0;
  uint32_t timeSyncStartedAt_ = 0;
  uint32_t timeSyncRetryAt_ = 0;
  uint32_t reconnectAt_ = 0;
  uint32_t helloStartedAt_ = 0;
  uint32_t lastPingSentAt_ = 0;
  float lastReportedTemp_ = -999.0f;
  float lastReportedHumid_ = -999.0f;
};
