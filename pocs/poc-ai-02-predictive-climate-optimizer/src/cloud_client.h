#pragma once

#include <Arduino.h>
#include <ArduinoWebsockets.h>
#include "config.h"
#include "sensors_manager.h"
#include "display_manager.h"

class CloudClient {
public:
  CloudClient();
  bool begin(const char* ssid, const char* pass, const char* host, uint16_t port, const char* path);
  void loop(); // Goi lien tuc trong loop() de duy tri ket noi va nhan tin nhan
  
  bool sendTelemetry(const SensorData& sensor, bool fan_state, bool heat_state);
  bool isConnected() const { return _wsConnected; }
  bool isWiFiConnected() const;
  
  const AIDecisionData& getLatestAIDecision() const { return _aiDecision; }
  unsigned long getLastResponseTimeMs() const { return _lastResponseMs; }
  bool hasNewCommand() const { return _hasNewCommand; }
  void clearNewCommandFlag() { _hasNewCommand = false; }

private:
  websockets::WebsocketsClient _client;
  AIDecisionData _aiDecision;
  
  const char* _host = DEFAULT_WS_HOST;
  uint16_t _port = DEFAULT_WS_PORT;
  const char* _path = DEFAULT_WS_PATH;
  
  bool _wsConnected = false;
  bool _hasNewCommand = false;
  unsigned long _lastResponseMs = 0;
  unsigned long _lastReconnectAttemptMs = 0;
  
  void onMessageCallback(websockets::WebsocketsMessage message);
  void onEventCallback(websockets::WebsocketsEvent event, String data);
  void connectWebSocket();
};
