#include "rtc_manager.h"

bool RtcManager::begin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  if (_rtc.begin()) {
    _rtcAvailable = true;
    if (!_rtc.isrunning()) {
      Serial.println("[RTC] Dong ho chua duoc set gio, dat mac dinh...");
      _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    Serial.println("[RTC] Module RTC DS1307/DS3231 ket noi thanh cong!");
  } else {
    _rtcAvailable = false;
    Serial.println("[RTC] CANH BAO: Khong tim thay RTC phan cung. Chuyen sang che do gia lap thoi gian!");
  }

  loadLogsFromNvs();
  return _rtcAvailable;
}

uint32_t RtcManager::getUnixTime() {
  if (_rtcAvailable) {
    DateTime now = _rtc.now();
    return now.unixtime();
  }
  // Giả lập từ uptime nếu không có phần cứng RTC
  return 1790800000 + (millis() / 1000);
}

String RtcManager::getFormattedTime() {
  if (_rtcAvailable) {
    DateTime now = _rtc.now();
    char buf[24];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
    return String(buf);
  }

  // Fallback định dạng thời gian
  unsigned long totalSec = millis() / 1000;
  unsigned int h = (totalSec / 3600) % 24;
  unsigned int m = (totalSec / 60) % 60;
  unsigned int s = totalSec % 60;
  char buf[24];
  snprintf(buf, sizeof(buf), "2026-09-30 %02d:%02d:%02d", h, m, s);
  return String(buf);
}

void RtcManager::logEvent(SecurityZone zone, const char* source, const char* desc) {
  SecurityLogEntry entry;
  entry.timestamp = getUnixTime();
  strncpy(entry.timeStr, getFormattedTime().c_str(), sizeof(entry.timeStr) - 1);
  entry.timeStr[sizeof(entry.timeStr) - 1] = '\0';
  entry.zone = (uint8_t)zone;
  strncpy(entry.triggerSource, source, sizeof(entry.triggerSource) - 1);
  entry.triggerSource[sizeof(entry.triggerSource) - 1] = '\0';
  strncpy(entry.description, desc, sizeof(entry.description) - 1);
  entry.description[sizeof(entry.description) - 1] = '\0';

  // Lưu vào ring buffer trong RAM
  _logs[_logHead] = entry;
  _logHead = (_logHead + 1) % MAX_LOG_ENTRIES;
  if (_logCount < MAX_LOG_ENTRIES) {
    _logCount++;
  }

  // Đồng bộ lưu vào NVS Flash
  saveLogsToNvs();

  Serial.printf("[AUDIT LOG] [%s] %s | Nguon: %s | Chi tiet: %s\n",
                entry.timeStr, getZoneName(zone), entry.triggerSource, entry.description);
}

int RtcManager::getLogCount() const {
  return _logCount;
}

SecurityLogEntry RtcManager::getLog(int index) const {
  if (index < 0 || index >= _logCount) {
    SecurityLogEntry empty;
    memset(&empty, 0, sizeof(empty));
    return empty;
  }
  // Duyệt từ mới nhất đến cũ nhất
  int actualIdx = (_logHead - 1 - index + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
  return _logs[actualIdx];
}

void RtcManager::printAllLogs() {
  Serial.println("==================================================");
  Serial.printf("   NHAT KY SU KIEN AN NINH (Tong: %d ban ghi)\n", _logCount);
  Serial.println("==================================================");
  if (_logCount == 0) {
    Serial.println("  (Chua co su kien vi pham nao duoc ghi nhan)");
  } else {
    for (int i = 0; i < _logCount; i++) {
      SecurityLogEntry e = getLog(i);
      Serial.printf(" #%d | %s | %s | %s\n",
                    i + 1, e.timeStr, getZoneName((SecurityZone)e.zone), e.description);
    }
  }
  Serial.println("==================================================");
}

void RtcManager::clearLogs() {
  _logCount = 0;
  _logHead = 0;
  saveLogsToNvs();
  Serial.println("[NVS] Da xoa toan bo nhat ky an ninh!");
}

void RtcManager::loadLogsFromNvs() {
  _prefs.begin("smh_logs", true);
  _logCount = _prefs.getInt("count", 0);
  _logHead = _prefs.getInt("head", 0);

  if (_logCount > MAX_LOG_ENTRIES) _logCount = MAX_LOG_ENTRIES;
  if (_logHead >= MAX_LOG_ENTRIES) _logHead = 0;

  for (int i = 0; i < _logCount; i++) {
    char key[16];
    snprintf(key, sizeof(key), "log_%d", i);
    size_t len = _prefs.getBytes(key, &_logs[i], sizeof(SecurityLogEntry));
    if (len != sizeof(SecurityLogEntry)) {
      memset(&_logs[i], 0, sizeof(SecurityLogEntry));
    }
  }
  _prefs.end();
  Serial.printf("[NVS] Da tai %d nhat ky su kien tu bo nho Flash\n", _logCount);
}

void RtcManager::saveLogsToNvs() {
  _prefs.begin("smh_logs", false);
  _prefs.putInt("count", _logCount);
  _prefs.putInt("head", _logHead);

  for (int i = 0; i < _logCount; i++) {
    char key[16];
    snprintf(key, sizeof(key), "log_%d", i);
    _prefs.putBytes(key, &_logs[i], sizeof(SecurityLogEntry));
  }
  _prefs.end();
}
