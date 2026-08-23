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
  realDeviceOn_ = false;
}

void DeviceController::setSetupReady(bool ready) {
  digitalWrite(LED_SETUP_PIN, ready ? HIGH : LOW);
}

void DeviceController::setProvisioningClientConnected(bool connected) {
  digitalWrite(LED_CLIENT_PIN, connected ? HIGH : LOW);
}

void DeviceController::setWifiConnected(bool connected) {
  digitalWrite(LED_WIFI_PIN, connected ? HIGH : LOW);
}

void DeviceController::setServerConnected(bool connected) {
  digitalWrite(LED_SERVER_PIN, connected ? HIGH : LOW);
}

void DeviceController::setRealDevice(bool on) {
  digitalWrite(REAL_DEVICE_PIN, on ? HIGH : LOW);
  realDeviceOn_ = on;
}

bool DeviceController::realDeviceOn() const {
  return realDeviceOn_;
}
