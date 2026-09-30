#include "rtc_manager.h"
#include "config.h"
#include <Wire.h>

RtcManager rtcManager;

RtcManager::RtcManager()
    : rtcFound(false),
      isDs3231(false),
      baseEpoch(1727712000), // Default 2024-09-30 16:00:00 UTC
      baseMillis(0) {}

void RtcManager::begin() {
    baseMillis = millis();

    // Check DS1307 first (matches Wokwi part wokwi-ds1307)
    if (rtc1307.begin()) {
        rtcFound = true;
        isDs3231 = false;
        if (!rtc1307.isrunning()) {
            rtc1307.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
        return;
    }

    // Try DS3231 (common on physical breakouts)
    if (rtc3231.begin()) {
        rtcFound = true;
        isDs3231 = true;
        if (rtc3231.lostPower()) {
            rtc3231.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
        return;
    }

    rtcFound = false;
}

DateTime RtcManager::getCurrentDateTime() {
    if (rtcFound) {
        if (isDs3231) {
            return rtc3231.now();
        } else {
            return rtc1307.now();
        }
    }

    // Software RTC fallback
    unsigned long elapsedSeconds = (millis() - baseMillis) / 1000;
    return DateTime(baseEpoch + elapsedSeconds);
}

String RtcManager::getFormattedTime() {
    DateTime now = getCurrentDateTime();
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    return String(buf);
}

String RtcManager::getFormattedDate() {
    DateTime now = getCurrentDateTime();
    char buf[16];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", now.year(), now.month(), now.day());
    return String(buf);
}

String RtcManager::getFormattedDateTime() {
    DateTime now = getCurrentDateTime();
    char buf[24];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
    return String(buf);
}

uint32_t RtcManager::getEpochTime() {
    DateTime now = getCurrentDateTime();
    return now.unixtime();
}

void RtcManager::setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second) {
    DateTime dt(year, month, day, hour, minute, second);
    if (rtcFound) {
        if (isDs3231) {
            rtc3231.adjust(dt);
        } else {
            rtc1307.adjust(dt);
        }
    }
    baseEpoch = dt.unixtime();
    baseMillis = millis();
}

bool RtcManager::isHardwareRtcAvailable() const {
    return rtcFound;
}

void RtcManager::update() {
    // Nothing mandatory per tick, hardware RTC handles crystal oscillation
}
