#pragma once

#include <Arduino.h>
#include "config.h"

class RelayController {
public:
  RelayController();
  bool begin();
  
  bool setFan(bool on, bool force = false);
  bool setHeat(bool on, bool force = false);
  
  bool isFanOn() const { return _fanState; }
  bool isHeatOn() const { return _heatState; }
  
  void updateLEDs(bool isComfort, bool isWarning);

private:
  bool _fanState = false;
  bool _heatState = false;
  unsigned long _lastFanChangeMs = 0;
  unsigned long _lastHeatChangeMs = 0;
};
