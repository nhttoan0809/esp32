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

// Peripheral Instances
DHT dht(PIN_DHT_DATA, DHT_TYPE);
DisplayManager displayManager;
CloudClient cloudClient;
DeviceConfig deviceConfig;
SystemState systemState;

// Debounce & Timing
uint32_t lastBtnModeDebounce = 0;
uint32_t lastBtnLampDebounce = 0;
bool lastBtnModeState = HIGH;
bool lastBtnLampState = HIGH;

uint32_t lastDhtReadAt = 0;
uint32_t lastLightReadAt = 0;
uint32_t lastPotReadAt = 0;
uint32_t lastTelemetryAt = 0;
int lastPotRaw = -999;

uint32_t alarmStartedAt = 0;
constexpr uint32_t ALARM_DURATION_MS = 3000;

void setRelayState(bool on) {
  systemState.lampOn = on;
  // Relay IN1 is Active LOW
  digitalWrite(PIN_RELAY, on ? LOW : HIGH);
  Serial.printf("[RELAY] Lamp is now: %s\r\n", on ? "ON" : "OFF");
}

void setDimmerBrightness(uint8_t percent) {
  if (percent > 100) percent = 100;
  systemState.ledBrightness = percent;
  const uint32_t duty = (static_cast<uint32_t>(percent) * 255) / 100;
  ledcWrite(LEDC_CHANNEL, duty);
  Serial.printf("[PWM] Dimmer LED brightness set to: %u%% (duty=%u)\r\n", percent, duty);
}

void triggerAlarm() {
  systemState.alarmActive = true;
  alarmStartedAt = millis();
  digitalWrite(PIN_LED_ALERT, HIGH);
  digitalWrite(PIN_BUZZER, HIGH);
  Serial.println(F("[SECURITY] !! ALARM TRIGGERED: PIR INTRUSION IN GUARD MODE !!"));
}

void stopAlarm() {
  systemState.alarmActive = false;
  digitalWrite(PIN_LED_ALERT, LOW);
  digitalWrite(PIN_BUZZER, LOW);
}

void setupPins() {
  // Buttons with internal pullup
  pinMode(PIN_BTN_MODE, INPUT_PULLUP);
  pinMode(PIN_BTN_LAMP, INPUT_PULLUP);

  // Sensor inputs
  pinMode(PIN_PIR_MOTION, INPUT);
  pinMode(PIN_LDR_DO, INPUT);
  pinMode(PIN_POT_ADC, INPUT);

  // Digital Outputs
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED_ALERT, OUTPUT);
  pinMode(PIN_LED_COMFORT, OUTPUT);
  pinMode(PIN_LED_CLOUD, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // Initialize outputs
  setRelayState(false);
  digitalWrite(PIN_LED_ALERT, LOW);
  digitalWrite(PIN_LED_COMFORT, LOW);
  digitalWrite(PIN_LED_CLOUD, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  // Initialize LEDC PWM for Dimmer LED (GPIO 18)
  ledcSetup(LEDC_CHANNEL, LEDC_FREQ_HZ, LEDC_RES_BITS);
  ledcAttachPin(PIN_LED_DIMMER, LEDC_CHANNEL);
  setDimmerBrightness(50); // Default 50%
}

void setupNetwork() {
  displayManager.showBootScreen("Connecting WiFi...");

  if (WOKWI_PRECONFIG_ENABLED) {
    Serial.printf("[WIFI] Connecting preconfigured SSID: %s\r\n", WOKWI_PRECONFIG_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WOKWI_PRECONFIG_SSID, WOKWI_PRECONFIG_PASSWORD);

    const uint32_t timeout = millis() + 10000;
    while (WiFi.status() != WL_CONNECTED && millis() < timeout) {
      delay(250);
      Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      systemState.wifiConnected = true;
      systemState.ipAddress = WiFi.localIP().toString();
      deviceConfig.wifiSsid = WOKWI_PRECONFIG_SSID;
      deviceConfig.serverHost = WOKWI_PRECONFIG_SERVER_HOST;
      deviceConfig.serverPort = WOKWI_PRECONFIG_SERVER_PORT;
      deviceConfig.serverPath = "/ws/devices";
      deviceConfig.valid = true;
      Serial.printf("[WIFI] Connected! IP: %s\r\n", systemState.ipAddress.c_str());
      return;
    }
  }

  // WiFiManager captive portal for real hardware
  Serial.println(F("[WIFI] Starting WiFiManager portal..."));
  displayManager.showBootScreen("WiFi AP Portal");
  WiFiManager wm;
  wm.setConfigPortalTimeout(120);

  if (wm.autoConnect("ESP32-Hub-Setup")) {
    systemState.wifiConnected = true;
    systemState.ipAddress = WiFi.localIP().toString();
    deviceConfig.wifiSsid = WiFi.SSID();
    deviceConfig.serverHost = WOKWI_PRECONFIG_SERVER_HOST;
    deviceConfig.serverPort = WOKWI_PRECONFIG_SERVER_PORT;
    deviceConfig.serverPath = "/ws/devices";
    deviceConfig.valid = true;
    Serial.printf("[WIFI] Connected! IP: %s\r\n", systemState.ipAddress.c_str());
  } else {
    Serial.println(F("[WIFI] Portal timeout. Running offline."));
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("\r\n========================================================"));
  Serial.println(F("  POC-VA-06: Unified AI Smart Home Hub"));
  Serial.println(F("  Features: Climate + Lighting + Security + Potentiometer"));
  Serial.println(F("========================================================"));

  setupPins();

  // Initialize OLED Display
  if (!displayManager.begin()) {
    Serial.println(F("[WARN] OLED display not detected or failed to start"));
  }

  // Initialize DHT Sensor
  dht.begin();

  // Setup cloud command handlers
  cloudClient.setApplyRelayHandler([](bool on) {
    setRelayState(on);
    return on;
  });

  cloudClient.setApplyBrightnessHandler([](uint8_t brightness) {
    setDimmerBrightness(brightness);
    return brightness;
  });

  cloudClient.setApplySecurityModeHandler([](bool guard) {
    systemState.guardMode = guard;
    if (!guard && systemState.alarmActive) {
      stopAlarm();
    }
    Serial.printf("[SECURITY] Mode switched to: %s\r\n", guard ? "GUARD" : "ECO");
    return guard;
  });

  // Connect Network & Start Cloud
  setupNetwork();
  if (WiFi.status() == WL_CONNECTED && deviceConfig.valid) {
    cloudClient.begin(deviceConfig, DEVICE_ID, DEVICE_TOKEN);
  }

  displayManager.showBootScreen("Hub Ready!");
}

void handleButtons(uint32_t now) {
  // 1. Button Mode (GPIO 4) -> Switch OLED Page
  const int readBtnMode = digitalRead(PIN_BTN_MODE);
  if (readBtnMode != lastBtnModeState) {
    lastBtnModeDebounce = now;
  }
  if ((now - lastBtnModeDebounce) > DEBOUNCE_DELAY_MS) {
    static int debouncedBtnMode = HIGH;
    if (readBtnMode != debouncedBtnMode) {
      debouncedBtnMode = readBtnMode;
      if (debouncedBtnMode == LOW) {
        displayManager.nextPage();
      }
    }
  }
  lastBtnModeState = readBtnMode;

  // 2. Button Lamp (GPIO 14) -> Toggle Relay Lamp
  const int readBtnLamp = digitalRead(PIN_BTN_LAMP);
  if (readBtnLamp != lastBtnLampState) {
    lastBtnLampDebounce = now;
  }
  if ((now - lastBtnLampDebounce) > DEBOUNCE_DELAY_MS) {
    static int debouncedBtnLamp = HIGH;
    if (readBtnLamp != debouncedBtnLamp) {
      debouncedBtnLamp = readBtnLamp;
      if (debouncedBtnLamp == LOW) {
        setRelayState(!systemState.lampOn);
        cloudClient.reportRelayState(systemState.lampOn);
      }
    }
  }
  lastBtnLampState = readBtnLamp;
}

void loop() {
  const uint32_t now = millis();
  const bool wifiOk = (WiFi.status() == WL_CONNECTED);
  systemState.wifiConnected = wifiOk;
  systemState.cloudOnline = cloudClient.online();

  // 1. Cloud Client cooperative loop
  cloudClient.loop(wifiOk);

  // 2. Physical Button Handling (Debounced)
  handleButtons(now);

  // 3. Read DHT11 Sensor (every 2s)
  if (now - lastDhtReadAt >= SENSOR_DHT_INTERVAL_MS) {
    lastDhtReadAt = now;
    const float t = dht.readTemperature();
    const float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      systemState.temperature = t;
      systemState.humidity = h;

      // Update Comfort LED
      const bool isComfort = (t >= TEMP_COMFORT_MIN && t <= TEMP_COMFORT_MAX &&
                              h >= HUMID_COMFORT_MIN && h <= HUMID_COMFORT_MAX);
      digitalWrite(PIN_LED_COMFORT, isComfort ? HIGH : LOW);

      Serial.printf("SENSOR_DHT temp=%.1f humid=%.1f\r\n", t, h);
    }
  }

  // 4. Read LDR Ambient Light Sensor (every 1s)
  if (now - lastLightReadAt >= SENSOR_LIGHT_INTERVAL_MS) {
    lastLightReadAt = now;
    // Standard LM393 LDR module DO is HIGH when dark, LOW when bright
    systemState.ldrDark = (digitalRead(PIN_LDR_DO) == HIGH);
  }

  // 5. Read Potentiometer Dimmer (ADC1 GPIO 34 every 100ms)
  if (now - lastPotReadAt >= SENSOR_ADC_INTERVAL_MS) {
    lastPotReadAt = now;
    const int raw = analogRead(PIN_POT_ADC);
    if (abs(raw - lastPotRaw) > 60) { // ~1.5% hysteresis to avoid jitter
      lastPotRaw = raw;
      const uint8_t pct = map(raw, 0, 4095, 0, 100);
      setDimmerBrightness(pct);
    }
  }

  // 6. Read PIR Motion Sensor & Alarm Logic
  const bool motionNow = (digitalRead(PIN_PIR_MOTION) == HIGH);
  if (motionNow != systemState.motionDetected) {
    systemState.motionDetected = motionNow;
    if (motionNow) {
      systemState.motionCount++;
      Serial.printf("[PIR] Motion Detected! Total Count: %u\r\n", systemState.motionCount);
      cloudClient.reportMotionEvent(true);

      if (systemState.guardMode) {
        triggerAlarm();
      }
    } else {
      cloudClient.reportMotionEvent(false);
    }
  }

  // Check alarm timeout
  if (systemState.alarmActive) {
    if (now - alarmStartedAt >= ALARM_DURATION_MS) {
      stopAlarm();
    }
  }

  // 7. Update Status LEDs
  digitalWrite(PIN_LED_CLOUD, systemState.cloudOnline ? HIGH : LOW);
  if (!systemState.alarmActive) {
    const bool heatAlert = (!isnan(systemState.temperature) && systemState.temperature >= TEMP_ALERT_HOT);
    digitalWrite(PIN_LED_ALERT, heatAlert ? HIGH : LOW);
  }

  // 8. Send Periodic Cloud Telemetry (every 5s)
  if (now - lastTelemetryAt >= CLOUD_REPORT_INTERVAL_MS) {
    lastTelemetryAt = now;
    cloudClient.reportTelemetry(systemState);
  }

  // 9. Update OLED Display
  displayManager.update(systemState);

  yield();
}
