#pragma once

#include <Arduino.h>
#include "config.h"

struct SensorData {
  float temperature = 25.0f;
  float humidity = 60.0f;
  int light_level = 1;         // 1: Sáng (Light), 0: Tối (Dark)
  int light_analog = 2048;     // 0-4095
  uint32_t rtc_timestamp = 0;  // Unix timestamp
  char rtc_time_str[16] = "00:00:00";
  bool dht_valid = false;
  bool rtc_valid = false;
};

class SensorsManager {
public:
  SensorsManager();
  bool begin();
  bool update(); // Gọi trong loop(), tự động kiểm tra chu kỳ non-blocking
  const SensorData& getData() const { return _data; }

private:
  SensorData _data;
  unsigned long _lastReadMs = 0;
  void readDHT();
  void readLDR();
  void readRTC();
};
