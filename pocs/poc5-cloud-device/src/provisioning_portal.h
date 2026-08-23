#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "device_config.h"

struct PortalRuntimeStatus {
  String wifiState = "idle";
  String cloudState = "disabled";
  String lastError;
  String stationIp;
  bool realDeviceOn = false;
};

class ProvisioningPortal {
 public:
  bool begin(const char *deviceId);
  void stop();
  void loop();
  bool active() const;
  uint8_t clientCount() const;
  const String &ssid() const;
  void setRuntimeStatus(const PortalRuntimeStatus &status);
  bool takeSubmittedConfig(DeviceConfig &config);
  bool takeResetRequest();

 private:
  void registerRoutes();
  void handleRoot();
  void handleStatus();
  void handleConfigureBody();
  void handleConfigure();
  void handleReset();
  void handleNotFound();
  void sendJson(int statusCode, const String &body);
  void sendError(int statusCode, const __FlashStringHelper *error);
  bool parseConfigureBody(DeviceConfig &config);

  WebServer server_{80};
  String deviceId_;
  String apSsid_;
  String configureBody_;
  PortalRuntimeStatus runtimeStatus_;
  DeviceConfig submittedConfig_;
  bool active_ = false;
  bool routesRegistered_ = false;
  bool submitted_ = false;
  bool resetRequested_ = false;
  bool bodyComplete_ = false;
  bool bodyTooLarge_ = false;
};
