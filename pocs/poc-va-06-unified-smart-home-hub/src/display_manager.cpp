#include "display_manager.h"
#include <Wire.h>

using namespace app_config;

DisplayManager::DisplayManager()
    : display_(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1) {}

bool DisplayManager::begin() {
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  if (!display_.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println(F("[OLED] Failed to initialize SSD1306"));
    initialized_ = false;
    return false;
  }

  initialized_ = true;
  display_.clearDisplay();
  display_.setTextColor(SSD1306_WHITE);
  showBootScreen("Starting Hub...");
  Serial.println(F("[OLED] SSD1306 OLED Ready (4 Pages Multi-View)"));
  return true;
}

void DisplayManager::nextPage() {
  currentPage_ = static_cast<Page>((currentPage_ + 1) % PageCount);
  Serial.printf("[OLED] Switched to Page: %d\r\n", static_cast<int>(currentPage_) + 1);
}

void DisplayManager::showBootScreen(const char *statusText) {
  if (!initialized_) return;

  display_.clearDisplay();
  display_.setTextSize(1);
  display_.setCursor(12, 6);
  display_.println(F("AI SMART HOME HUB"));
  display_.drawLine(0, 18, 127, 18, SSD1306_WHITE);

  display_.setCursor(16, 28);
  display_.println(F("POC-VA-06 Unified"));

  display_.setCursor(8, 46);
  display_.print(F("Status: "));
  display_.println(statusText);

  display_.display();
}

void DisplayManager::drawHeader(const char *title, bool wifi, bool cloud) {
  display_.setTextSize(1);
  display_.setCursor(0, 0);
  display_.print(title);

  display_.setCursor(80, 0);
  display_.print(wifi ? F("W:OK") : F("W:--"));

  display_.setCursor(106, 0);
  display_.print(cloud ? F("C:OK") : F("C:--"));

  display_.drawLine(0, 10, 127, 10, SSD1306_WHITE);
}

void DisplayManager::renderClimatePage(const SystemState &state) {
  drawHeader("[1/4] CLIMATE", state.wifiConnected, state.cloudOnline);

  if (isnan(state.temperature) || isnan(state.humidity)) {
    display_.setTextSize(1);
    display_.setCursor(16, 26);
    display_.println(F("Reading DHT11..."));
  } else {
    display_.setTextSize(2);
    display_.setCursor(2, 16);
    display_.printf("%.1f", state.temperature);
    display_.setTextSize(1);
    display_.setCursor(50, 16);
    display_.write(247);
    display_.print(F("C"));

    display_.setTextSize(2);
    display_.setCursor(68, 16);
    display_.printf("%.0f", state.humidity);
    display_.setTextSize(1);
    display_.setCursor(106, 16);
    display_.print(F("%"));

    display_.setTextSize(1);
    display_.setCursor(2, 34);
    display_.print(F("Temp"));
    display_.setCursor(68, 34);
    display_.print(F("Humidity"));
  }

  display_.drawLine(0, 46, 127, 46, SSD1306_WHITE);
  display_.setTextSize(1);
  display_.setCursor(4, 52);

  if (state.temperature >= TEMP_ALERT_HOT) {
    display_.print(F("! ALERT: HEAT HIGH !"));
  } else if (state.humidity >= HUMID_ALERT_HIGH) {
    display_.print(F("! ALERT: HIGH HUMID !"));
  } else if (state.temperature >= TEMP_COMFORT_MIN && state.temperature <= TEMP_COMFORT_MAX) {
    display_.print(F("* COMFORTABLE *"));
  } else {
    display_.print(F("Condition Normal"));
  }
}

void DisplayManager::renderLightingPage(const SystemState &state) {
  drawHeader("[2/4] LIGHTING", state.wifiConnected, state.cloudOnline);

  // Relay Lamp state
  display_.setTextSize(1);
  display_.setCursor(4, 16);
  display_.print(F("Lamp Relay: "));
  display_.print(state.lampOn ? F("[ON]") : F("[OFF]"));

  // Dimmer Brightness
  display_.setCursor(4, 28);
  display_.printf("Dimmer PWM: %3d%%", state.ledBrightness);

  // Small progress bar for brightness
  display_.drawRect(4, 40, 120, 7, SSD1306_WHITE);
  const int barWidth = (state.ledBrightness * 116) / 100;
  if (barWidth > 0) {
    display_.fillRect(6, 42, barWidth, 3, SSD1306_WHITE);
  }

  // LDR Status
  display_.drawLine(0, 50, 127, 50, SSD1306_WHITE);
  display_.setCursor(4, 54);
  display_.print(F("LDR Ambient: "));
  display_.print(state.ldrDark ? F("DARK (Night)") : F("BRIGHT (Day)"));
}

void DisplayManager::renderSecurityPage(const SystemState &state) {
  drawHeader("[3/4] SECURITY", state.wifiConnected, state.cloudOnline);

  // Guard Mode
  display_.setTextSize(1);
  display_.setCursor(4, 16);
  display_.print(F("Mode: "));
  display_.print(state.guardMode ? F("[GUARD ACTIVE]") : F("[ECO MODE]"));

  // PIR Motion State
  display_.setCursor(4, 28);
  display_.print(F("Motion: "));
  if (state.motionDetected) {
    display_.print(F(">> DETECTED! <<"));
  } else {
    display_.print(F("Clear"));
  }

  // Alarm & Count
  display_.setCursor(4, 40);
  display_.printf("Alarm: %s  Cnt: %u",
                 state.alarmActive ? "BUZZING!" : "Quiet",
                 state.motionCount);

  display_.drawLine(0, 50, 127, 50, SSD1306_WHITE);
  display_.setCursor(4, 54);
  display_.print(state.alarmActive ? F("!! INTRUSION ALERT !!") : F("Zone Protected"));
}

void DisplayManager::renderSystemPage(const SystemState &state) {
  drawHeader("[4/4] SYSTEM", state.wifiConnected, state.cloudOnline);

  display_.setTextSize(1);
  display_.setCursor(4, 16);
  display_.print(F("IP: "));
  display_.print(state.ipAddress);

  display_.setCursor(4, 28);
  display_.print(F("Cloud: "));
  display_.print(state.cloudOnline ? F("WSS Online") : F("Offline"));

  display_.setCursor(4, 40);
  display_.printf("Uptime: %lu s", millis() / 1000UL);

  display_.drawLine(0, 50, 127, 50, SSD1306_WHITE);
  display_.setCursor(4, 54);
  display_.print(F("Press Btn 4 for Next"));
}

void DisplayManager::update(const SystemState &state) {
  if (!initialized_) return;

  const uint32_t now = millis();
  if (now - lastUpdate_ < DISPLAY_REFRESH_MS) {
    return;
  }
  lastUpdate_ = now;

  display_.clearDisplay();

  switch (currentPage_) {
    case PageClimate:
      renderClimatePage(state);
      break;
    case PageLighting:
      renderLightingPage(state);
      break;
    case PageSecurity:
      renderSecurityPage(state);
      break;
    case PageSystem:
      renderSystemPage(state);
      break;
    default:
      renderClimatePage(state);
      break;
  }

  display_.display();
}
