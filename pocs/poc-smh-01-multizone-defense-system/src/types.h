#pragma once
#include <Arduino.h>

// ============================================================================
// CÁC KIỂU DỮ LIỆU & ĐỊNH NGHĨA TRẠNG THÁI CHO SMH-01
// ============================================================================

enum SystemState {
  STATE_INIT,         // Đang khởi động hệ thống, kiểm tra ngoại vi POST
  STATE_DISARMED,     // Hệ thống an toàn, không kích hoạt báo động
  STATE_EXIT_DELAY,   // Đếm ngược 15 giây rời khỏi nhà
  STATE_ARMED,        // Chế độ bảo vệ tuần tra 3 vùng
  STATE_ENTRY_DELAY,  // Đếm ngược 15 giây vào nhà
  STATE_ALARM         // Báo động toàn diện: Còi hú, Đèn pha, Strobe LED, Tin nhắn
};

enum SecurityZone {
  ZONE_NONE = 0,
  ZONE_1_PIR = 1,     // Vùng 1: Chuyển động hồng ngoại (Hành lang)
  ZONE_2_IR = 2,      // Vùng 2: Tia chắn hồng ngoại (Cửa sổ/Ban công)
  ZONE_3_SOUND = 3    // Vùng 3: Âm thanh vỡ kính / cậy cửa (Phòng khách)
};

struct SecurityLogEntry {
  uint32_t timestamp;      // Unix timestamp từ RTC (hoặc uptime nếu không có RTC)
  char timeStr[20];        // "YYYY-MM-DD HH:MM:SS"
  uint8_t zone;            // 1, 2, 3
  char triggerSource[16];  // "SENSOR", "CLI", "BUTTON", "RFID", "PIN"
  char description[32];    // Mô tả sự kiện
};

inline const char* getZoneName(SecurityZone zone) {
  switch (zone) {
    case ZONE_1_PIR:   return "Vung 1 (PIR - Hanh lang)";
    case ZONE_2_IR:    return "Vung 2 (IR - Cua so)";
    case ZONE_3_SOUND: return "Vung 3 (Am thanh - Phong khach)";
    default:           return "Khong xac dinh";
  }
}

inline const char* getStateName(SystemState state) {
  switch (state) {
    case STATE_INIT:        return "KHOI DONG (INIT)";
    case STATE_DISARMED:    return "AN TOAN (DISARMED)";
    case STATE_EXIT_DELAY:  return "TRE ROI NHA (EXIT DELAY)";
    case STATE_ARMED:       return "BAO VE (ARMED)";
    case STATE_ENTRY_DELAY: return "TRE VAO NHA (ENTRY DELAY)";
    case STATE_ALARM:       return "BAO DONG (ALARM BREACH)";
    default:                return "UNKNOWN";
  }
}
