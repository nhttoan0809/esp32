#pragma once

#include <Arduino.h>

namespace app_config {

constexpr uint8_t LED_SETUP_PIN = 18;
constexpr uint8_t LED_CLIENT_PIN = 19;
constexpr uint8_t LED_WIFI_PIN = 21;
constexpr uint8_t LED_SERVER_PIN = 22;
constexpr uint8_t REAL_DEVICE_PIN = 23;
constexpr uint8_t SETUP_BUTTON_PIN = 25;

constexpr char AP_SSID_PREFIX[] = "ESP32-SETUP-";
constexpr char AP_PASSWORD[] = "configure-me";
constexpr uint8_t AP_INITIAL_CHANNEL = 1;
constexpr uint8_t AP_MAX_CLIENTS = 2;

constexpr char NVS_NAMESPACE[] = "poc5_cfg";
constexpr char NVS_VALID_KEY[] = "valid";
constexpr char NVS_SSID_KEY[] = "ssid";
constexpr char NVS_PASSWORD_KEY[] = "pass";
constexpr char NVS_SERVER_HOST_KEY[] = "host";
constexpr char NVS_SERVER_PORT_KEY[] = "port";
constexpr char NVS_SERVER_PATH_KEY[] = "path";

constexpr uint16_t HTTPS_PORT = 443;
constexpr char DEFAULT_SERVER_PATH[] = "/ws/devices";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
constexpr uint32_t TIME_SYNC_TIMEOUT_MS = 15000;
constexpr uint32_t APP_HELLO_TIMEOUT_MS = 5000;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 40;
constexpr uint32_t BUTTON_FACTORY_RESET_MS = 5000;
constexpr size_t MAX_WEBSOCKET_MESSAGE_BYTES = 512;
constexpr uint32_t WEBSOCKET_PING_INTERVAL_MS = 20000;

}  // namespace app_config
