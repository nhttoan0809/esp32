#include "relay_controller.h"

RelayController::RelayController() {}

bool RelayController::begin() {
  Serial.println("[AI-02] Dang khoi tao module Relay 2 kenh (Active LOW)...");
  
  // 1. Cau hinh chan Relay: Ban dau dat HIGH (NGAT RELAY) truoc khi pinMode de tranh xung giat luc boot
  digitalWrite(PIN_RELAY1_FAN, RELAY_INACTIVE_LEVEL);
  digitalWrite(PIN_RELAY2_HEAT, RELAY_INACTIVE_LEVEL);
  pinMode(PIN_RELAY1_FAN, OUTPUT);
  pinMode(PIN_RELAY2_HEAT, OUTPUT);
  
  // 2. Cau hinh LED chi thi
  pinMode(PIN_LED_COMFORT, OUTPUT);
  pinMode(PIN_LED_WARN, OUTPUT);
  digitalWrite(PIN_LED_COMFORT, LOW);
  digitalWrite(PIN_LED_WARN, LOW);
  
  _fanState = false;
  _heatState = false;
  _lastFanChangeMs = millis();
  _lastHeatChangeMs = millis();
  
  Serial.println("[AI-02] RELAY_INIT_OK: Relay 2 kenh va LED chi thi da san sang!");
  return true;
}

bool RelayController::setFan(bool on, bool force) {
  if (_fanState == on) return true;
  
  unsigned long now = millis();
  if (!force && (now - _lastFanChangeMs < ANTI_RAPID_CYCLE_MS)) {
    Serial.printf("[AI-02] RELAY1 (FAN): Chan chong chap chon (con %lu ms)\n",
                  (ANTI_RAPID_CYCLE_MS - (now - _lastFanChangeMs)));
    return false;
  }
  
  _fanState = on;
  _lastFanChangeMs = now;
  digitalWrite(PIN_RELAY1_FAN, on ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL);
  Serial.printf("[AI-02] RELAY1 (FAN) -> %s\n", on ? "BAT (ON)" : "TAT (OFF)");
  return true;
}

bool RelayController::setHeat(bool on, bool force) {
  if (_heatState == on) return true;
  
  unsigned long now = millis();
  if (!force && (now - _lastHeatChangeMs < ANTI_RAPID_CYCLE_MS)) {
    Serial.printf("[AI-02] RELAY2 (HEAT): Chan chong chap chon (con %lu ms)\n",
                  (ANTI_RAPID_CYCLE_MS - (now - _lastHeatChangeMs)));
    return false;
  }
  
  _heatState = on;
  _lastHeatChangeMs = now;
  digitalWrite(PIN_RELAY2_HEAT, on ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL);
  Serial.printf("[AI-02] RELAY2 (HEAT) -> %s\n", on ? "BAT (ON)" : "TAT (OFF)");
  return true;
}

void RelayController::updateLEDs(bool isComfort, bool isWarning) {
  digitalWrite(PIN_LED_COMFORT, isComfort ? HIGH : LOW);
  digitalWrite(PIN_LED_WARN, isWarning ? HIGH : LOW);
}
