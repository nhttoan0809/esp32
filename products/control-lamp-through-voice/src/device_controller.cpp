#include "device_controller.h"

#include "app_config.h"

using namespace app_config;

namespace {

void prepareOutput(uint8_t pin, uint8_t initialLevel) {
  digitalWrite(pin, initialLevel);
  pinMode(pin, OUTPUT);
}

}  // namespace

void DeviceController::begin() {
  prepareOutput(LED_SETUP_PIN, LOW);
  prepareOutput(LED_WIFI_PIN, LOW);
  prepareOutput(LED_SERVER_PIN, LOW);

  // Real Device (Active HIGH LED indicator) starts OFF
  prepareOutput(REAL_DEVICE_PIN, LOW);

  // Relay Module (Active LOW: HIGH = Coil de-energized = OFF) starts OFF
  prepareOutput(RELAY_PIN, HIGH);

  pinMode(LAMP_BUTTON_PIN, INPUT_PULLUP);

  setupReady_ = false;
  wifiConnected_ = false;
  serverConnected_ = false;
  realDeviceOn_ = false;

  Serial.println(F("[DEVICE] ⚡ Hardware controller initialized (Fail-Safe OFF)"));
}

void DeviceController::setSetupReady(bool ready) {
  if (setupReady_ != ready) {
    setupReady_ = ready;
    digitalWrite(LED_SETUP_PIN, ready ? HIGH : LOW);
    Serial.printf("[LED] 🟡 GPIO%u (Setup Portal): %s\r\n", LED_SETUP_PIN,
                  ready ? "ON" : "OFF");
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

    // LED (Active HIGH): HIGH = ON
    digitalWrite(REAL_DEVICE_PIN, on ? HIGH : LOW);

    // Relay (Active LOW): LOW = ON, HIGH = OFF
    digitalWrite(RELAY_PIN, on ? LOW : HIGH);

    Serial.printf("[LAMP] 💡 State: %s (Relay=GPIO%u %s, LED=GPIO%u %s)\r\n",
                  on ? "ON" : "OFF",
                  RELAY_PIN, on ? "LOW" : "HIGH",
                  REAL_DEVICE_PIN, on ? "HIGH" : "LOW");
  }
}

bool DeviceController::toggleRealDevice() {
  setRealDevice(!realDeviceOn_);
  return realDeviceOn_;
}

void DeviceController::forceOff() {
  Serial.println(F("[SAFETY] ⚠️ Force OFF triggered!"));
  setRealDevice(false);
}

bool DeviceController::realDeviceOn() const {
  return realDeviceOn_;
}
