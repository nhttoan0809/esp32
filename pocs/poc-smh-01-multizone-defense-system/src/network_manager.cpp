#include "network_manager.h"

void NetworkManager::begin() {
  Serial.printf("[WIFI] Dang ket noi toi mang: %s ...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // Không dùng while(!connected) để tránh block hệ thống an ninh offline
}

void NetworkManager::update() {
  unsigned long now = millis();
  if (now - _lastWifiCheck >= 5000) {
    _lastWifiCheck = now;
    if (WiFi.status() == WL_CONNECTED) {
      if (!_wifiConnected) {
        _wifiConnected = true;
        Serial.printf("[WIFI] Da ket noi thanh cong! IP: %s\n", WiFi.localIP().toString().c_str());
      }
    } else {
      if (_wifiConnected) {
        _wifiConnected = false;
        Serial.println("[WIFI] Mat ket noi mang. He thong van hoat dong offline 100%!");
      }
    }
  }
}

bool NetworkManager::isConnected() const {
  return _wifiConnected;
}

bool NetworkManager::sendTelegramAlert(SecurityZone zone, const String &timeStr) {
  if (!_wifiConnected) {
    Serial.println("[TELEGRAM] Khong the gui tin nhan do chua co Wi-Fi!");
    return false;
  }

  Serial.println("[TELEGRAM] Dang gui tin nhan canh bao khan cap toi gia chu...");

  WiFiClientSecure client;
  client.setInsecure(); // Bỏ qua kiểm tra chứng chỉ SSL để kết nối nhanh chóng
  client.setTimeout(3); // Timeout ngắn 3 giây tránh chặn hệ thống

  HTTPClient https;
  String message = "🚨 [CANH BAO AN NINH SMH-01] 🚨\n";
  message += "Phat hien xam nhap tai: " + String(getZoneName(zone)) + "\n";
  message += "Thoi diem: " + timeStr + "\n";
  message += "Phan ung: Da bat Den pha va Coi hu ngoai troi!";

  // URL encode sơ bộ
  String urlEncoded = "";
  for (char c : message) {
    if (c == ' ') urlEncoded += "%20";
    else if (c == '\n') urlEncoded += "%0A";
    else urlEncoded += c;
  }

  String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) +
               "/sendMessage?chat_id=" + String(TELEGRAM_CHAT_ID) +
               "&text=" + urlEncoded;

  if (https.begin(client, url)) {
    int httpCode = https.GET();
    if (httpCode > 0) {
      Serial.printf("[TELEGRAM] Gui thanh cong! HTTP Response: %d\n", httpCode);
      https.end();
      return true;
    } else {
      Serial.printf("[TELEGRAM] Gui that bai! Error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  }
  return false;
}
