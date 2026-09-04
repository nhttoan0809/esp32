#pragma once

#include <Arduino.h>

class DeviceController {
 public:
  void begin();
  void setSetupReady(bool ready);
  void setProvisioningClientConnected(bool connected);
  void setWifiConnected(bool connected);
  void setServerConnected(bool connected);
  void setRealDevice(bool on);
  bool realDeviceOn() const;

 private:
  bool setupReady_ = false;
  bool clientConnected_ = false;
  bool wifiConnected_ = false;
  bool serverConnected_ = false;
  bool realDeviceOn_ = false;
};

