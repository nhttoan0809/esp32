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
bool cloudStarted = false;

// Custom WiFiManager Parameters at static lifetime so pointers never dangle
WiFiManagerParameter custom_server_host("host", "Server Host (Tunnel/Public Domain)", serverHostParam, 128, "placeholder=\"e.g. xxx.trycloudflare.com\"");
WiFiManagerParameter custom_server_port("port", "Server Port", serverPortParam, 8, "placeholder=\"443\"");
WiFiManagerParameter custom_server_path("path", "WebSocket Path", serverPathParam, 64, "placeholder=\"/ws/devices\"");

// ===== Dual-Pin Lamp Button Tracking (GPIO 14 & GPIO 25) =====
bool lampButtonPressed = false;
uint32_t lampPressStartTime = 0;
uint32_t lampLastReleaseTime = 0;
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

void checkAndStartCloud() {
  if (!cloudStarted && WiFi.status() == WL_CONNECTED) {
    cloudStarted = true;
    Serial.println(F("[WM] 🎉 WI-FI CONNECTED!"));
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
      shouldSaveCustomConfig = false;
    }

    startCloudConnection();
  } else if (cloudStarted && WiFi.status() != WL_CONNECTED) {
    cloudStarted = false;
  }
}

void handleLampButton(uint32_t now) {
  const bool p14 = (digitalRead(LAMP_BUTTON_PIN) == LOW);
  const bool p25 = (digitalRead(ALT_BUTTON_PIN) == LOW);
  const bool isDown = (p14 || p25);
  const uint8_t triggeredPin = p14 ? LAMP_BUTTON_PIN : (p25 ? ALT_BUTTON_PIN : 0);

  // 1. Falling edge: Button pressed down -> Toggle IMMEDIATELY on press!
  if (isDown && !lampButtonPressed) {
    if (now - lampLastReleaseTime >= 50) {  // 50ms debounce lockout
      lampButtonPressed = true;
      lampPressStartTime = now;
      lampForceOffTriggered = false;

      const bool newState = deviceController.toggleRealDevice();
      Serial.printf("[BUTTON] 🔘 Press on GPIO%u -> Lamp Toggled: %s\r\n",
                    triggeredPin, newState ? "ON" : "OFF");
      cloudClient.reportState(newState);
    }
  }

  // 2. While held down:
  if (lampButtonPressed && isDown) {
    const uint32_t heldMs = now - lampPressStartTime;

    // Safety Force-OFF check (hold >= 3s)
    if (!lampForceOffTriggered && heldMs >= LAMP_FORCE_OFF_HOLD_MS) {
      lampForceOffTriggered = true;
      Serial.printf("[SAFETY] ⚠️ Lamp Button Held >= %u ms on GPIO%u: Force OFF triggered!\r\n",
                    LAMP_FORCE_OFF_HOLD_MS, triggeredPin);
      deviceController.forceOff();
      cloudClient.reportState(false);
    }

    // Factory Reset check (hold >= 10s): Wipes Wi-Fi & NVS, restarts device
    if (heldMs >= BUTTON_FACTORY_RESET_MS) {
      Serial.println(F("[SYSTEM] ⚠️ Button held >= 10s: FACTORY RESET! Clearing Wi-Fi & NVS..."));
      wm.resetSettings();
      configStore.clear();
      delay(500);
      ESP.restart();
    }
  }

  // 3. Rising edge: Button released
  if (lampButtonPressed && !isDown) {
    lampButtonPressed = false;
    lampLastReleaseTime = now;
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

  // SoftAP Client connection event logging
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("[WM] 📱 Client CONNECTED to SoftAP! MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                  info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
                  info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
                  info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);
  }, ARDUINO_EVENT_WIFI_AP_STACONNECTED);

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println(F("[WM] 📱 Client DISCONNECTED from SoftAP"));
  }, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

  // Station Event listeners for detailed Wi-Fi diagnostic
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    uint8_t reason = info.wifi_sta_disconnected.reason;
    Serial.printf("\r\n[WIFI] ❌ Station Disconnected / Connect Failed! Reason Code: %u\r\n", reason);
    if (reason == 202 || reason == 15 || reason == 204) {
      Serial.println(F("[WIFI] 🔑 Gợi ý: Sai mật khẩu Wi-Fi (Auth Fail / 4-way Handshake Timeout)!"));
    } else if (reason == 201) {
      Serial.println(F("[WIFI] 📡 Gợi ý: Không tìm thấy SSID hoặc sóng Wi-Fi 2.4GHz quá yếu (No AP Found)!"));
    } else if (reason == 203) {
      Serial.println(F("[WIFI] 🚫 Gợi ý: Router từ chối kết nối (Assoc Failed / Band Steering / Mạng Enterprise)!"));
    }
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("\r\n[WIFI] ✅ Station Connected to Router! Channel: %u\r\n", WiFi.channel());
  }, ARDUINO_EVENT_WIFI_STA_CONNECTED);

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("[WIFI] 🎉 Station Got IP: %s\r\n", WiFi.localIP().toString().c_str());
  }, ARDUINO_EVENT_WIFI_STA_GOT_IP);

  Serial.println();
  Serial.println(F("========================================================="));
  Serial.println(F("🚀 SMART LAMP CONTROLLER INITIALIZED"));
  Serial.printf("   Device ID: %s | Relay: GPIO%u (Active LOW) | Status LED: GPIO%u\r\n",
                DEVICE_ID, RELAY_PIN, REAL_DEVICE_PIN);
  Serial.printf("   Buttons: GPIO%u (primary) & GPIO%u (alternate)\r\n",
                LAMP_BUTTON_PIN, ALT_BUTTON_PIN);
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
      cloudStarted = true;
      startCloudConnection();
      return;
    }
  }

  // Non-blocking WiFiManager configuration portal
  custom_server_host.setValue(serverHostParam, sizeof(serverHostParam));
  custom_server_port.setValue(serverPortParam, sizeof(serverPortParam));
  custom_server_path.setValue(serverPathParam, sizeof(serverPathParam));

  wm.addParameter(&custom_server_host);
  wm.addParameter(&custom_server_port);
  wm.addParameter(&custom_server_path);

  // Fix SoftAP IP and subnet to guarantee DHCP service for connecting clients
  wm.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));

  wm.setConfigPortalBlocking(false);  // CRITICAL: Non-blocking mode ensures loop() runs immediately!
  wm.setTitle("Smart Lamp Setup");
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setAPCallback(configModeCallback);
  wm.setConnectTimeout(30);          // Allow up to 30s for router WPA2 handshake & DHCP
  wm.setConnectRetries(3);           // Retry 3 times
  wm.setCleanConnect(true);          // Clean disconnect before connecting
  wm.setConfigPortalTimeout(180);

  String apSsid = String(AP_SSID_PREFIX) + String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);
  apSsid.toUpperCase();

  const char *apPass = (strlen(AP_PASSWORD) >= 8) ? AP_PASSWORD : nullptr;
  Serial.printf("[WM] ⏳ Starting AP: %s | Security: %s\r\n",
                apSsid.c_str(), apPass ? apPass : "OPEN (NO PASSWORD - 1-TAP CONNECT)");

  const bool res = wm.autoConnect(apSsid.c_str(), apPass);

  if (res) {
    Serial.println(F("[WM] 🎉 WI-FI CONNECTED IMMEDIATELY IN SETUP!"));
    checkAndStartCloud();
  } else {
    Serial.println(F("[WM] ℹ️ Background Wi-Fi / SoftAP active. Physical button control is 100% active!"));
  }
}

void loop() {
  const uint32_t now = millis();

  // Process WiFiManager background events (DNS, WebServer, Captive Portal)
  wm.process();

  // Monitor Wi-Fi state & initiate Cloud WSS when connected
  checkAndStartCloud();

  const bool wifiOk = (WiFi.status() == WL_CONNECTED);

  // Maintain Cloud WSS connection
  cloudClient.loop(wifiOk);

  // Update status LEDs
  deviceController.setWifiConnected(wifiOk);
  deviceController.setServerConnected(cloudClient.online());

  // Handle hardware lamp button (always 100% active, zero blocking)
  handleLampButton(now);

  delay(2);
}
