#pragma once

#include <Arduino.h>

class DeviceController {
 public:
  void begin();
  void setSetupReady(bool ready);
  void setWifiConnected(bool connected);
  void setServerConnected(bool connected);
  void setRealDevice(bool on);
  bool toggleRealDevice();
  void forceOff();
  bool realDeviceOn() const;

 private:
  bool setupReady_ = false;
  bool wifiConnected_ = false;
  bool serverConnected_ = false;
  bool realDeviceOn_ = false;
};
