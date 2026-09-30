#include "display_manager.h"

bool DisplayManager::begin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  if (_display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    _displayAvailable = true;
    _display.clearDisplay();
    _display.setTextColor(SSD1306_WHITE);
    _display.setTextSize(1);
    _display.setCursor(16, 20);
    _display.println(F("SMH-01 DEFENSE"));
    _display.setCursor(20, 36);
    _display.println(F("SYSTEM STARTING"));
    _display.display();
    delay(200);
    Serial.println("[OLED] Khoi tao SSD1306 thanh cong!");
  } else {
    _displayAvailable = false;
    Serial.println("[OLED] CANH BAO: Khong tim thay man hinh OLED SSD1306!");
  }
  return _displayAvailable;
}

void DisplayManager::update(SystemState state,
                            const String &currentTime,
                            bool z1, bool z2, bool z3,
                            uint8_t remainingSeconds,
                            SecurityZone breachedZone) {
  if (!_displayAvailable) return;

  unsigned long now = millis();
  if (now - _lastRenderTime < 180) return;
  _lastRenderTime = now;
  _blinkState = !_blinkState;

  _display.clearDisplay();

  switch (state) {
    case STATE_INIT:
    case STATE_DISARMED:
    case STATE_ARMED:
      drawDashboard(currentTime, z1, z2, z3);
      break;

    case STATE_EXIT_DELAY:
    case STATE_ENTRY_DELAY:
      drawCountdown(state, remainingSeconds);
      break;

    case STATE_ALARM:
      drawAlarm(breachedZone);
      break;
  }

  _display.display();
}

void DisplayManager::drawDashboard(const String &currentTime, bool z1, bool z2, bool z3) {
  // Thanh tiêu đề
  _display.setTextSize(1);
  _display.setCursor(0, 0);
  _display.print(F("SMH-01"));

  // Giờ hiển thị bên phải
  String timeOnly = currentTime.length() >= 8 ? currentTime.substring(currentTime.length() - 8) : currentTime;
  _display.setCursor(76, 0);
  _display.print(timeOnly);

  _display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  // Trạng thái hệ thống
  _display.setCursor(0, 14);
  _display.print(F("STATUS: "));
  _display.setCursor(48, 14);
  _display.println(F("ACTIVE"));

  // Hiển thị 3 Vùng
  _display.setCursor(0, 26);
  _display.print(F("Z1 (PIR) : "));
  _display.println(z1 ? F("MOTION!") : F("CLEAR"));

  _display.setCursor(0, 38);
  _display.print(F("Z2 (IR)  : "));
  _display.println(z2 ? F("BREACH!") : F("CLEAR"));

  _display.setCursor(0, 50);
  _display.print(F("Z3 (SND) : "));
  _display.println(z3 ? F("NOISE! ") : F("CLEAR"));
}

void DisplayManager::drawCountdown(SystemState state, uint8_t remainingSec) {
  _display.setTextSize(1);
  _display.setCursor(10, 2);
  if (state == STATE_EXIT_DELAY) {
    _display.print(F(">> EXIT DELAY <<"));
  } else {
    _display.print(F(">> ENTRY DELAY <<"));
  }
  _display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

  // Số đếm ngược to bản
  _display.setTextSize(3);
  _display.setCursor(42, 20);
  if (remainingSec < 10) _display.print("0");
  _display.print(remainingSec);
  _display.print("s");

  // Dòng nhắc nhở phía dưới
  _display.setTextSize(1);
  _display.setCursor(6, 52);
  _display.print(F("Swipe Card or PIN"));
}

void DisplayManager::drawAlarm(SecurityZone breachedZone) {
  // Cảnh báo chớp nháy viền
  if (_blinkState) {
    _display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
    _display.drawRect(2, 2, 124, 60, SSD1306_WHITE);
  }

  _display.setTextSize(2);
  _display.setCursor(18, 8);
  _display.println(F("!! ALARM !!"));

  _display.setTextSize(1);
  _display.setCursor(8, 30);
  _display.print(F("VI PHAM: "));
  _display.println(getZoneName(breachedZone));

  _display.setCursor(12, 46);
  _display.println(F("SIREN & LIGHT: ON"));
}
