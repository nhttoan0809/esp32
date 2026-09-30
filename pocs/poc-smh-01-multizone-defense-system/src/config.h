#pragma once
#include <Arduino.h>

// ============================================================================
// POC SMH-01: MULTI-ZONE DEFENSE SYSTEM CONFIGURATION
// Vi điều khiển mục tiêu: ESP32 DevKit V1 (30 chân)
// ============================================================================

// ----------------------------------------------------------------------------
// 1. NGOẠI VI CẢM BIẾN (INPUT-ONLY PINS AN TOÀN TRÊN ESP32 30-PIN)
// ----------------------------------------------------------------------------
#define PIN_ZONE1_PIR        34   // Vùng 1: PIR HC-SR501 (Tín hiệu 3.3V TTL active HIGH)
#define PIN_ZONE2_IR         35   // Vùng 2: LM393 IR Obstacle (Active LOW khi cắt tia)
#define PIN_ZONE3_SOUND      39   // Vùng 3: Sound Sensor HW-484 (Active LOW khi có tiếng động lớn)
#define PIN_BTN_ARM          13   // Nút bấm thao tác Arm/Disarm tại chỗ (INPUT_PULLUP)

// ----------------------------------------------------------------------------
// 2. BUS I2C (CHIA SẺ CHO OLED SSD1306 VÀ RTC DS3231/DS1307)
// ----------------------------------------------------------------------------
#define PIN_I2C_SDA          21
#define PIN_I2C_SCL          22
#define OLED_I2C_ADDR        0x3C
#define OLED_SCREEN_WIDTH    128
#define OLED_SCREEN_HEIGHT   64
#define OLED_RESET_PIN       -1
#define RTC_I2C_ADDR         0x68

// ----------------------------------------------------------------------------
// 3. BUS SPI (VSPI DÀNH CHO MODULE THẺ TỪ RFID RC522)
// ----------------------------------------------------------------------------
#define PIN_RFID_SCK         18
#define PIN_RFID_MISO        19
#define PIN_RFID_MOSI        23
#define PIN_RFID_SS          5    // SDA / CS trên module RC522
#define PIN_RFID_RST         4    // RST trên module RC522

// ----------------------------------------------------------------------------
// 4. CƠ CẤU CHẤP HÀNH & CẢNH BÁO (ACTUATORS & INDICATORS)
// ----------------------------------------------------------------------------
#define PIN_BUZZER           25   // Còi Piezo Buzzer (Hỗ trợ phát đa tần số Siren)
#define PIN_RELAY_FLOODLIGHT 26   // Relay Kênh 1: Bật đèn pha rọi sáng
#define PIN_RELAY_SIREN      27   // Relay Kênh 2: Bật còi hú công suất lớn ngoài trời
#define PIN_LED_ALARM        2    // LED Đỏ cảnh báo / ARMED / Strobe (Nối trở 220R)
#define PIN_LED_DISARMED     15   // LED Xanh báo hệ thống an toàn (Nối trở 220R)

// Mức logic kích hoạt Relay (Module relay optocoupler phổ biến active LOW)
#define RELAY_ACTIVE_LEVEL   LOW
#define RELAY_INACTIVE_LEVEL HIGH

// ----------------------------------------------------------------------------
// 5. THỜI GIAN ĐỊNH THÌ & BẢO MẬT (SYSTEM TIMINGS & CREDENTIALS)
// ----------------------------------------------------------------------------
#define EXIT_DELAY_MS        15000   // 15 giây đếm ngược rời nhà sau khi ARM
#define ENTRY_DELAY_MS       15000   // 15 giây đếm ngược vào nhà trước khi hú còi
#define ALARM_DURATION_MS    60000   // 60 giây còi hú liên tục trước khi tạm ngắt
#define SENSOR_DEBOUNCE_MS   80      // Lọc rung tiếp điểm cảm biến
#define MAX_LOG_ENTRIES      10      // Lưu tối đa 10 sự kiện gần nhất trong NVS

#define DEFAULT_PIN          "1234"
#define DEFAULT_MASTER_UID   "DE:AD:BE:EF" // Trùng khớp UID mặc định của Wokwi MFRC522

// ----------------------------------------------------------------------------
// 6. CẤU HÌNH WI-FI & TELEGRAM BOT
// ----------------------------------------------------------------------------
#if defined(WOKWI_SIMULATION)
  #define WIFI_SSID          "Wokwi-GUEST"
  #define WIFI_PASSWORD      ""
#else
  #define WIFI_SSID          "AILOGY"
  #define WIFI_PASSWORD      "12345678"
#endif

// Token mẫu cho Telegram Bot (Người dùng có thể thay thế trong triển khai thật)
#define TELEGRAM_BOT_TOKEN   "123456789:ABCdefGHIjklMNOpqrSTUvwxYZ"
#define TELEGRAM_CHAT_ID     "12345678"
