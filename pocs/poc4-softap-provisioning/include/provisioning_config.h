#pragma once

#include <Arduino.h>

namespace provisioning_config {

constexpr char AP_SSID_PREFIX[] = "ESP32-SETUP-";
constexpr char AP_PASSWORD[] = "configure-me";
constexpr uint8_t AP_INITIAL_CHANNEL = 1;
constexpr uint8_t AP_MAX_CLIENTS = 2;

constexpr char NVS_NAMESPACE[] = "wifi_cfg";
constexpr char NVS_VALID_KEY[] = "valid";
constexpr char NVS_SSID_KEY[] = "ssid";
constexpr char NVS_PASSWORD_KEY[] = "pass";

constexpr char UPSTREAM_URL[] =
    "http://jsonplaceholder.typicode.com/todos/1";
constexpr uint32_t STA_CONNECT_TIMEOUT_MS = 20000;
constexpr uint32_t FALLBACK_DELAY_MS = 3000;
constexpr uint16_t UPSTREAM_TIMEOUT_MS = 5000;
constexpr size_t MAX_FORM_BODY_BYTES = 256;

}  // namespace provisioning_config
