#pragma once

#include <Arduino.h>
#include "config.h"
#include "sensors_manager.h"
#include "relay_controller.h"
#include "display_manager.h"

class FailsafeController {
public:
  FailsafeController();
  
  // Kiem tra timeout va chay bo dieu khien cuc bo neu can
  void evaluate(const SensorData& sensor,
                AIDecisionData& aiData,
                RelayController& relay,
                unsigned long lastAiResponseMs,
                bool isWsConnected);

private:
  bool _fallbackActive = false;
  unsigned long _lastLogMs = 0;
};
