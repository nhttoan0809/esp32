#include "cloud_client.h"
#include <WiFi.h>
#include <ArduinoJson.h>

CloudClient::CloudClient() {}

bool CloudClient::begin(const char* ssid, const char* pass, const char* host, uint16_t port, const char* path) {
  _host = host;
  _port = port;
  _path = path;
  
  Serial.printf("[AI-02] WIFI_CONNECTING... Dang ket noi toi SSID: %s\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  
  // Callback WebSocket
  _client.onMessage([this](websockets::WebsocketsMessage msg) {
    this->onMessageCallback(msg);
  });
  
  _client.onEvent([this](websockets::WebsocketsEvent ev, String data) {
    this->onEventCallback(ev, data);
  });
  
  return true;
}

bool CloudClient::isWiFiConnected() const {
  return (WiFi.status() == WL_CONNECTED);
}

void CloudClient::loop() {
  if (!isWiFiConnected()) {
    _wsConnected = false;
    _aiDecision.is_ai_online = false;
    return;
  }
  
  if (_wsConnected) {
    _client.poll();
  } else {
    unsigned long now = millis();
    if (now - _lastReconnectAttemptMs >= 5000) {
      _lastReconnectAttemptMs = now;
      connectWebSocket();
    }
  }
}

void CloudClient::connectWebSocket() {
  Serial.printf("[AI-02] Dang ket noi WebSocket den %s:%d%s...\n", _host, _port, _path);
  bool ok = _client.connect(_host, _port, _path);
  if (ok) {
    _wsConnected = true;
    _aiDecision.is_ai_online = true;
    _lastResponseMs = millis();
    Serial.println("[AI-02] WS_CONNECTED: Ket noi WebSocket voi AI Server thanh cong!");
  } else {
    _wsConnected = false;
    _aiDecision.is_ai_online = false;
    Serial.println("[AI-02] WS_DISCONNECTED: Ket noi WebSocket that bai, se thu lai...");
  }
}

void CloudClient::onEventCallback(websockets::WebsocketsEvent event, String data) {
  if (event == websockets::WebsocketsEvent::ConnectionOpened) {
    _wsConnected = true;
    _aiDecision.is_ai_online = true;
    _lastResponseMs = millis();
    Serial.println("[AI-02] WS_EVENT: ConnectionOpened");
  } else if (event == websockets::WebsocketsEvent::ConnectionClosed) {
    _wsConnected = false;
    _aiDecision.is_ai_online = false;
    Serial.println("[AI-02] WS_EVENT: ConnectionClosed");
  }
}

void CloudClient::onMessageCallback(websockets::WebsocketsMessage message) {
  if (!message.isText()) return;
  
  String payload = message.data();
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.printf("[AI-02] LOI JSON DESERIALIZE: %s\n", error.c_str());
    return;
  }
  
  const char* type = doc["type"] | "";
  if (strcmp(type, "climate_prediction") == 0) {
    _aiDecision.predicted_temp_15m = doc["predicted_temp_15m"] | 0.0f;
    _aiDecision.predicted_temp_30m = doc["predicted_temp_30m"] | 0.0f;
    
    const char* tr = doc["trend"] | "STABLE";
    strncpy(_aiDecision.trend, tr, sizeof(_aiDecision.trend) - 1);
    
    const char* cmf = doc["comfort_status"] | "COMFORT";
    strncpy(_aiDecision.comfort_status, cmf, sizeof(_aiDecision.comfort_status) - 1);
    
    const char* opt = doc["optimization_mode"] | "NORMAL";
    strncpy(_aiDecision.optimization_mode, opt, sizeof(_aiDecision.optimization_mode) - 1);
    
    _aiDecision.relay1_fan = doc["relay1_fan"] | false;
    _aiDecision.relay2_heat = doc["relay2_heat"] | false;
    _aiDecision.is_ai_online = true;
    
    _lastResponseMs = millis();
    _hasNewCommand = true;
    
    const char* reason = doc["reason"] | "";
    Serial.printf("[AI-02] AI_PREDICTION_RECEIVED: Pred(+30m)=%.1fC, Trend=%s, Fan=%d, Heat=%d, Reason: %s\n",
                  _aiDecision.predicted_temp_30m, _aiDecision.trend, _aiDecision.relay1_fan,
                  _aiDecision.relay2_heat, reason);
  }
}

bool CloudClient::sendTelemetry(const SensorData& s, bool fan_state, bool heat_state) {
  if (!_wsConnected) return false;
  
  JsonDocument doc;
  doc["device_id"] = DEFAULT_DEVICE_ID;
  doc["temperature"] = serialized(String(s.temperature, 2));
  doc["humidity"] = serialized(String(s.humidity, 1));
  doc["light_level"] = s.light_level;
  doc["light_analog"] = s.light_analog;
  doc["rtc_timestamp"] = s.rtc_timestamp;
  doc["rtc_time_str"] = s.rtc_time_str;
  doc["relay1_fan"] = fan_state;
  doc["relay2_heat"] = heat_state;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["uptime_sec"] = millis() / 1000;
  
  String out;
  serializeJson(doc, out);
  bool res = _client.send(out);
  if (res) {
    Serial.printf("[AI-02] TELEMETRY_SENT: T=%.1fC, H=%.1f%%, L=%d, RTC=%s\n",
                  s.temperature, s.humidity, s.light_level, s.rtc_time_str);
  }
  return res;
}
