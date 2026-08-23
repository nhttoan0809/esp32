#pragma once

#include <Arduino.h>
#include <WebSocketsClient.h>

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

  using ApplyStateHandler = std::function<bool(bool)>;

  void setApplyStateHandler(ApplyStateHandler handler);
  void setReportedState(bool on);
  void begin(const DeviceConfig &config, const char *deviceId,
             const char *deviceToken);
  void stop();
  void loop(bool wifiConnected);
  bool online() const;
  State state() const;
  const char *stateName() const;
  const String &lastError() const;

 private:
  void startTimeSync(uint32_t now);
  void startTransport();
  // Non-blocking plain-TCP pre-flight probe of host:port. It exists to tell
  // the Serial trace WHICH layer a failed connection attempt died in:
  //   TCP_PROBE_FAILED -> DNS or gateway routing problem (below TLS)
  //   TCP_PROBE_OK     -> plain TCP works; the failure is at TLS/WebSocket
  void probeTcpConnectivity();
  // Full TLS handshake probe using the SAME CA as the WebSocket transport,
  // with timing:
  //   TLS_PROBE_OK            -> handshake completes; elapsed_ms shows how
  //                              slow the gateway is (the WebSockets library
  //                              gives its connect() a hard 5000ms budget, so
  //                              a slow-but-working handshake still fails).
  //   code=0, elapsed ~5000   -> handshake stalled/timeout (gateway latency).
  //   code<0                  -> handshake rejected (cert/CA/cipher issue).
  void probeTlsConnectivity();
  void handleEvent(WStype_t type, uint8_t *payload, size_t length);
  void handleText(uint8_t *payload, size_t length);
  void sendHello();
  void sendStateReport(const String &commandId, bool on);
  uint32_t nextReconnectDelay();
  bool validCommandId(const String &value) const;

  WebSocketsClient webSocket_;
  ApplyStateHandler applyStateHandler_;
  DeviceConfig config_;
  String deviceId_;
  String deviceToken_;
  String socketPath_;
  String authorizationHeader_;
  String lastError_;
  State state_ = State::Disabled;
  bool configured_ = false;
  bool transportStarted_ = false;
  bool reportedOn_ = false;
  bool wifiWasConnected_ = false;
  uint8_t reconnectAttempt_ = 0;
  uint32_t timeSyncStartedAt_ = 0;
  uint32_t timeSyncRetryAt_ = 0;
  uint32_t helloStartedAt_ = 0;
};
