#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <Preferences.h>
#include "config.h"
#include "types.h"

class RtcManager {
public:
  bool begin();
  String getFormattedTime();
  uint32_t getUnixTime();

  void logEvent(SecurityZone zone, const char* source, const char* desc);
  int getLogCount() const;
  SecurityLogEntry getLog(int index) const;
  void printAllLogs();
  void clearLogs();

private:
  RTC_DS1307 _rtc;
  bool _rtcAvailable = false;
  Preferences _prefs;

  SecurityLogEntry _logs[MAX_LOG_ENTRIES];
  int _logCount = 0;
  int _logHead = 0;

  void loadLogsFromNvs();
  void saveLogsToNvs();
};
