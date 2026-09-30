#pragma once
#include <Arduino.h>
#include "config.h"
#include "types.h"

class SensorManager {
public:
  void begin();
  void update();

  bool isZone1Active() const;
  bool isZone2Active() const;
  bool isZone3Active() const;

  bool isArmButtonPressed();
  SecurityZone checkIntrusion();
  void injectTrigger(SecurityZone zone);

private:
  bool _z1Active = false;
  bool _z2Active = false;
  bool _z3Active = false;

  SecurityZone _injectedZone = ZONE_NONE;

  // Debounce nút Arm
  int _lastBtnState = HIGH;
  int _btnState = HIGH;
  unsigned long _lastDebounceTime = 0;
  bool _buttonPressedEvent = false;

  // Lọc rung cảm biến
  unsigned long _lastZ1Time = 0;
  unsigned long _lastZ2Time = 0;
  unsigned long _lastZ3Time = 0;
};
