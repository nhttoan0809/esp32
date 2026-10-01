#include "failsafe.h"

FailsafeController::FailsafeController() {}

void FailsafeController::evaluate(const SensorData& sensor,
                                  AIDecisionData& aiData,
                                  RelayController& relay,
                                  unsigned long lastAiResponseMs,
                                  bool isWsConnected) {
  unsigned long now = millis();
  bool timeout = (lastAiResponseMs == 0) || (now - lastAiResponseMs > FAILSAFE_TIMEOUT_MS);
  
  if (!isWsConnected || timeout) {
    if (!_fallbackActive) {
      _fallbackActive = true;
      Serial.println("[AI-02] CANH BAO: Mat ket noi AI Server > 20s! Kich hoat che do tu hanh cuc bo (Local Fallback).");
    }
    
    aiData.is_ai_online = false;
    strncpy(aiData.optimization_mode, "LOCAL-FALLBACK", sizeof(aiData.optimization_mode) - 1);
    
    // Giai thuat dieu khien Hysteresis cuc bo
    float t = sensor.temperature;
    float h = sensor.humidity;
    
    // 1. Dieu khien Quat (Relay 1)
    if (t >= 30.0f) {
      relay.setFan(true);
    } else if (t <= 25.5f) {
      relay.setFan(false);
    }
    
    // 2. Dieu khien Suoi (Relay 2)
    if (t <= 18.0f) {
      relay.setHeat(true);
    } else if (t >= 22.0f) {
      relay.setHeat(false);
    }
    
    // 3. Cap nhat LED chi thi
    bool isComfort = (t >= 21.0f && t <= 28.0f && h >= 40.0f && h <= 70.0f);
    bool isAlert = (t >= 32.0f || h >= 75.0f);
    relay.updateLEDs(isComfort, isAlert);
    
    if (now - _lastLogMs >= 5000) {
      _lastLogMs = now;
      Serial.printf("[AI-02] LOCAL_FALLBACK: T=%.1fC, H=%.0f%% -> Fan=%s, Heat=%s\n",
                    t, h, relay.isFanOn() ? "ON" : "OFF", relay.isHeatOn() ? "ON" : "OFF");
    }
  } else {
    if (_fallbackActive) {
      _fallbackActive = false;
      Serial.println("[AI-02] AI Server da phuc hoi! Chuyen ve che do AI Predictive Optimization.");
    }
    aiData.is_ai_online = true;
  }
}
