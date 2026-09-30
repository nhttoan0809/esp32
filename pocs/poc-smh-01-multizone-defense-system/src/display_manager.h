#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "types.h"

class DisplayManager {
public:
  bool begin();
  void update(SystemState state,
              const String &currentTime,
              bool z1, bool z2, bool z3,
              uint8_t remainingSeconds,
              SecurityZone breachedZone);

private:
  Adafruit_SSD1306 _display{OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &Wire, OLED_RESET_PIN};
  bool _displayAvailable = false;
  unsigned long _lastRenderTime = 0;
  bool _blinkState = false;

  void drawDashboard(const String &currentTime, bool z1, bool z2, bool z3);
  void drawCountdown(SystemState state, uint8_t remainingSec);
  void drawAlarm(SecurityZone breachedZone);
};
