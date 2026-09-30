#include "display_manager.h"
#include "config.h"
#include <Wire.h>

DisplayManager displayManager;

DisplayManager::DisplayManager()
    : display(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &Wire, -1),
      displayFound(false),
      lastRefreshTime(0) {}

void DisplayManager::begin() {
    if (display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
        displayFound = true;
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(10, 20);
        display.println(F("SMART LOCK"));
        display.setCursor(10, 35);
        display.println(F("Initializing..."));
        display.display();
    } else {
        displayFound = false;
        Serial.println(F("[OLED] Warning: SSD1306 not detected on I2C bus!"));
    }
}

void DisplayManager::drawHeader(const String& timeStr, bool wifiConnected) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(timeStr.length() > 0 ? timeStr : F("00:00:00"));

    display.setCursor(85, 0);
    if (wifiConnected) {
        display.print(F("[WiFi]"));
    } else {
        display.print(F("[----]"));
    }
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
}

void DisplayManager::showHome(const String& timeStr, const String& dateStr, LockState state, bool wifiConnected) {
    if (!displayFound) return;

    display.clearDisplay();
    drawHeader(timeStr, wifiConnected);

    display.setTextSize(1);
    display.setCursor(15, 16);
    display.print(F("SMH-02 SMART LOCK"));

    display.setCursor(20, 28);
    display.print(F("Status: LOCKED"));

    display.drawRect(8, 40, 112, 22, SSD1306_WHITE);
    display.setCursor(12, 47);
    display.print(F("TAP RFID / KEY PIN"));

    display.display();
}

void DisplayManager::showPinInput(const String& maskedPin, int length) {
    if (!displayFound) return;

    display.clearDisplay();
    drawHeader("PIN ENTRY", true);

    display.setTextSize(1);
    display.setCursor(20, 16);
    display.print(F("Enter 4-Digit PIN:"));

    // Draw PIN box
    display.drawRect(24, 30, 80, 18, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(35, 32);
    display.print(maskedPin);

    display.setTextSize(1);
    display.setCursor(10, 53);
    display.print(F("(*)Clear   (#)Confirm"));

    display.display();
}

void DisplayManager::showAccessGranted(const String& methodStr, int remainingSec) {
    if (!displayFound) return;

    display.clearDisplay();
    drawHeader("UNLOCKED", true);

    display.setTextSize(1);
    display.setCursor(15, 14);
    display.print(F(">> ACCESS GRANTED <<"));

    display.setCursor(10, 26);
    display.print(F("By: "));
    display.print(methodStr);

    display.setCursor(10, 38);
    display.print(F("Auto-lock in: "));
    display.print(remainingSec);
    display.print(F("s"));

    // Visual progress bar for countdown (5s total)
    int barWidth = map(remainingSec, 0, 5, 0, 108);
    if (barWidth < 0) barWidth = 0;
    if (barWidth > 108) barWidth = 108;
    display.drawRect(10, 50, 108, 8, SSD1306_WHITE);
    display.fillRect(10, 50, barWidth, 8, SSD1306_WHITE);

    display.display();
}

void DisplayManager::showAccessDenied(const String& reasonStr, int failedAttempts) {
    if (!displayFound) return;

    display.clearDisplay();
    drawHeader("DENIED", false);

    display.setTextSize(1);
    display.setCursor(12, 16);
    display.print(F("!! ACCESS DENIED !!"));

    display.setCursor(10, 30);
    display.print(reasonStr);

    display.setCursor(10, 45);
    display.print(F("Failed: "));
    display.print(failedAttempts);
    display.print(F(" / "));
    display.print(MAX_FAILED_ATTEMPTS);

    display.display();
}

void DisplayManager::showLockout(int remainingSec) {
    if (!displayFound) return;

    display.clearDisplay();
    drawHeader("LOCKOUT", false);

    display.setTextSize(1);
    display.setCursor(10, 16);
    display.print(F("! SYSTEM LOCKOUT !"));

    display.setCursor(10, 30);
    display.print(F("Too many failed tries"));

    display.setCursor(10, 45);
    display.print(F("Penalty: "));
    display.print(remainingSec);
    display.print(F("s left"));

    display.display();
}

void DisplayManager::showMessage(const String& line1, const String& line2, const String& line3) {
    if (!displayFound) return;

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 10);
    display.println(line1);
    if (line2.length() > 0) {
        display.setCursor(0, 26);
        display.println(line2);
    }
    if (line3.length() > 0) {
        display.setCursor(0, 42);
        display.println(line3);
    }
    display.display();
}

void DisplayManager::update() {
    // Throttled refresh hook if needed
}
