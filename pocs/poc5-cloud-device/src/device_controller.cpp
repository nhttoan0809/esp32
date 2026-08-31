#include "device_controller.h"

#include "app_config.h"

using namespace app_config;

namespace {

void prepareOutput(uint8_t pin) {
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
}

}  // namespace

void DeviceController::begin() {
  prepareOutput(LED_SETUP_PIN);
  prepareOutput(LED_CLIENT_PIN);
  prepareOutput(LED_WIFI_PIN);
  prepareOutput(LED_SERVER_PIN);
  prepareOutput(REAL_DEVICE_PIN);
  pinMode(SETUP_BUTTON_PIN, INPUT_PULLUP);
  setupReady_ = false;
  clientConnected_ = false;
  wifiConnected_ = false;
  serverConnected_ = false;
  realDeviceOn_ = false;
}

void DeviceController::setSetupReady(bool ready) {
  if (setupReady_ != ready) {
    setupReady_ = ready;
    digitalWrite(LED_SETUP_PIN, ready ? HIGH : LOW);
    Serial.printf("[LED] 🟡 GPIO%u (Setup Portal): %s\r\n", LED_SETUP_PIN,
                  ready ? "ON" : "OFF");
  }
}

void DeviceController::setProvisioningClientConnected(bool connected) {
  if (clientConnected_ != connected) {
    clientConnected_ = connected;
    digitalWrite(LED_CLIENT_PIN, connected ? HIGH : LOW);
    Serial.printf("[LED] 🔵 GPIO%u (Client Joined SoftAP): %s\r\n",
                  LED_CLIENT_PIN, connected ? "ON" : "OFF");
  }
}

void DeviceController::setWifiConnected(bool connected) {
  if (wifiConnected_ != connected) {
    wifiConnected_ = connected;
    digitalWrite(LED_WIFI_PIN, connected ? HIGH : LOW);
    Serial.printf("[LED] 🟢 GPIO%u (Wi-Fi STA Connected): %s\r\n",
                  LED_WIFI_PIN, connected ? "ON" : "OFF");
  }
}

void DeviceController::setServerConnected(bool connected) {
  if (serverConnected_ != connected) {
    serverConnected_ = connected;
    digitalWrite(LED_SERVER_PIN, connected ? HIGH : LOW);
    Serial.printf("[LED] ⚪ GPIO%u (Cloud WSS Ready): %s\r\n", LED_SERVER_PIN,
                  connected ? "ON" : "OFF");
  }
}

void DeviceController::setRealDevice(bool on) {
  if (realDeviceOn_ != on) {
    realDeviceOn_ = on;
    digitalWrite(REAL_DEVICE_PIN, on ? HIGH : LOW);
    Serial.printf("[LED] 🔴 GPIO%u (Real_Device Relay): %s\r\n", REAL_DEVICE_PIN,
                  on ? "ON" : "OFF");
  }
}

bool DeviceController::realDeviceOn() const {
  return realDeviceOn_;
}

