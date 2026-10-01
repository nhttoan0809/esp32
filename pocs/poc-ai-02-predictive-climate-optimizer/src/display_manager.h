#pragma once

#include <Arduino.h>
#include "config.h"
#include "sensors_manager.h"

struct AIDecisionData {
  float predicted_temp_15m = 0.0f;
  float predicted_temp_30m = 0.0f;
  char trend[16] = "STABLE";
  char comfort_status[16] = "COMFORT";
  char optimization_mode[24] = "NORMAL";
  bool relay1_fan = false;
  bool relay2_heat = false;
  bool is_ai_online = false;
};

class DisplayManager {
public:
  DisplayManager();
  bool begin();
  void togglePage();
  void update(const SensorData& sensor, const AIDecisionData& ai, bool relay1_state, bool relay2_state);

private:
  uint8_t _currentPage = 0; // 0: Overview, 1: AI Prediction Details
  unsigned long _lastRefreshMs = 0;
  void renderPageOverview(const SensorData& sensor, const AIDecisionData& ai, bool r1, bool r2);
  void renderPagePrediction(const SensorData& sensor, const AIDecisionData& ai, bool r1, bool r2);
};
