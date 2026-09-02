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
  deviceController.setProvisioningClientConnected(false);
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

  // Lắng nghe sự kiện SoftAP Client kết nối/ngắt kết nối (LED 19)
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    deviceController.setProvisioningClientConnected(true);
  }, ARDUINO_EVENT_WIFI_AP_STACONNECTED);

  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (WiFi.softAPgetStationNum() == 0) {
      deviceController.setProvisioningClientConnected(false);
    }
  }, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

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
  WiFiManagerParameter custom_server_host("host", "Server Host (Cloudflare/ngrok)", serverHostParam, 128, "placeholder=\"e.g. xxx.trycloudflare.com\"");
  WiFiManagerParameter custom_server_port("port", "Server Port", serverPortParam, 8, "placeholder=\"443\"");
  WiFiManagerParameter custom_server_path("path", "WebSocket Path", serverPathParam, 64, "placeholder=\"/ws/devices\"");

  wm.addParameter(&custom_server_host);
  wm.addParameter(&custom_server_port);
  wm.addParameter(&custom_server_path);

  wm.setTitle("WiFi Configuration");
  wm.setCustomHeadElement(
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no\">"
      "<style>"
      ":root{--bg:#f1f5f9;--card:#ffffff;--primary:#007aff;--primary-hover:#0062cc;--text:#0f172a;--muted:#64748b;--border:#cbd5e1;}"
      "*{box-sizing:border-box;-webkit-tap-highlight-color:transparent;}"
      "body{background:var(--bg)!important;color:var(--text)!important;font-family:-apple-system,system-ui,sans-serif!important;margin:0;padding:max(16px,env(safe-area-inset-top)) 14px max(24px,env(safe-area-inset-bottom)) 14px;}"
      ".c{width:94%!important;max-width:390px!important;background:var(--card)!important;border-radius:20px!important;border:1px solid rgba(0,0,0,0.06)!important;padding:26px 20px!important;box-shadow:0 12px 36px rgba(0,0,0,0.06)!important;margin:10px auto!important;text-align:left!important;}"
      "h1,h2,h3{letter-spacing:-0.02em;}"
      "h1{font-size:1.55rem!important;color:var(--primary)!important;font-weight:800!important;margin:0 0 16px 0!important;text-align:center!important;}"
      "h3{font-size:0.88rem!important;color:var(--muted)!important;margin:14px 0 6px!important;text-align:left!important;font-weight:700!important;}"
      "label,.custom-lbl{font-size:0.9rem!important;font-weight:700!important;color:#1e293b!important;display:block!important;margin:12px 0 6px!important;}"
      "input,select,.custom-select{width:100%!important;box-sizing:border-box!important;background:#ffffff!important;color:var(--text)!important;border:1.5px solid var(--border)!important;border-radius:10px!important;padding:0 14px!important;font-size:16px!important;outline:none!important;margin-bottom:12px!important;height:48px!important;line-height:48px!important;transition:border-color 0.2s,box-shadow 0.2s;touch-action:manipulation;}"
      "input:focus,select:focus,.custom-select:focus{border-color:var(--primary)!important;box-shadow:0 0 0 3px rgba(0,122,255,0.18)!important;}"
      ".select-row{display:flex!important;gap:8px!important;align-items:center!important;margin-bottom:12px!important;}"
      ".select-row select{margin-bottom:0!important;flex:1!important;}"
      ".refresh-icon-btn{display:inline-flex!important;align-items:center!important;justify-content:center!important;width:48px!important;height:48px!important;background:#f8fafc!important;border:1.5px solid var(--border)!important;border-radius:10px!important;text-decoration:none!important;font-size:1.2rem!important;flex-shrink:0!important;color:var(--primary)!important;}"
      ".refresh-icon-btn:hover{background:#e2e8f0!important;}"
      "button{width:100%!important;background:var(--primary)!important;color:#fff!important;border:none!important;border-radius:10px!important;height:48px!important;font-size:1rem!important;font-weight:700!important;cursor:pointer!important;margin-top:14px!important;box-shadow:0 4px 14px rgba(0,122,255,0.25)!important;transition:all 0.2s;touch-action:manipulation;}"
      "button:hover{background:var(--primary-hover)!important;}"
      "button.D{background:#ef4444!important;box-shadow:0 4px 14px rgba(239,68,68,0.25)!important;}"
      "a{color:var(--primary)!important;text-decoration:none!important;font-weight:600;}"
      "div:has(> a[href='#p']),div:has(> a[onclick*='c(this)']),div.q,.q{display:none!important;}"
      "</style>"
      "<script>"
      "window.addEventListener('DOMContentLoaded',function(){"
      "var s=document.getElementById('s'),p=document.getElementById('p');"
      "if(!s)return;"
      "var links=document.querySelectorAll(\"a[href='#p'],a[onclick*='c(this)']\");"
      "var set={};"
      "for(var i=0;i<links.length;i++){"
      "var txt=links[i].getAttribute('data-ssid')||links[i].innerText||links[i].textContent;"
      "if(txt)set[txt.trim()]=true;"
      "}"
      "var wrap=document.createElement('div');"
      "wrap.innerHTML='<label class=\"custom-lbl\">WiFi Network</label><div class=\"select-row\"><select id=\"wifi-sel\" class=\"custom-select\"><option value=\"\" disabled selected>Please select a network</option></select><a href=\"/wifi\" class=\"refresh-icon-btn\" title=\"Quét lại WiFi\">&#8635;</a></div>';"
      "var sel=wrap.querySelector('#wifi-sel');"
      "for(var name in set){"
      "var opt=document.createElement('option');"
      "opt.value=name;opt.innerText=name;"
      "if(s.value===name)opt.selected=true;"
      "sel.appendChild(opt);"
      "}"
      "var manual=document.createElement('option');"
      "manual.value='__custom__';manual.innerText='✍️ Manual / Other WiFi...';"
      "sel.appendChild(manual);"
      "var f=s.form||s.parentElement;"
      "f.insertBefore(wrap,s.previousElementSibling||s);"
      "if(!s.value){s.style.display='none';var prev=s.previousElementSibling;if(prev&&prev.tagName==='LABEL')prev.style.display='none';}"
      "sel.addEventListener('change',function(){"
      "if(this.value==='__custom__'){s.style.display='block';s.value='';s.focus();}"
      "else if(this.value){s.value=this.value;s.style.display='none';if(p)p.focus();}"
      "});"
      "});"
      "</script>"
  );

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
    deviceController.setProvisioningClientConnected(false);
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
