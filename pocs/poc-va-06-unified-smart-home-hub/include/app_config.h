#pragma once

#include <Arduino.h>

namespace app_config {

// =============================================================================
// GPIO PINOUT MAP — ESP32 DevKit V1 (30-pin)
// =============================================================================
constexpr uint8_t PIN_BTN_MODE    = 4;   // Button 1: Mode / OLED page toggle
constexpr uint8_t PIN_RELAY       = 5;   // Relay IN1 (Active LOW optocoupler)
constexpr uint8_t PIN_LED_ALERT   = 13;  // Red LED: Security alarm / High heat
constexpr uint8_t PIN_BTN_LAMP    = 14;  // Button 2: Manual relay lamp toggle
constexpr uint8_t PIN_LED_COMFORT = 15;  // Green LED: Comfort condition
constexpr uint8_t PIN_LED_DIMMER  = 18;  // White/Yellow LED: PWM Dimmer
constexpr uint8_t PIN_DHT_DATA    = 19;  // DHT11 1-Wire Data pin
constexpr uint8_t PIN_OLED_SDA    = 21;  // OLED I2C SDA
constexpr uint8_t PIN_OLED_SCL    = 22;  // OLED I2C SCL
constexpr uint8_t PIN_LED_CLOUD   = 23;  // Blue LED: Cloud Online status
constexpr uint8_t PIN_BUZZER      = 27;  // Security alarm buzzer
constexpr uint8_t PIN_PIR_MOTION  = 33;  // PIR HC-SR501 Motion sensor OUT
constexpr uint8_t PIN_POT_ADC     = 34;  // Potentiometer (ADC1_CHANNEL_6)
constexpr uint8_t PIN_LDR_DO      = 35;  // LDR Light sensor DO (Input-Only)

// =============================================================================
// OLED DISPLAY SETTINGS
// =============================================================================
constexpr uint8_t OLED_I2C_ADDR   = 0x3C;
constexpr uint8_t SCREEN_WIDTH    = 128;
constexpr uint8_t SCREEN_HEIGHT   = 64;

// =============================================================================
// PWM LEDC TIMER SETTINGS
// =============================================================================
constexpr uint8_t LEDC_CHANNEL    = 0;
constexpr uint32_t LEDC_FREQ_HZ   = 5000;
constexpr uint8_t LEDC_RES_BITS   = 8;   // 0 - 255

// =============================================================================
// CLIMATE THRESHOLDS
// =============================================================================
constexpr float TEMP_COMFORT_MIN  = 20.0f;
constexpr float TEMP_COMFORT_MAX  = 28.0f;
constexpr float HUMID_COMFORT_MIN = 40.0f;
constexpr float HUMID_COMFORT_MAX = 70.0f;
constexpr float TEMP_ALERT_HOT    = 32.0f;
constexpr float HUMID_ALERT_HIGH  = 75.0f;

// =============================================================================
// TIMING INTERVALS (milliseconds)
// =============================================================================
constexpr uint32_t SENSOR_DHT_INTERVAL_MS   = 2000;
constexpr uint32_t SENSOR_LIGHT_INTERVAL_MS = 1000;
constexpr uint32_t SENSOR_ADC_INTERVAL_MS   = 100;
constexpr uint32_t CLOUD_REPORT_INTERVAL_MS = 5000;
constexpr uint32_t DISPLAY_REFRESH_MS       = 250;
constexpr uint32_t DEBOUNCE_DELAY_MS        = 50;

// Network & WebSocket
constexpr uint16_t HTTP_PORT = 80;
constexpr uint16_t HTTPS_PORT = 443;
constexpr size_t MAX_WEBSOCKET_MESSAGE_BYTES = 2048;

}  // namespace app_config
