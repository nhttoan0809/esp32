#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "app_config.h"
#include "cloud_client.h"
#include "config_store.h"
#include "device_config.h"
#include "device_controller.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

using namespace app_config;

namespace {

WiFiManager wm;
ConfigStore configStore;
DeviceController deviceController;
CloudClient cloudClient;

DeviceConfig activeConfig;
char serverHostParam[128] = "";
char serverPortParam[8] = "443";
char serverPathParam[64] = "/ws/devices";

bool shouldSaveCustomConfig = false;

// ===== Lamp Button Tracking (GPIO 14) =====
bool lastLampReading = HIGH;
bool stableLampReading = HIGH;
uint32_t lampChangedAt = 0;
uint32_t lampPressedAt = 0;
bool lampIsPressing = false;
bool lampForceOffTriggered = false;

void saveConfigCallback() {
  Serial.println(F("[WM] 💾 User saved new configuration from portal!"));
  shouldSaveCustomConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println(F("========================================================="));
  Serial.printf("[WM] 🟡 SOFTAP ACTIVE: %s (IP: %s)\r\n",
                myWiFiManager->getConfigPortalSSID().c_str(),
                WiFi.softAPIP().toString().c_str());
  Serial.println(F("[WM] 👉 Connect to this Wi-Fi to configure your device"));
  Serial.println(F("========================================================="));
  deviceController.setSetupReady(true);
}

void startCloudConnection() {
  deviceController.setSetupReady(false);
  deviceController.setWifiConnected(true);

  activeConfig.wifiSsid = WiFi.SSID();
  activeConfig.wifiPassword = WiFi.psk();
  activeConfig.serverHost = serverHostParam;
  activeConfig.serverPort = static_cast<uint16_t>(atoi(serverPortParam));
  if (activeConfig.serverPort == 0) {
    activeConfig.serverPort = HTTPS_PORT;
  }
  activeConfig.serverPath = serverPathParam;
  activeConfig.valid = true;

  Serial.printf("[CLOUD] 🌐 Starting Cloud WSS connection: host=%s port=%u path=%s device_id=%s\r\n",
                activeConfig.serverHost.c_str(),
                activeConfig.serverPort,
                activeConfig.serverPath.c_str(),
                DEVICE_ID);

  cloudClient.setReportedState(deviceController.realDeviceOn());
  cloudClient.begin(activeConfig, DEVICE_ID, DEVICE_TOKEN);
}

void handleLampButton(uint32_t now) {
  const bool reading = digitalRead(LAMP_BUTTON_PIN);
  if (reading != lastLampReading) {
    lastLampReading = reading;
    lampChangedAt = now;
  }

  if (now - lampChangedAt >= BUTTON_DEBOUNCE_MS) {
    if (reading != stableLampReading) {
      stableLampReading = reading;

      if (stableLampReading == LOW) {
        // Button pressed down
        lampIsPressing = true;
        lampPressedAt = now;
        lampForceOffTriggered = false;
      } else {
        // Button released
        if (lampIsPressing && !lampForceOffTriggered) {
          // Short press: Toggle lamp
          const bool newState = deviceController.toggleRealDevice();
          Serial.printf("[BUTTON] 💡 Lamp Toggle: New State = %s\r\n",
                        newState ? "ON" : "OFF");
          cloudClient.reportState(newState);
        }
        lampIsPressing = false;
      }
    }
  }

  // Safety Force-OFF check (Hold >= 3s)
  if (lampIsPressing && !lampForceOffTriggered) {
    if (now - lampPressedAt >= LAMP_FORCE_OFF_HOLD_MS) {
      lampForceOffTriggered = true;
      Serial.println(F("[SAFETY] ⚠️ Lamp Button Held >= 3s: Force OFF triggered!"));
      deviceController.forceOff();
      cloudClient.reportState(false);
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(100);

  deviceController.begin();

  cloudClient.setApplyStateHandler([](bool on) {
    deviceController.setRealDevice(on);
    return deviceController.realDeviceOn();
  });

  Serial.println();
  Serial.println(F("========================================================="));
  Serial.println(F("🚀 SMART LAMP CONTROLLER INITIALIZED"));
  Serial.printf("   Device ID: %s | Active LOW Relay: GPIO%u | LED: GPIO%u\r\n",
                DEVICE_ID, RELAY_PIN, REAL_DEVICE_PIN);
  Serial.println(F("========================================================="));

  // Load stored server config from NVS
  DeviceConfig stored;
  if (configStore.load(stored)) {
    strncpy(serverHostParam, stored.serverHost.c_str(), sizeof(serverHostParam) - 1);
    snprintf(serverPortParam, sizeof(serverPortParam), "%u", stored.serverPort);
    strncpy(serverPathParam, stored.serverPath.c_str(), sizeof(serverPathParam) - 1);
    Serial.printf("[NVS] ✅ Loaded Server Config: %s:%s%s\r\n",
                  serverHostParam, serverPortParam, serverPathParam);
  } else {
    // Preconfig fallback
    strncpy(serverHostParam, WOKWI_PRECONFIG_SERVER_HOST, sizeof(serverHostParam) - 1);
  }

  // Wokwi simulation bypass when preconfig is enabled and no NVS is stored
  if (WOKWI_PRECONFIG_ENABLED && !stored.valid) {
    Serial.println(F("[WOKWI] ⚡ Using Wokwi Preconfigured Wi-Fi Connection..."));
    WiFi.mode(WIFI_STA);
    WiFi.begin(WOKWI_PRECONFIG_SSID, WOKWI_PRECONFIG_PASSWORD);
    uint32_t startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
      delay(250);
      Serial.print('.');
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("[WOKWI] 🎉 Wi-Fi Connected!"));
      Serial.printf("[WOKWI] IP: %s\r\n", WiFi.localIP().toString().c_str());
      startCloudConnection();
      return;
    }
  }

  // Standard WiFiManager configuration portal
  WiFiManagerParameter custom_server_host("host", "Server Host (Tunnel/Public Domain)", serverHostParam, 128, "placeholder=\"e.g. xxx.trycloudflare.com\"");
  WiFiManagerParameter custom_server_port("port", "Server Port", serverPortParam, 8, "placeholder=\"443\"");
  WiFiManagerParameter custom_server_path("path", "WebSocket Path", serverPathParam, 64, "placeholder=\"/ws/devices\"");

  wm.addParameter(&custom_server_host);
  wm.addParameter(&custom_server_port);
  wm.addParameter(&custom_server_path);

  wm.setTitle("Smart Lamp Setup");
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setAPCallback(configModeCallback);
  wm.setConnectTimeout(25);
  wm.setConfigPortalTimeout(180);

  String apSsid = String(AP_SSID_PREFIX) + String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);
  apSsid.toUpperCase();

  Serial.println(F("[WM] ⏳ Checking Wi-Fi or opening configuration portal..."));
  const bool res = wm.autoConnect(apSsid.c_str(), AP_PASSWORD);

  if (!res) {
    Serial.println(F("[WM] ❌ Wi-Fi connect failed or portal timeout reached."));
    deviceController.setSetupReady(false);
    deviceController.setWifiConnected(false);
  } else {
    Serial.println(F("[WM] 🎉 WI-FI CONNECTED SUCCESSFULLY!"));
    Serial.printf("[WM] 🌐 IP: %s | RSSI: %d dBm\r\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());

    if (shouldSaveCustomConfig) {
      strncpy(serverHostParam, custom_server_host.getValue(), sizeof(serverHostParam) - 1);
      strncpy(serverPortParam, custom_server_port.getValue(), sizeof(serverPortParam) - 1);
      strncpy(serverPathParam, custom_server_path.getValue(), sizeof(serverPathParam) - 1);

      DeviceConfig toSave;
      toSave.wifiSsid = WiFi.SSID();
      toSave.wifiPassword = WiFi.psk();
      toSave.serverHost = serverHostParam;
      toSave.serverPort = static_cast<uint16_t>(atoi(serverPortParam));
      toSave.serverPath = serverPathParam;
      toSave.valid = true;

      configStore.save(toSave);
      Serial.println(F("[NVS] 💾 Saved new server configuration to NVS Flash"));
    }

    startCloudConnection();
  }
}

void loop() {
  const uint32_t now = millis();
  const bool wifiOk = (WiFi.status() == WL_CONNECTED);

  // Maintain Cloud WSS connection
  cloudClient.loop(wifiOk);

  // Update status LEDs
  deviceController.setWifiConnected(wifiOk);
  deviceController.setServerConnected(cloudClient.online());

  // Handle hardware lamp button
  handleLampButton(now);

  delay(2);
}
