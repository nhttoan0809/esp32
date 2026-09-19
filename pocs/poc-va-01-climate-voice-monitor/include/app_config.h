#pragma once

#include <Arduino.h>

namespace app_config {

// GPIO Pinout ESP32 DevKit V1 (30-pin)
constexpr uint8_t PIN_DHT_DATA = 19;     // DHT11 Data pin
constexpr uint8_t PIN_LED_COMFORT = 15;  // Green LED: Comfort state
constexpr uint8_t PIN_LED_ALERT = 13;    // Red LED: Alert state (high temp/humidity)
constexpr uint8_t PIN_OLED_SDA = 21;    // I2C SDA for SSD1306
constexpr uint8_t PIN_OLED_SCL = 22;    // I2C SCL for SSD1306

// OLED Configuration
constexpr uint8_t OLED_I2C_ADDR = 0x3C;
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;

// Climate Comfort Thresholds
constexpr float TEMP_COMFORT_MIN = 20.0f;
constexpr float TEMP_COMFORT_MAX = 28.0f;
constexpr float HUMID_COMFORT_MIN = 40.0f;
constexpr float HUMID_COMFORT_MAX = 70.0f;

constexpr float TEMP_ALERT_THRESHOLD = 32.0f;
constexpr float HUMID_ALERT_THRESHOLD = 75.0f;

// Operational Timings (milliseconds)
constexpr uint32_t SENSOR_READ_INTERVAL_MS = 2000;
constexpr uint32_t CLOUD_REPORT_INTERVAL_MS = 5000;
constexpr uint32_t DISPLAY_UPDATE_INTERVAL_MS = 500;

// Network & WebSocket
constexpr uint16_t HTTP_PORT = 80;
constexpr uint16_t HTTPS_PORT = 443;
constexpr size_t MAX_WEBSOCKET_MESSAGE_BYTES = 2048;

}  // namespace app_config
