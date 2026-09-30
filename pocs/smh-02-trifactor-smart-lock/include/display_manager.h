#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "types.h"

class DisplayManager {
public:
    DisplayManager();
    void begin();
    void update();

    void showHome(const String& timeStr, const String& dateStr, LockState state, bool wifiConnected);
    void showPinInput(const String& maskedPin, int length);
    void showAccessGranted(const String& methodStr, int remainingSec);
    void showAccessDenied(const String& reasonStr, int failedAttempts);
    void showLockout(int remainingSec);

    void showMessage(const String& line1, const String& line2, const String& line3 = "");

private:
    Adafruit_SSD1306 display;
    bool displayFound;
    unsigned long lastRefreshTime;

    void drawHeader(const String& timeStr, bool wifiConnected);
};

extern DisplayManager displayManager;

#endif // DISPLAY_MANAGER_H
