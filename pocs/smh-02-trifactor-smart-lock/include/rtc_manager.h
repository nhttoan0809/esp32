#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <Arduino.h>
#include <RTClib.h>

class RtcManager {
public:
    RtcManager();
    void begin();
    void update();

    String getFormattedTime();
    String getFormattedDate();
    String getFormattedDateTime();
    uint32_t getEpochTime();

    void setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
    bool isHardwareRtcAvailable() const;

private:
    RTC_DS1307 rtc1307;
    RTC_DS3231 rtc3231;
    bool rtcFound;
    bool isDs3231;

    // Fallback software time tracking if hardware RTC is disconnected
    unsigned long baseEpoch;
    unsigned long baseMillis;

    DateTime getCurrentDateTime();
};

extern RtcManager rtcManager;

#endif // RTC_MANAGER_H
