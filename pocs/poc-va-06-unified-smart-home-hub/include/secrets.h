#pragma once

constexpr char DEVICE_ID[] = "esp32-smart-home-hub";
constexpr char DEVICE_TOKEN[] = "hub-secret-token";

#ifdef WOKWI_SIMULATION
constexpr bool WOKWI_PRECONFIG_ENABLED = true;
constexpr char WOKWI_PRECONFIG_SSID[] = "Wokwi-GUEST";
constexpr char WOKWI_PRECONFIG_PASSWORD[] = "";
#else
constexpr bool WOKWI_PRECONFIG_ENABLED = false;
constexpr char WOKWI_PRECONFIG_SSID[] = "";
constexpr char WOKWI_PRECONFIG_PASSWORD[] = "";
#endif
constexpr char WOKWI_PRECONFIG_SERVER_HOST[] = "displaying-profession-newbie-welding.trycloudflare.com";
constexpr uint16_t WOKWI_PRECONFIG_SERVER_PORT = 443;

