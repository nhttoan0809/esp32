#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>

namespace {

constexpr char DEVICE_NAME[] = "ESP32-POC2-BLE";
constexpr char SERVICE_UUID[] = "3b8e21c0-56d6-4d4e-9c5b-1eb12a20c002";
constexpr char CHARACTERISTIC_UUID[] = "3b8e21c1-56d6-4d4e-9c5b-1eb12a20c002";

BLEServer *bleServer = nullptr;
volatile bool advertisingRestartPending = false;

class ServerCallbacks final : public BLEServerCallbacks {
  void onConnect(BLEServer *server,
                 esp_ble_gatts_cb_param_t *params) override {
    // Arduino-ESP32 2.0.17 updates its internal count after this callback.
    const uint32_t clients = server->getConnectedCount() + 1;
    const uint8_t *peer = params->connect.remote_bda;

    Serial.printf(
        "BLE_CONNECTED peer=%02X:%02X:%02X:%02X:%02X:%02X conn_id=%u "
        "addr_type=%u clients=%lu\n",
        peer[0], peer[1], peer[2], peer[3], peer[4], peer[5],
        static_cast<unsigned int>(params->connect.conn_id),
        static_cast<unsigned int>(params->connect.ble_addr_type),
        static_cast<unsigned long>(clients));
  }

  void onDisconnect(BLEServer *server,
                    esp_ble_gatts_cb_param_t *params) override {
    // Arduino-ESP32 2.0.17 decrements its internal count after this callback.
    const uint32_t countBeforeCallback = server->getConnectedCount();
    const uint32_t clients =
        countBeforeCallback > 0 ? countBeforeCallback - 1 : 0;

    Serial.printf("BLE_DISCONNECTED conn_id=%u reason=%u clients=%lu\n",
                  static_cast<unsigned int>(params->disconnect.conn_id),
                  static_cast<unsigned int>(params->disconnect.reason),
                  static_cast<unsigned long>(clients));
    advertisingRestartPending = true;
  }
};

ServerCallbacks serverCallbacks;

} // namespace

void setup() {
  Serial.begin(115200);

  BLEDevice::init(DEVICE_NAME);
  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(&serverCallbacks);

  BLEService *service = bleServer->createService(SERVICE_UUID);
  BLECharacteristic *statusCharacteristic = service->createCharacteristic(
      CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ);
  statusCharacteristic->setValue("ready");
  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.printf("BLE_READY name=%s service=%s\n", DEVICE_NAME, SERVICE_UUID);
  Serial.println("BLE_ADVERTISING");
}

void loop() {
  if (advertisingRestartPending) {
    // Give Bluedroid time to finish disconnect processing before advertising.
    delay(500);
    advertisingRestartPending = false;
    bleServer->startAdvertising();
    Serial.println("BLE_ADVERTISING_RESTARTED");
  }

  delay(10);
}
