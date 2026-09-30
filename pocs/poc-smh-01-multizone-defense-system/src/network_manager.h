#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "config.h"
#include "types.h"

class NetworkManager {
public:
  void begin();
  void update();

  bool isConnected() const;
  bool sendTelegramAlert(SecurityZone zone, const String &timeStr);

private:
  bool _wifiConnected = false;
  unsigned long _lastWifiCheck = 0;
  bool _alertSentForCurrentAlarm = false;
};
