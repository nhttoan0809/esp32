#include "display_manager.h"
#include <Wire.h>

using namespace app_config;

DisplayManager::DisplayManager()
    : display_(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1) {}

bool DisplayManager::begin() {
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  if (!display_.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println(F("[OLED] Failed to initialize SSD1306 OLED"));
    initialized_ = false;
    return false;
  }

  initialized_ = true;
  display_.clearDisplay();
  display_.setTextColor(SSD1306_WHITE);
  showBootScreen("Starting up...");
  Serial.println(F("[OLED] SSD1306 OLED Initialized successfully"));
  return true;
}

void DisplayManager::showBootScreen(const char *statusText) {
  if (!initialized_) return;

  display_.clearDisplay();
  display_.setTextSize(1);
  display_.setCursor(8, 8);
  display_.println(F("ESP32 SMART HOME"));
  display_.drawLine(0, 20, 127, 20, SSD1306_WHITE);

  display_.setCursor(12, 30);
  display_.setTextSize(1);
  display_.println(F("Climate Voice Monitor"));

  display_.setCursor(12, 48);
  display_.print(F("Status: "));
  display_.println(statusText);

  display_.display();
}

void DisplayManager::showError(const char *errorMessage) {
  if (!initialized_) return;

  display_.clearDisplay();
  display_.setTextSize(1);
  display_.setCursor(0, 0);
  display_.println(F("SYSTEM ERROR:"));
  display_.drawLine(0, 12, 127, 12, SSD1306_WHITE);

  display_.setCursor(0, 20);
  display_.println(errorMessage);
  display_.display();
}

DisplayManager::ComfortState DisplayManager::evaluateComfort(float temperature,
                                                             float humidity) const {
  if (isnan(temperature) || isnan(humidity)) {
    return ComfortState::Unknown;
  }
  if (temperature >= TEMP_ALERT_THRESHOLD) {
    return ComfortState::AlertHot;
  }
  if (humidity >= HUMID_ALERT_THRESHOLD) {
    return ComfortState::AlertHumid;
  }
  if (temperature < TEMP_COMFORT_MIN) {
    return ComfortState::AlertCold;
  }
  if (temperature <= TEMP_COMFORT_MAX && humidity >= HUMID_COMFORT_MIN &&
      humidity <= HUMID_COMFORT_MAX) {
    return ComfortState::Comfortable;
  }
  return ComfortState::Unknown;
}

void DisplayManager::update(float temperature, float humidity, bool wifiConnected,
                            bool cloudOnline) {
  if (!initialized_) return;

  const uint32_t now = millis();
  if (now - lastUpdate_ < DISPLAY_UPDATE_INTERVAL_MS) {
    return;
  }
  lastUpdate_ = now;

  display_.clearDisplay();

  // Top Title Bar
  display_.setTextSize(1);
  display_.setCursor(0, 0);
  display_.print(F("CLIMATE"));

  // Network indicator icons on top-right
  display_.setCursor(76, 0);
  display_.print(wifiConnected ? F("W:OK") : F("W:--"));

  display_.setCursor(104, 0);
  display_.print(cloudOnline ? F("C:OK") : F("C:--"));

  display_.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Center Values: Temperature & Humidity
  if (isnan(temperature) || isnan(humidity)) {
    display_.setTextSize(1);
    display_.setCursor(16, 26);
    display_.println(F("Reading sensor..."));
  } else {
    // Temperature Row
    display_.setTextSize(2);
    display_.setCursor(4, 16);
    display_.printf("%.1f", temperature);
    display_.setTextSize(1);
    display_.setCursor(54, 16);
    display_.write(247);  // degree symbol
    display_.print(F("C"));

    // Humidity Row
    display_.setTextSize(2);
    display_.setCursor(70, 16);
    display_.printf("%.0f", humidity);
    display_.setTextSize(1);
    display_.setCursor(110, 16);
    display_.print(F("%"));

    display_.setTextSize(1);
    display_.setCursor(4, 34);
    display_.print(F("Temp"));
    display_.setCursor(70, 34);
    display_.print(F("Humidity"));
  }

  // Bottom Status Bar
  display_.drawLine(0, 46, 127, 46, SSD1306_WHITE);
  display_.setTextSize(1);
  display_.setCursor(4, 52);

  const ComfortState state = evaluateComfort(temperature, humidity);
  switch (state) {
    case ComfortState::Comfortable:
      display_.print(F("* COMFORTABLE *"));
      break;
    case ComfortState::AlertHot:
      display_.print(F("! ALERT: TOO HOT !"));
      break;
    case ComfortState::AlertHumid:
      display_.print(F("! ALERT: HIGH HUMID !"));
      break;
    case ComfortState::AlertCold:
      display_.print(F("! NOTICE: CHILLY !"));
      break;
    case ComfortState::Unknown:
    default:
      display_.print(F("System Active"));
      break;
  }

  display_.display();
}
