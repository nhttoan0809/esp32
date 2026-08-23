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
  bool realDeviceOn_ = false;
};
