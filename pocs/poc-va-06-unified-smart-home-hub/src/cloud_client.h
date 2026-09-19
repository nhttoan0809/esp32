#pragma once

#include <Arduino.h>
#include <ArduinoWebsockets.h>
#include <functional>

#include "device_config.h"
#include "display_manager.h"

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

  using ApplyRelayHandler = std::function<bool(bool on)>;
  using ApplyBrightnessHandler = std::function<uint8_t(uint8_t percent)>;
  using ApplySecurityModeHandler = std::function<bool(bool guardMode)>;

  void setApplyRelayHandler(ApplyRelayHandler handler);
  void setApplyBrightnessHandler(ApplyBrightnessHandler handler);
  void setApplySecurityModeHandler(ApplySecurityModeHandler handler);

  void begin(const DeviceConfig &config, const char *deviceId,
             const char *deviceToken);
  void stop();
  void loop(bool wifiConnected);

  void reportTelemetry(const SystemState &state);
  void reportRelayState(bool on);
  void reportMotionEvent(bool detected);

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
  void sendStateReport(const String &commandId, bool on);
  void sendBrightnessReport(const String &commandId, uint8_t brightness);
  uint32_t nextReconnectDelay();

  websockets::WebsocketsClient webSocket_;
  ApplyRelayHandler applyRelayHandler_;
  ApplyBrightnessHandler applyBrightnessHandler_;
  ApplySecurityModeHandler applySecurityModeHandler_;

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
};
