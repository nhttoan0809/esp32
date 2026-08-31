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

bool lastButtonReading = HIGH;
bool stableButtonReading = HIGH;
uint32_t buttonChangedAt = 0;
uint32_t buttonPressedAt = 0;

void saveConfigCallback() {
  Serial.println(F("[WM] 💾 Người dùng đã lưu cấu hình mới từ Portal!"));
  shouldSaveCustomConfig = true;
}

void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println(F("========================================================="));
  Serial.printf("[WM] 🟡 ĐÃ BẬT SOFTAP: %s (IP: %s)\r\n",
                myWiFiManager->getConfigPortalSSID().c_str(),
                WiFi.softAPIP().toString().c_str());
  Serial.println(F("[WM] 👉 Hãy dùng điện thoại kết nối vào Wi-Fi này để cấu hình"));
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

  Serial.printf("[CLOUD] 🌐 Khởi động kết nối Cloud: host=%s port=%u path=%s device_id=%s\r\n",
                activeConfig.serverHost.c_str(),
                activeConfig.serverPort,
                activeConfig.serverPath.c_str(),
                DEVICE_ID);

  cloudClient.setReportedState(deviceController.realDeviceOn());
  cloudClient.begin(activeConfig, DEVICE_ID, DEVICE_TOKEN);
}

void factoryReset() {
  Serial.println(F("[SYSTEM] ⚠️ FACTORY RESET: Đang xoá toàn bộ cấu hình NVS..."));
  wm.resetSettings();
  configStore.clear();
  deviceController.setRealDevice(false);
  delay(1000);
  ESP.restart();
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
    Serial.println(F("[BUTTON] 🔘 Nhấn ngắn: Mở lại On-Demand Config Portal..."));
    deviceController.setSetupReady(true);
    wm.startConfigPortal("ESP32-SETUP-POC5", AP_PASSWORD);
    deviceController.setSetupReady(false);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  deviceController.begin();

  cloudClient.setApplyStateHandler([](bool on) {
    deviceController.setRealDevice(on);
    return deviceController.realDeviceOn();
  });

  Serial.println();
  Serial.println(F("========================================================="));
  Serial.printf("🚀 POC5 CLOUD WEBSOCKET DEVICE - ID: %s\r\n", DEVICE_ID);
  Serial.println(F("========================================================="));

  // Tải cấu hình Server đã lưu trong NVS (nếu có)
  DeviceConfig stored;
  if (configStore.load(stored)) {
    strncpy(serverHostParam, stored.serverHost.c_str(), sizeof(serverHostParam) - 1);
    snprintf(serverPortParam, sizeof(serverPortParam), "%u", stored.serverPort);
    strncpy(serverPathParam, stored.serverPath.c_str(), sizeof(serverPathParam) - 1);
    Serial.printf("[NVS] ✅ Đã tải cấu hình Server: %s:%s%s\r\n",
                  serverHostParam, serverPortParam, serverPathParam);
  } else {
    // Mặc định từ preconfig / fallback
    strncpy(serverHostParam, WOKWI_PRECONFIG_SERVER_HOST, sizeof(serverHostParam) - 1);
  }

  // Khởi tạo các ô nhập tùy chỉnh cho WiFiManager
  WiFiManagerParameter custom_server_host("host", "Server Host (ngrok domain)", serverHostParam, 128);
  WiFiManagerParameter custom_server_port("port", "Server Port", serverPortParam, 8);
  WiFiManagerParameter custom_server_path("path", "WebSocket Path", serverPathParam, 64);

  wm.addParameter(&custom_server_host);
  wm.addParameter(&custom_server_port);
  wm.addParameter(&custom_server_path);

  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setAPCallback(configModeCallback);
  wm.setConnectTimeout(25);       // Thử kết nối Wi-Fi trong 25 giây
  wm.setConfigPortalTimeout(180); // Mở portal trong 3 phút nếu không ai nhập sẽ tự đóng

  // Tự động tạo tên AP dựa trên MAC chip
  String apSsid = String(AP_SSID_PREFIX) + String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);
  apSsid.toUpperCase();

  Serial.println(F("[WM] ⏳ Đang kiểm tra Wi-Fi hoặc mở Portal cấu hình..."));
  const bool res = wm.autoConnect(apSsid.c_str(), AP_PASSWORD);

  if (!res) {
    Serial.println(F("[WM] ❌ Kết nối Wi-Fi thất bại hoặc hết thời gian chờ Portal."));
    deviceController.setSetupReady(false);
    deviceController.setWifiConnected(false);
  } else {
    Serial.println(F("[WM] 🎉 KẾT NỐI WI-FI THÀNH CÔNG!"));
    Serial.printf("[WM] 🌐 IP: %s | RSSI: %d dBm | Kênh CH%d\r\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI(), WiFi.channel());

    // Nếu người dùng vừa nhập cấu hình mới từ Web Portal -> lưu vào NVS
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
      Serial.println(F("[NVS] 💾 Đã lưu cấu hình Server mới vào Flash"));
    }

    startCloudConnection();
  }
}

void loop() {
  const uint32_t now = millis();

  // Duy trì kết nối Cloud Client khi Wi-Fi đang online
  const bool wifiOk = (WiFi.status() == WL_CONNECTED);
  cloudClient.loop(wifiOk);

  // Cập nhật trạng thái đèn LED
  deviceController.setWifiConnected(wifiOk);
  deviceController.setServerConnected(cloudClient.online());

  // Xử lý nút bấm Setup / Factory Reset
  handleButton(now);

  delay(2);
}
