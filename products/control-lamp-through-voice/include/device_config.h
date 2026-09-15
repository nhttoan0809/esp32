#pragma once

#include <Arduino.h>

struct DeviceConfig {
  String wifiSsid;
  String wifiPassword;
  String serverHost;
  uint16_t serverPort = 443;
  String serverPath = "/ws/devices";
  bool valid = false;
};

inline void clearSensitiveString(String &value) {
  for (size_t index = 0; index < value.length(); ++index) {
    value.setCharAt(index, '\0');
  }
  value = "";
}

inline void clearDeviceConfig(DeviceConfig &config) {
  clearSensitiveString(config.wifiPassword);
  config.wifiSsid = "";
  config.serverHost = "";
  config.serverPort = 443;
  config.serverPath = "/ws/devices";
  config.valid = false;
}
