#pragma once

// POC-only credential. Copy this file to secrets.h and replace the value when
// testing against a public server. secrets.h is ignored by Git.
constexpr char DEVICE_ID[] = "esp32-poc5";
constexpr char DEVICE_TOKEN[] = "poc-device-token-change-me";

// Wokwi-only shortcut. Wokwi's free tier has no Private IoT Gateway, so the
// SoftAP/portal provisioning step cannot be driven from a browser. When this
// flag is true AND no config is stored in NVS, the firmware skips the portal
// and connects straight to the simulated Wokwi-GUEST network (empty password)
// and the ngrok host below, so the STA -> WSS -> dashboard flow can be
// validated without a portal.
//
//  - Keep `true` while testing on Wokwi.
//  - Set back to `false` before flashing a REAL board, or an empty-NVS real
//    board will try to join the (nonexistent) Wokwi-GUEST instead of opening
//    its setup portal.
constexpr bool WOKWI_PRECONFIG_ENABLED = false;
constexpr char WOKWI_PRECONFIG_SSID[] = "Wokwi-GUEST";
constexpr char WOKWI_PRECONFIG_PASSWORD[] = "";
// Replace with your ngrok domain, e.g. "a1b2c3d4.ngrok-free.app"
// constexpr char WOKWI_PRECONFIG_SERVER_HOST[] = "sombrous-homomorphous-zavier.ngrok-free.dev";
constexpr char WOKWI_PRECONFIG_SERVER_HOST[] = "northwest-mysimon-printers-given.trycloudflare.com";
