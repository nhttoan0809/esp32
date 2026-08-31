#include "wifi_manager.h"

#include <WiFi.h>

#include "app_config.h"

using namespace app_config;

namespace {

bool deadlineReached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

const char *wifiStatusString(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD: return "NO_SHIELD";
    case WL_IDLE_STATUS: return "IDLE";
    case WL_NO_SSID_AVAIL: return "NO_SSID_AVAIL (Không tìm thấy SSID mạng này)";
    case WL_SCAN_COMPLETED: return "SCAN_COMPLETED";
    case WL_CONNECTED: return "CONNECTED (Đã kết nối)";
    case WL_CONNECT_FAILED: return "CONNECT_FAILED (Sai mật khẩu hoặc bị từ chối)";
    case WL_CONNECTION_LOST: return "CONNECTION_LOST (Mất kết nối)";
    case WL_DISCONNECTED: return "DISCONNECTED";
    default: return "UNKNOWN";
  }
}

const char *wifiDisconnectReasonString(uint8_t reason) {
  switch (reason) {
    case 1: return "UNSPECIFIED";
    case 2: return "PREV_AUTH_NOT_VALID";
    case 3: return "DEAUTH_LEAVING";
    case 4: return "DISASSOC_DUE_TO_INACTIVITY";
    case 5: return "DISASSOC_AP_BUSY";
    case 6: return "CLASS2_FRAME_FROM_NONAUTH_STA";
    case 7: return "CLASS3_FRAME_FROM_NONASSOC_STA";
    case 8: return "DISASSOC_STA_HAS_LEFT";
    case 9: return "STA_REQ_ASSOC_WITHOUT_AUTH";
    case 15: return "4WAY_HANDSHAKE_TIMEOUT (Sai mật khẩu hoặc lỗi bắt tay WPA)";
    case 200: return "BEACON_TIMEOUT (Mất sóng Wi-Fi / sóng quá yếu)";
    case 201: return "NO_AP_FOUND (Không tìm thấy SSID mạng Wi-Fi này - Có thể là 5GHz Only)";
    case 202: return "AUTH_FAIL (Sai mật khẩu Wi-Fi)";
    case 203: return "ASSOC_FAIL (Router từ chối kết nạp)";
    case 204: return "HANDSHAKE_TIMEOUT (Timeout bắt tay bảo mật)";
    case 205: return "CONNECTION_FAIL";
    default: return "OTHER_REASON";
  }
}

}  // namespace

void WifiManager::begin() {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_AP_STA);

  // Đăng ký bắt sự kiện Wi-Fi chi tiết của ESP32
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
      case ARDUINO_EVENT_WIFI_STA_START:
        Serial.println(F("[WIFI EVENT] 📡 STA_START: Giao diện Wi-Fi Station đã khởi động"));
        break;
      case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.printf("[WIFI EVENT] 🔗 STA_CONNECTED: Bắt tay lớp Link Layer thành công với SSID: %s (Kênh CH%d)\r\n",
                      info.wifi_sta_connected.ssid, info.wifi_sta_connected.channel);
        break;
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.printf("[WIFI EVENT] 🌐 STA_GOT_IP: Nhận IP thành công từ DHCP: %s | Gateway: %s | Subnet: %s\r\n",
                      IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str(),
                      IPAddress(info.got_ip.ip_info.gw.addr).toString().c_str(),
                      IPAddress(info.got_ip.ip_info.netmask.addr).toString().c_str());
        break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.printf("[WIFI EVENT] ❌ STA_DISCONNECTED: reason_code=%u (%s)\r\n",
                      info.wifi_sta_disconnected.reason,
                      wifiDisconnectReasonString(info.wifi_sta_disconnected.reason));
        break;
      default:
        break;
    }
  });
}

void WifiManager::scanAndPrintNetworks() {
  Serial.println(F("\n================================================================================"));
  Serial.println(F("🔍 BẮT ĐẦU QUÉT TẤT CẢ CÁC MẠNG WI-FI 2.4GHz XUNG QUANH ESP32..."));
  Serial.println(F("================================================================================"));
  
  const int n = WiFi.scanNetworks(false, false);
  if (n == 0) {
    Serial.println(F("⚠️ Không tìm thấy bất kỳ mạng Wi-Fi 2.4GHz nào gần đây!"));
  } else if (n < 0) {
    Serial.printf("❌ Quét Wi-Fi thất bại (code=%d)\r\n", n);
  } else {
    Serial.printf("✅ Tìm thấy %d mạng Wi-Fi 2.4GHz:\r\n", n);
    Serial.println(F("--------------------------------------------------------------------------------"));
    Serial.printf("%-3s | %-24s | %-8s | %-4s | %-16s | %s\r\n", 
                  "STT", "SSID (Tên mạng)", "Tín hiệu", "Kênh", "Bảo mật", "BSSID (MAC)");
    Serial.println(F("--------------------------------------------------------------------------------"));
    for (int i = 0; i < n; ++i) {
      const char *authStr = "UNKNOWN";
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN: authStr = "OPEN (Không mk)"; break;
        case WIFI_AUTH_WEP: authStr = "WEP"; break;
        case WIFI_AUTH_WPA_PSK: authStr = "WPA_PSK"; break;
        case WIFI_AUTH_WPA2_PSK: authStr = "WPA2_PSK"; break;
        case WIFI_AUTH_WPA_WPA2_PSK: authStr = "WPA/WPA2_PSK"; break;
        case WIFI_AUTH_WPA2_ENTERPRISE: authStr = "WPA2_ENTERPRISE"; break;
        case WIFI_AUTH_WPA3_PSK: authStr = "WPA3_SAE"; break;
        case WIFI_AUTH_WPA2_WPA3_PSK: authStr = "WPA2/WPA3"; break;
        default: authStr = "OTHER"; break;
      }
      Serial.printf("%-3d | %-24s | %4d dBm | CH%-2d | %-16s | %s\r\n",
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i),
                    WiFi.channel(i),
                    authStr,
                    WiFi.BSSIDstr(i).c_str());
    }
    Serial.println(F("--------------------------------------------------------------------------------"));
    Serial.println(F("💡 LƯU Ý KỸ THUẬT:"));
    Serial.println(F("   - Nếu mạng của bạn KHÔNG có trong bảng trên: Mạng đó là 5GHz Only hoặc"));
    Serial.println(F("     ở quá xa ngoài vùng phủ sóng 2.4GHz của ESP32."));
    Serial.println(F("   - Nếu mạng có trong bảng nhưng Auth là WPA3_SAE/ENTERPRISE: Cần đổi sang WPA2."));
  }
  Serial.println(F("================================================================================\n"));
  WiFi.scanDelete();
}

void WifiManager::connect(const DeviceConfig &config, bool finiteAttempts) {
  config_ = config;
  finiteAttempts_ = finiteAttempts;
  attempts_ = 0;
  lastError_ = "";
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_STA);  // Chuyển sang STA thuần túy để giải phóng RF và tránh kẹt kênh SoftAP
  startAttempt(millis());
}

void WifiManager::stop() {
  WiFi.disconnect(false, false);
  state_ = State::Idle;
  attempts_ = 0;
  clearDeviceConfig(config_);
}


void WifiManager::loop() {
  const uint32_t now = millis();
  const wl_status_t status = WiFi.status();

  if (state_ == State::Connected) {
    if (status != WL_CONNECTED) {
      Serial.printf("WIFI_DISCONNECTED status=%d (%s)\r\n", status, wifiStatusString(status));
      scheduleRetry(now, F("disconnected"));
    }
    return;
  }

  if (state_ == State::Connecting) {
    if (status == WL_CONNECTED &&
        WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
      state_ = State::Connected;
      lastError_ = "";
      Serial.printf("WIFI_CONNECTED ip=%s rssi=%d channel=%d\r\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI(), WiFi.channel());
      return;
    }
    if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL) {
      Serial.printf("WIFI_CONNECT_ERROR status=%d (%s)\r\n", status, wifiStatusString(status));
      scheduleRetry(now, F("connect_error"));
      return;
    }
    if (now - attemptStartedAt_ >= WIFI_CONNECT_TIMEOUT_MS) {
      Serial.printf("WIFI_CONNECT_TIMEOUT status=%d (%s)\r\n", status, wifiStatusString(status));
      scheduleRetry(now, F("connect_timeout"));
    }
    return;
  }

  if (state_ == State::RetryWait && deadlineReached(now, nextAttemptAt_)) {
    startAttempt(now);
  }
}

bool WifiManager::connected() const {
  return state_ == State::Connected && WiFi.status() == WL_CONNECTED;
}

bool WifiManager::failed() const {
  return state_ == State::Failed;
}

uint8_t WifiManager::attempts() const {
  return attempts_;
}

WifiManager::State WifiManager::state() const {
  return state_;
}

const String &WifiManager::lastError() const {
  return lastError_;
}

void WifiManager::startAttempt(uint32_t now) {
  ++attempts_;
  state_ = State::Connecting;
  attemptStartedAt_ = now;

  // Tự động dò tìm kênh (Channel) và BSSID của mạng đích để tránh lệch kênh với SoftAP
  int targetChannel = 0;
  uint8_t targetBssid[6];
  bool foundBssid = false;

  const int n = WiFi.scanNetworks(false, false);
  for (int i = 0; i < n; ++i) {
    if (WiFi.SSID(i) == config_.wifiSsid) {
      targetChannel = WiFi.channel(i);
      memcpy(targetBssid, WiFi.BSSID(i), 6);
      foundBssid = true;
      Serial.printf("[WIFI TARGET] Tìm thấy '%s' tại Kênh CH%d (RSSI: %d dBm | BSSID: %s)\r\n",
                    config_.wifiSsid.c_str(), targetChannel, WiFi.RSSI(i), WiFi.BSSIDstr(i).c_str());
      break;
    }
  }
  WiFi.scanDelete();

  Serial.printf("WIFI_CONNECTING ssid=\"%s\" pass_len=%u attempt=%u target_channel=%d\r\n",
                config_.wifiSsid.c_str(),
                static_cast<unsigned>(config_.wifiPassword.length()),
                attempts_,
                targetChannel);

  wl_status_t status;
  if (targetChannel > 0 && foundBssid) {
    status = WiFi.begin(config_.wifiSsid.c_str(), config_.wifiPassword.c_str(), targetChannel, targetBssid);
  } else if (targetChannel > 0) {
    status = WiFi.begin(config_.wifiSsid.c_str(), config_.wifiPassword.c_str(), targetChannel);
  } else {
    status = WiFi.begin(config_.wifiSsid.c_str(), config_.wifiPassword.c_str());
  }

  if (status == WL_CONNECT_FAILED) {
    Serial.printf("WIFI_BEGIN_FAILED status=%d (%s)\r\n", status, wifiStatusString(status));
    scheduleRetry(now, F("connect_start_failed"));
  }
}


void WifiManager::scheduleRetry(uint32_t now,
                                const __FlashStringHelper *reason) {
  lastError_ = String(reason);
  WiFi.disconnect(false, false);
  if (finiteAttempts_ && attempts_ >= 2) {
    state_ = State::Failed;
    Serial.printf("WIFI_FAILED reason=%s attempts=%u\r\n",
                  lastError_.c_str(), attempts_);
    return;
  }
  const uint32_t delayMs = retryDelayMs();
  nextAttemptAt_ = now + delayMs;
  state_ = State::RetryWait;
  Serial.printf("WIFI_RETRY_IN_MS %lu reason=%s\r\n",
                static_cast<unsigned long>(delayMs), lastError_.c_str());
}

uint32_t WifiManager::retryDelayMs() const {
  static constexpr uint32_t delays[] = {1000, 2000, 4000, 8000,
                                         16000, 30000, 60000};
  const size_t index =
      min(static_cast<size_t>(attempts_ == 0 ? 0 : attempts_ - 1),
          sizeof(delays) / sizeof(delays[0]) - 1);
  return delays[index] + static_cast<uint32_t>(esp_random() % 501);
}

