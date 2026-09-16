#pragma once

#include <Arduino.h>

namespace app_config {

// Status LEDs
constexpr uint8_t LED_SETUP_PIN = 18;
constexpr uint8_t LED_WIFI_PIN = 21;
constexpr uint8_t LED_SERVER_PIN = 22;

// Output Relay (Active LOW: 5V Relay IN1, LOW = Coil ON / COM-NO Closed, HIGH = OFF)
constexpr uint8_t RELAY_PIN = 26;
// Status LED representing real lamp on breadboard (Active HIGH: HIGH = ON, LOW = OFF)
constexpr uint8_t REAL_DEVICE_PIN = 23;

// Manual Lamp Button (INPUT_PULLUP)
constexpr uint8_t LAMP_BUTTON_PIN = 14;  // Primary lamp button pin
constexpr uint8_t ALT_BUTTON_PIN = 25;   // Alternate button pin (compatible with POC5 breadboard setup)

// SoftAP Provisioning
constexpr char AP_SSID_PREFIX[] = "ESP32-LAMP-";
// Empty string = Open Network (no password) - Industry standard for IoT setup (Tuya, Shelly, Sonoff).
// Eliminates WPA2 4-way handshake failures, PMF conflicts on macOS/iOS, and keyboard typo/auto-capitalization bugs.
constexpr char AP_PASSWORD[] = "configure-me";
constexpr uint8_t AP_INITIAL_CHANNEL = 1;
constexpr uint8_t AP_MAX_CLIENTS = 2;

// NVS Storage
constexpr char NVS_NAMESPACE[] = "lamp_cfg";
constexpr char NVS_VALID_KEY[] = "valid";
constexpr char NVS_SSID_KEY[] = "ssid";
constexpr char NVS_PASSWORD_KEY[] = "pass";
constexpr char NVS_SERVER_HOST_KEY[] = "host";
constexpr char NVS_SERVER_PORT_KEY[] = "port";
constexpr char NVS_SERVER_PATH_KEY[] = "path";

// Networking & WebSocket Timing
constexpr uint16_t HTTPS_PORT = 443;
constexpr char DEFAULT_SERVER_PATH[] = "/ws/devices";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
constexpr uint32_t TIME_SYNC_TIMEOUT_MS = 15000;
constexpr uint32_t APP_HELLO_TIMEOUT_MS = 5000;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 40;
constexpr uint32_t LAMP_FORCE_OFF_HOLD_MS = 3000;
constexpr uint32_t BUTTON_FACTORY_RESET_MS = 10000;  // Hold >= 10s to wipe Wi-Fi credentials & reset
constexpr size_t MAX_WEBSOCKET_MESSAGE_BYTES = 512;
constexpr uint32_t WEBSOCKET_PING_INTERVAL_MS = 20000;

}  // namespace app_config
