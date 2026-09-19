#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <DHT.h>

#include "app_config.h"
#include "device_config.h"
#include "secrets.h"
#include "display_manager.h"
#include "cloud_client.h"

using namespace app_config;

// Hardware-First Dual-Target Rule (Rule 5):
// Real hardware uses DHT11 in starter kit.
// Wokwi simulation uses DHT22 virtual chip (wokwi-dht22).
#ifdef WOKWI_SIMULATION
#define DHT_TYPE DHT22
#else
#define DHT_TYPE DHT11
#endif

// Global Peripheral Instances
DHT dht(PIN_DHT_DATA, DHT_TYPE);
DisplayManager displayManager;
CloudClient cloudClient;
DeviceConfig deviceConfig;

// State Tracking
float currentTemperature = NAN;
float currentHumidity = NAN;
uint32_t lastSensorReadAt = 0;
uint32_t lastCloudReportAt = 0;

void setupPins() {
  pinMode(PIN_LED_COMFORT, OUTPUT);
  pinMode(PIN_LED_ALERT, OUTPUT);
  digitalWrite(PIN_LED_COMFORT, LOW);
  digitalWrite(PIN_LED_ALERT, LOW);
}

void updateLeds(float temp, float humid) {
  if (isnan(temp) || isnan(humid)) {
    digitalWrite(PIN_LED_COMFORT, LOW);
    digitalWrite(PIN_LED_ALERT, LOW);
    return;
  }

  // Comfort LED logic
  const bool isComfort = (temp >= TEMP_COMFORT_MIN && temp <= TEMP_COMFORT_MAX &&
                          humid >= HUMID_COMFORT_MIN && humid <= HUMID_COMFORT_MAX);
  digitalWrite(PIN_LED_COMFORT, isComfort ? HIGH : LOW);

  // Alert LED logic
  const bool isAlert = (temp >= TEMP_ALERT_THRESHOLD || humid >= HUMID_ALERT_THRESHOLD);
  digitalWrite(PIN_LED_ALERT, isAlert ? HIGH : LOW);
}

void setupNetwork() {
  displayManager.showBootScreen("Connecting WiFi...");

  // If Wokwi preconfiguration is enabled, connect directly
  if (WOKWI_PRECONFIG_ENABLED) {
    Serial.printf("[WIFI] Connecting to preconfigured SSID: %s\r\n", WOKWI_PRECONFIG_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WOKWI_PRECONFIG_SSID, WOKWI_PRECONFIG_PASSWORD);

    const uint32_t wifiTimeout = millis() + 10000;
    while (WiFi.status() != WL_CONNECTED && millis() < wifiTimeout) {
      delay(250);
      Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[WIFI] Connected! IP: %s\r\n", WiFi.localIP().toString().c_str());
      deviceConfig.wifiSsid = WOKWI_PRECONFIG_SSID;
      deviceConfig.serverHost = WOKWI_PRECONFIG_SERVER_HOST;
      deviceConfig.serverPort = WOKWI_PRECONFIG_SERVER_PORT;
      deviceConfig.serverPath = "/ws/devices";
      deviceConfig.valid = true;
      return;
    }
  }

  // Fallback to WiFiManager for real hardware captive portal provisioning
  Serial.println(F("[WIFI] Starting WiFiManager portal..."));
  displayManager.showBootScreen("WiFi AP Portal");
  WiFiManager wm;
  wm.setConfigPortalTimeout(120);

  if (wm.autoConnect("ESP32-Climate-Setup")) {
    Serial.printf("[WIFI] Connected via WiFiManager! IP: %s\r\n",
                  WiFi.localIP().toString().c_str());
    deviceConfig.wifiSsid = WiFi.SSID();
    deviceConfig.serverHost = WOKWI_PRECONFIG_SERVER_HOST;
    deviceConfig.serverPort = WOKWI_PRECONFIG_SERVER_PORT;
    deviceConfig.serverPath = "/ws/devices";
    deviceConfig.valid = true;
  } else {
    Serial.println(F("[WIFI] Portal timeout. Running offline mode."));
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("\r\n=============================================="));
  Serial.println(F("  POC-VA-01: Climate Voice Monitor"));
  Serial.printf("  Hardware Sensor: %s\r\n",
#ifdef WOKWI_SIMULATION
                "DHT22 (Wokwi Simulation)"
#else
                "DHT11 (Kit Real Hardware)"
#endif
  );
  Serial.println(F("=============================================="));

  setupPins();

  // Initialize OLED Display
  if (!displayManager.begin()) {
    Serial.println(F("[WARN] OLED display not detected or failed to initialize"));
  }

  // Initialize DHT Sensor
  dht.begin();
  Serial.println(F("[DHT] Sensor interface initialized"));

  // Connect WiFi & configure Cloud Client
  setupNetwork();

  const bool wifiOk = (WiFi.status() == WL_CONNECTED);
  if (wifiOk && deviceConfig.valid) {
    cloudClient.begin(deviceConfig, DEVICE_ID, DEVICE_TOKEN);
  }

  displayManager.showBootScreen("System Ready");
}

void loop() {
  const uint32_t now = millis();
  const bool wifiOk = (WiFi.status() == WL_CONNECTED);

  // 1. Cooperative loop for WebSocket Cloud Client
  cloudClient.loop(wifiOk);

  // 2. Read DHT sensor at designated interval (>= 2000ms)
  if (now - lastSensorReadAt >= SENSOR_READ_INTERVAL_MS) {
    lastSensorReadAt = now;

    const float t = dht.readTemperature();
    const float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      currentTemperature = t;
      currentHumidity = h;
      updateLeds(currentTemperature, currentHumidity);

      // Marker for verification & serial debugging
      Serial.printf("SENSOR_DATA temp=%.1f humid=%.1f\r\n", t, h);
    } else {
      Serial.println(F("[DHT] Read failed! Check wiring."));
    }
  }

  // 3. Send periodic telemetry report to server
  if (now - lastCloudReportAt >= CLOUD_REPORT_INTERVAL_MS) {
    lastCloudReportAt = now;
    if (!isnan(currentTemperature) && !isnan(currentHumidity)) {
      cloudClient.reportSensorData(currentTemperature, currentHumidity);
    }
  }

  // 4. Update OLED Display
  displayManager.update(currentTemperature, currentHumidity, wifiOk, cloudClient.online());

  yield();
}
