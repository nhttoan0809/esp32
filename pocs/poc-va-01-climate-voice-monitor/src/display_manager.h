#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "app_config.h"

class DisplayManager {
 public:
  enum class ComfortState {
    Unknown,
    Comfortable,
    AlertHot,
    AlertHumid,
    AlertCold
  };

  DisplayManager();

  bool begin();
  void update(float temperature, float humidity, bool wifiConnected, bool cloudOnline);
  void showBootScreen(const char *statusText);
  void showError(const char *errorMessage);

 private:
  Adafruit_SSD1306 display_;
  bool initialized_ = false;
  uint32_t lastUpdate_ = 0;

  ComfortState evaluateComfort(float temperature, float humidity) const;
};
