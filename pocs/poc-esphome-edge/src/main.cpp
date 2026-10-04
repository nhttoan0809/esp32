/**
 * POC-A: ESPHome Approach Companion Firmware (C++ / PlatformIO)
 * Triết lý: Compile-time tailored architecture with on-device edge autonomy.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// -----------------------------------------------------------------------------
// Pin Configuration (ESP32 DevKit V1 30-Pin)
// -----------------------------------------------------------------------------
constexpr uint8_t PIN_ACTUATOR = 23;     // Actuator Relay/LED
constexpr uint8_t PIN_BUTTON   = 18;     // Local Pushbutton
constexpr uint8_t PIN_DHT      = 19;     // DHT Sensor Data Pin
constexpr uint8_t PIN_ANALOG   = 32;     // Analog Input on ADC1

// -----------------------------------------------------------------------------
// Hardware-First Dual-Target Rule (DHT11 on hardware vs DHT22 on Wokwi)
// -----------------------------------------------------------------------------
#if defined(WOKWI_SIMULATION)
  #define DHT_TYPE DHT22
#else
  #define DHT_TYPE DHT11
#endif

DHT dht(PIN_DHT, DHT_TYPE);

// -----------------------------------------------------------------------------
// MQTT & Wi-Fi Configuration
// -----------------------------------------------------------------------------
const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
const char* MQTT_SERVER   = "10.0.2.2";  // Or local Self-Hosted Server IP
const int   MQTT_PORT     = 1883;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Local State
bool actuatorState = false;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
constexpr unsigned long DEBOUNCE_DELAY_MS = 30;

unsigned long lastTelemetryTime = 0;
constexpr unsigned long TELEMETRY_INTERVAL_MS = 5000;

void applyActuatorState(bool state) {
  actuatorState = state;
  digitalWrite(PIN_ACTUATOR, actuatorState ? HIGH : LOW);
  Serial.printf("[EDGE-AUTONOMY] Actuator GPIO%d -> %s\n", PIN_ACTUATOR, actuatorState ? "ON" : "OFF");
  
  if (mqttClient.connected()) {
    mqttClient.publish("edge/esphome/switch/living_fan_relay/state", actuatorState ? "ON" : "OFF");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  msg.trim();
  Serial.printf("[MQTT RECV] %s: %s\n", topic, msg.c_str());

  if (String(topic) == "edge/esphome/switch/living_fan_relay/command") {
    if (msg.equalsIgnoreCase("ON")) {
      applyActuatorState(true);
    } else if (msg.equalsIgnoreCase("OFF")) {
      applyActuatorState(false);
    }
  }
}

void reconnectMqtt() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  if (!mqttClient.connected()) {
    Serial.print("[MQTT] Connecting to Self-Hosted Server...");
    if (mqttClient.connect("esphome-edge-node", "edge/esphome/status", 0, true, "offline")) {
      Serial.println(" CONNECTED! ✅");
      mqttClient.publish("edge/esphome/status", "online", true);
      mqttClient.subscribe("edge/esphome/switch/living_fan_relay/command");
    } else {
      Serial.printf(" FAILED (rc=%d)\n", mqttClient.state());
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n=======================================================");
  Serial.println("  POC-A: ESPHome Compile-Time Edge Autonomy Firmware   ");
  Serial.println("=======================================================");

  pinMode(PIN_ACTUATOR, OUTPUT);
  digitalWrite(PIN_ACTUATOR, LOW);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_ANALOG, INPUT);

  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  Serial.println("[SETUP] Hardware initialized. Ready.");
}

void loop() {
  // ---------------------------------------------------------------------------
  // 1. Edge Autonomy: Local Button Debounce & Instant Toggle (< 5ms response)
  // ---------------------------------------------------------------------------
  int reading = digitalRead(PIN_BUTTON);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    static int buttonState = HIGH;
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) { // Button Pressed
        Serial.println("[EDGE EVENT] Physical Button Pressed -> Toggling Actuator Locally!");
        applyActuatorState(!actuatorState);
        if (mqttClient.connected()) {
          mqttClient.publish("edge/esphome/binary_sensor/physical_button/state", "ON");
        }
      } else {
        if (mqttClient.connected()) {
          mqttClient.publish("edge/esphome/binary_sensor/physical_button/state", "OFF");
        }
      }
    }
  }
  lastButtonState = reading;

  // ---------------------------------------------------------------------------
  // 2. MQTT Client Handling
  // ---------------------------------------------------------------------------
  if (!mqttClient.connected()) {
    static unsigned long lastReconnectAttempt = 0;
    if (millis() - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = millis();
      reconnectMqtt();
    }
  } else {
    mqttClient.loop();
  }

  // ---------------------------------------------------------------------------
  // 3. Periodic Telemetry Reporting
  // ---------------------------------------------------------------------------
  if (millis() - lastTelemetryTime > TELEMETRY_INTERVAL_MS) {
    lastTelemetryTime = millis();

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    int analogVal = analogRead(PIN_ANALOG);

    if (!isnan(temp) && !isnan(hum)) {
      Serial.printf("[TELEMETRY] Temp: %.1f C, Hum: %.1f %%, ADC: %d\n", temp, hum, analogVal);
      if (mqttClient.connected()) {
        char tempStr[10], humStr[10], adcStr[10];
        dtostrf(temp, 4, 1, tempStr);
        dtostrf(hum, 4, 1, humStr);
        snprintf(adcStr, sizeof(adcStr), "%d", analogVal);

        mqttClient.publish("edge/esphome/sensor/ambient_temperature/state", tempStr);
        mqttClient.publish("edge/esphome/sensor/ambient_humidity/state", humStr);
        mqttClient.publish("edge/esphome/sensor/analog_input/state", adcStr);
      }
    }
  }
}
