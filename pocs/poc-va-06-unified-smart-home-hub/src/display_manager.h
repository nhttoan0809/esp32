#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "app_config.h"

struct SystemState {
  float temperature = NAN;
  float humidity = NAN;
  bool lampOn = false;
  uint8_t ledBrightness = 0;   // 0 - 100%
  bool ldrDark = false;
  bool motionDetected = false;
  bool guardMode = true;
  bool alarmActive = false;
  uint16_t motionCount = 0;
  bool wifiConnected = false;
  bool cloudOnline = false;
  String ipAddress = "0.0.0.0";
};

class DisplayManager {
 public:
  enum Page {
    PageClimate = 0,
    PageLighting = 1,
    PageSecurity = 2,
    PageSystem = 3,
    PageCount = 4
  };

  DisplayManager();

  bool begin();
  void nextPage();
  void showBootScreen(const char *statusText);
  void update(const SystemState &state);

 private:
  Adafruit_SSD1306 display_;
  bool initialized_ = false;
  Page currentPage_ = PageClimate;
  uint32_t lastUpdate_ = 0;

  void drawHeader(const char *title, bool wifi, bool cloud);
  void renderClimatePage(const SystemState &state);
  void renderLightingPage(const SystemState &state);
  void renderSecurityPage(const SystemState &state);
  void renderSystemPage(const SystemState &state);
};
