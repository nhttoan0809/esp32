#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==========================================
// PINOUT MAPPING: ESP32 DevKit V1 (30-Pin)
// ==========================================

// RFID RC522 (VSPI Bus)
#define PIN_RFID_SS        5   // VSPI CS
#define PIN_RFID_RST       4   // Reset
#define PIN_RFID_SCK       18  // VSPI SCK
#define PIN_RFID_MISO      19  // VSPI MISO
#define PIN_RFID_MOSI      23  // VSPI MOSI

// I2C Bus (Shared between OLED SSD1306 & RTC DS3231/DS1307)
#define PIN_I2C_SDA        21
#define PIN_I2C_SCL        22
#define OLED_I2C_ADDR      0x3C
#define OLED_SCREEN_WIDTH  128
#define OLED_SCREEN_HEIGHT 64
#define RTC_I2C_ADDR       0x68

// Keypad 4x4 Matrix
#define PIN_KEYPAD_R1      14
#define PIN_KEYPAD_R2      27
#define PIN_KEYPAD_R3      16
#define PIN_KEYPAD_R4      17
#define PIN_KEYPAD_C1      26
#define PIN_KEYPAD_C2      25
#define PIN_KEYPAD_C3      33
#define PIN_KEYPAD_C4      32

// Actuator & Feedback Peripherals
#define PIN_SERVO          13  // SG90 Servo PWM Signal
#define PIN_BUZZER         15  // Active Buzzer Positive Pin
#define PIN_LED_UNLOCK     2   // Unlock Status LED (also Onboard LED)

// ==========================================
// SYSTEM OPERATING PARAMETERS
// ==========================================

// Servo Mechanical Lock Positions
#define SERVO_POS_LOCKED       0    // 0 degrees: Locked state
#define SERVO_POS_UNLOCKED     90   // 90 degrees: Unlocked state

// Timings (Milliseconds)
#define AUTO_RELOCK_DELAY_MS        5000   // Auto-relock after 5 seconds
#define PIN_INPUT_TIMEOUT_MS        10000  // Reset PIN input after 10s of inactivity
#define ACCESS_DENIED_DISPLAY_MS    2000   // Show denied message for 2 seconds
#define LOCKOUT_DURATION_MS         60000  // Lockout penalty: 60 seconds
#define MAX_FAILED_ATTEMPTS         5      // Max failed attempts before lockout

// Security Defaults
#define DEFAULT_USER_PIN            "1234"
#define DEFAULT_ADMIN_PASSWORD      "admin123"
#define DEFAULT_MASTER_CARD_UID     "DEADBEEF"

// Wi-Fi Configuration
#ifdef WOKWI_SIMULATION
#define DEFAULT_WIFI_SSID           "Wokwi-GUEST"
#define DEFAULT_WIFI_PASS           ""
#else
#define DEFAULT_WIFI_SSID           "Your_Home_WiFi"
#define DEFAULT_WIFI_PASS           "Your_Password"
#endif

#define AP_SSID                     "SMH02-SmartLock-AP"
#define AP_PASS                     "12345678"
#define WEB_SERVER_PORT             80

#endif // CONFIG_H
