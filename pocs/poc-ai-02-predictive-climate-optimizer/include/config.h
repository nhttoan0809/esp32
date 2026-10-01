#pragma once

#include <Arduino.h>

// ==========================================
// ĐỊNH NGHĨA CHÂN NGOẠI VI (PINOUT DEFINITIONS)
// Tuân thủ nghiêm ngặt ESP32 DevKit V1 (30 chân) & AGENTS.md
// ==========================================

// 1. Cảm biến nhiệt ẩm DHT11 / DHT22 (1-Wire)
#define PIN_DHT11           19    // GPIO 19: Single-bus data DHT

#if defined(WOKWI_SIMULATION)
  #define DHT_TYPE          DHT22 // Mô phỏng Wokwi dùng wokwi-dht22
#else
  #define DHT_TYPE          DHT11 // Phần cứng thật dùng DHT11 Type A
#endif

// 2. Tuyến truyền thông I2C dùng chung cho OLED SSD1306 và RTC DS3231/DS1307
#define PIN_I2C_SDA         21    // GPIO 21: SDA
#define PIN_I2C_SCL         22    // GPIO 22: SCL
#define OLED_I2C_ADDRESS    0x3C  // Màn hình OLED SSD1306 0.96 inch
#define RTC_I2C_ADDRESS     0x68  // Đồng hồ thời gian thực DS1307 / DS3231

// 3. Cảm biến quang trở LDR LM393
#define PIN_LDR_DO          35    // GPIO 35: Digital Out từ LM393 (Input-Only, ADC1)
#define PIN_LDR_AO          34    // GPIO 34: Analog Out (ADC1, đo mức sáng 0-4095)

// 4. Module Relay 2 Kênh (Active LOW)
#define PIN_RELAY1_FAN      26    // GPIO 26: Kích mở Quạt làm mát (IN1)
#define PIN_RELAY2_HEAT     27    // GPIO 27: Kích mở Sưởi / Phụ trợ (IN2)
#define RELAY_ACTIVE_LEVEL  LOW   // Active LOW: LOW = ĐÓNG RƠ-LE, HIGH = NGẮT
#define RELAY_INACTIVE_LEVEL HIGH

// 5. Nút bấm tương tác & chuyển đổi chế độ
#define PIN_BUTTON_PAGE     25    // GPIO 25: Nút chuyển trang OLED / Manual (INPUT_PULLUP)

// 6. Đèn LED chỉ thị trực quan (qua trở 220Ω)
#define PIN_LED_COMFORT     23    // GPIO 23: LED Xanh lá (Comfort Zone / Cloud Connected)
#define PIN_LED_WARN        18    // GPIO 18: LED Đỏ/Vàng (AI Pre-cooling Active / Alert)

// ==========================================
// CẤU HÌNH THỜI GIAN VÀ CHU KỲ (TIMING CONSTANTS)
// ==========================================
#define SENSOR_READ_INTERVAL_MS   2000   // Chu kỳ đọc cảm biến: 2.0s
#define TELEMETRY_INTERVAL_MS     5000   // Chu kỳ gửi dữ liệu lên AI Server: 5.0s
#define OLED_REFRESH_INTERVAL_MS  1000   // Chu kỳ cập nhật màn hình OLED: 1.0s
#define FAILSAFE_TIMEOUT_MS       20000  // Quá 20s không có phản hồi AI -> Chuyển Fallback
#define ANTI_RAPID_CYCLE_MS       15000  // Tối thiểu 15s giữa 2 lần đóng/ngắt Relay liên tiếp

// ==========================================
// CẤU HÌNH CLOUD & WEBSOCKET
// ==========================================
#define DEFAULT_DEVICE_ID         "esp32-ai02"
#define DEFAULT_WIFI_SSID         "Wokwi-GUEST"
#define DEFAULT_WIFI_PASS         ""

#if defined(WOKWI_SIMULATION)
  // Wokwi Public Gateway nối về localhost server của máy host
  #define DEFAULT_WS_HOST         "host.wokwi.internal"
  #define DEFAULT_WS_PORT         8000
#else
  // Cấu hình mạng LAN hoặc Domain Cloudflare/ngrok khi chạy board thật
  #define DEFAULT_WS_HOST         "192.168.1.100"
  #define DEFAULT_WS_PORT         8000
#endif

#define DEFAULT_WS_PATH           "/ws/climate/esp32-ai02"
