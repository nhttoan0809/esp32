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

  // Relay Module (Active LOW: HIGH = Coil de-energized = COM-NO Open = Lamp OFF)
  prepareOutput(RELAY_PIN, HIGH);

  // Status LED representing real lamp on breadboard (Active HIGH: LOW = OFF, HIGH = ON)
  prepareOutput(REAL_DEVICE_PIN, LOW);

  // Configure button inputs with internal pull-up
  pinMode(LAMP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ALT_BUTTON_PIN, INPUT_PULLUP);

  setupReady_ = false;
  wifiConnected_ = false;
  serverConnected_ = false;
  realDeviceOn_ = false;

  Serial.println(F("[DEVICE] ⚡ Hardware controller initialized (Relay Fail-Safe OFF, Status LED OFF)"));
  Serial.printf("[DEVICE] 🔘 Lamp Button listening on GPIO%u (primary) and GPIO%u (alternate)\r\n",
                LAMP_BUTTON_PIN, ALT_BUTTON_PIN);
  Serial.printf("[DEVICE] 💡 Output driving Relay GPIO%u (Active LOW) & Indicator LED GPIO%u (Active HIGH)\r\n",
                RELAY_PIN, REAL_DEVICE_PIN);
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

    // Relay (Active LOW): LOW = Coil energized (COM-NO Closed), HIGH = Coil off (COM-NO Open)
    digitalWrite(RELAY_PIN, on ? LOW : HIGH);

    // Indicator LED (Active HIGH): HIGH = ON, LOW = OFF
    digitalWrite(REAL_DEVICE_PIN, on ? HIGH : LOW);

    Serial.printf("[RELAY] ⚡ Coil: %s (GPIO%u %s) | Status LED (GPIO%u %s) -> Lamp: %s\r\n",
                  on ? "ENERGIZED" : "DE-ENERGIZED",
                  RELAY_PIN, on ? "LOW" : "HIGH",
                  REAL_DEVICE_PIN, on ? "HIGH" : "LOW",
                  on ? "ON" : "OFF");
  }
}

bool DeviceController::toggleRealDevice() {
  setRealDevice(!realDeviceOn_);
  return realDeviceOn_;
}

void DeviceController::forceOff() {
  Serial.println(F("[SAFETY] ⚠️ Force OFF triggered! Opening Relay NO contact"));
  setRealDevice(false);
}

bool DeviceController::realDeviceOn() const {
  return realDeviceOn_;
}
