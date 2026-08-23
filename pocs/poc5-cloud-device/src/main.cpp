#include <Arduino.h>
#include <WiFi.h>

#include "app_config.h"
#include "cloud_client.h"
#include "config_store.h"
#include "device_config.h"
#include "device_controller.h"
#include "provisioning_portal.h"
#include "wifi_manager.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

using namespace app_config;

namespace {

ConfigStore configStore;
DeviceController deviceController;
WifiManager wifiManager;
CloudClient cloudClient;
ProvisioningPortal portal;

DeviceConfig storedConfig;
DeviceConfig activeConfig;
DeviceConfig pendingConfig;
bool pendingConfigActive = false;
uint32_t pendingConfigStartedAt = 0;
uint32_t portalOpenedAt = 0;
String applicationError;
uint8_t previousApClients = 0;

bool lastButtonReading = HIGH;
bool stableButtonReading = HIGH;
uint32_t buttonChangedAt = 0;
uint32_t buttonPressedAt = 0;

bool deadlineReached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

const char *wifiStateName() {
  switch (wifiManager.state()) {
    case WifiManager::State::Connecting:
      return "connecting";
    case WifiManager::State::Connected:
      return "connected";
    case WifiManager::State::RetryWait:
      return "retry_wait";
    case WifiManager::State::Failed:
      return "failed";
    case WifiManager::State::Idle:
    default:
      return "idle";
  }
}

void openPortal() {
  if (portal.begin(DEVICE_ID)) {
    portalOpenedAt = millis();
  } else {
    applicationError = F("softap_start_failed");
    Serial.println("FATAL softap_start_failed");
  }
}

void beginConnection(const DeviceConfig &config, bool finiteWifiAttempts) {
  activeConfig = config;
  cloudClient.setReportedState(deviceController.realDeviceOn());
  cloudClient.begin(activeConfig, DEVICE_ID, DEVICE_TOKEN);
  wifiManager.connect(activeConfig, finiteWifiAttempts);
}

void beginPreconfigConnection() {
  DeviceConfig preconfig;
  preconfig.wifiSsid = WOKWI_PRECONFIG_SSID;
  preconfig.wifiPassword = WOKWI_PRECONFIG_PASSWORD;
  preconfig.serverHost = WOKWI_PRECONFIG_SERVER_HOST;
  preconfig.serverPort = HTTPS_PORT;
  preconfig.serverPath = DEFAULT_SERVER_PATH;
  preconfig.valid = true;
  // Deliberately not written to NVS: Wokwi resets NVS on every run, so this is
  // only a starting point for this simulation. Press the setup button to open
  // the portal if you still need to configure the device manually.
  Serial.printf("PRECONFIG ssid=%s host=%s port=%u path=%s\r\n",
                WOKWI_PRECONFIG_SSID, WOKWI_PRECONFIG_SERVER_HOST,
                static_cast<unsigned>(HTTPS_PORT), DEFAULT_SERVER_PATH);
  beginConnection(preconfig, false);
}

void fallbackFromPending(const __FlashStringHelper *reason) {
  applicationError = String(reason);
  Serial.printf("PENDING_CONFIG_FAILED reason=%s\r\n",
                applicationError.c_str());
  cloudClient.stop();
  wifiManager.stop();
  clearDeviceConfig(activeConfig);
  clearDeviceConfig(pendingConfig);
  pendingConfigActive = false;
  openPortal();

  if (storedConfig.valid) {
    Serial.println("STORED_CONFIG_FALLBACK");
    beginConnection(storedConfig, false);
  }
}

void commitPendingConfig() {
  if (!configStore.save(pendingConfig)) {
    fallbackFromPending(F("nvs_write_failed"));
    return;
  }

  clearDeviceConfig(storedConfig);
  storedConfig = pendingConfig;
  activeConfig = storedConfig;
  clearDeviceConfig(pendingConfig);
  pendingConfigActive = false;
  applicationError = "";
  portalOpenedAt = millis();
  Serial.println("CONFIG_COMMITTED namespace=poc5_cfg");
}

void applySubmittedConfig() {
  DeviceConfig submitted;
  if (!portal.takeSubmittedConfig(submitted)) {
    return;
  }

  cloudClient.stop();
  wifiManager.stop();
  clearDeviceConfig(pendingConfig);
  pendingConfig = submitted;
  clearSensitiveString(submitted.wifiPassword);
  pendingConfigActive = true;
  pendingConfigStartedAt = millis();
  applicationError = "";
  Serial.println("PENDING_CONFIG_STARTED");
  beginConnection(pendingConfig, true);
}

void factoryReset() {
  const bool cleared = configStore.clear();
  cloudClient.stop();
  wifiManager.stop();
  clearDeviceConfig(storedConfig);
  clearDeviceConfig(activeConfig);
  clearDeviceConfig(pendingConfig);
  pendingConfigActive = false;
  deviceController.setRealDevice(false);
  applicationError = cleared ? String() : String(F("nvs_reset_failed"));
  openPortal();
  Serial.printf("FACTORY_RESET result=%s\r\n", cleared ? "ok" : "failed");
}

void handleButton(uint32_t now) {
  const bool reading = digitalRead(SETUP_BUTTON_PIN);
  if (reading != lastButtonReading) {
    lastButtonReading = reading;
    buttonChangedAt = now;
  }
  if (reading == stableButtonReading ||
      now - buttonChangedAt < BUTTON_DEBOUNCE_MS) {
    return;
  }

  stableButtonReading = reading;
  if (stableButtonReading == LOW) {
    buttonPressedAt = now;
    return;
  }

  const uint32_t heldMs = now - buttonPressedAt;
  if (heldMs >= BUTTON_FACTORY_RESET_MS) {
    factoryReset();
  } else {
    Serial.println("SETUP_BUTTON_SHORT_PRESS");
    applicationError = "";
    openPortal();
  }
}

void updatePendingConfig(uint32_t now) {
  if (!pendingConfigActive) {
    return;
  }
  if (wifiManager.failed()) {
    fallbackFromPending(F("wifi_connect_failed"));
    return;
  }
  if (cloudClient.online()) {
    commitPendingConfig();
    return;
  }
  if (now - pendingConfigStartedAt >= PENDING_CONFIG_TIMEOUT_MS) {
    fallbackFromPending(F("cloud_validation_timeout"));
  }
}

void updatePortalLifecycle(uint32_t now) {
  if (storedConfig.valid && wifiManager.attempts() >= 3 &&
      !wifiManager.connected() && !portal.active()) {
    Serial.println("PROVISIONING_FALLBACK reason=wifi_retries");
    openPortal();
  }

  if (portal.active() && !pendingConfigActive && cloudClient.online() &&
      now - portalOpenedAt >= AP_GRACE_PERIOD_MS) {
    portal.stop();
  }
}

void updateIndicatorsAndStatus() {
  const uint8_t currentApClients = portal.clientCount();
  if (currentApClients != previousApClients) {
    Serial.printf(currentApClients > previousApClients
                      ? "AP_CLIENT_CONNECTED clients=%u\r\n"
                      : "AP_CLIENT_DISCONNECTED clients=%u\r\n",
                  currentApClients);
    previousApClients = currentApClients;
  }

  deviceController.setSetupReady(portal.active());
  deviceController.setProvisioningClientConnected(currentApClients > 0);
  deviceController.setWifiConnected(wifiManager.connected());
  deviceController.setServerConnected(cloudClient.online());

  PortalRuntimeStatus status;
  status.wifiState = wifiStateName();
  status.cloudState = cloudClient.stateName();
  status.stationIp = wifiManager.connected()
                         ? WiFi.localIP().toString()
                         : String();
  status.realDeviceOn = deviceController.realDeviceOn();
  if (!applicationError.isEmpty()) {
    status.lastError = applicationError;
  } else if (!wifiManager.lastError().isEmpty()) {
    status.lastError = wifiManager.lastError();
  } else {
    status.lastError = cloudClient.lastError();
  }
  portal.setRuntimeStatus(status);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  deviceController.begin();
  wifiManager.begin();
  cloudClient.setApplyStateHandler([](bool on) {
    deviceController.setRealDevice(on);
    return deviceController.realDeviceOn();
  });

  Serial.printf("POC5_BOOT device_id=%s\r\n", DEVICE_ID);
  if (configStore.load(storedConfig)) {
    Serial.println("STORED_CONFIG_LOADED");
    beginConnection(storedConfig, false);
  } else {
    Serial.println("NO_STORED_CONFIG");
    if (WOKWI_PRECONFIG_ENABLED) {
      beginPreconfigConnection();
    } else {
      openPortal();
    }
  }
}

void loop() {
  const uint32_t now = millis();
  portal.loop();
  applySubmittedConfig();
  if (portal.takeResetRequest()) {
    factoryReset();
  }

  wifiManager.loop();
  cloudClient.loop(wifiManager.connected());
  updatePendingConfig(now);
  updatePortalLifecycle(now);
  handleButton(now);
  updateIndicatorsAndStatus();
  delay(2);
}
