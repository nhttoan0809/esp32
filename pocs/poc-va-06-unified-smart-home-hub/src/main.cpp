#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <DHT.h>

#include "app_config.h"
#include "device_config.h"
#include "config_store.h"
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
ConfigStore configStore;
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

uint8_t lastActiveBrightness = 70; // Memory of last active brightness (1-100%)

void applySmartLamp(bool power, uint8_t brightness) {
  systemState.lampOn = power;
  if (power) {
    if (brightness == 0) {
      brightness = (lastActiveBrightness > 0) ? lastActiveBrightness : 70;
    }
    lastActiveBrightness = brightness;
    systemState.ledBrightness = brightness;
    systemState.memoryBrightness = lastActiveBrightness;

    // 1. Close Relay to connect Cathode to GND (Relay Active-LOW: LOW = ON)
    digitalWrite(PIN_RELAY, LOW);

    // 2. Output PWM duty cycle on GPIO 18
    const uint32_t duty = (static_cast<uint32_t>(brightness) * 255) / 100;
    ledcWrite(LEDC_CHANNEL, duty);
    Serial.printf("[SMART_LAMP] ON (Relay CLOSED, PWM=%u%%, duty=%u)\r\n", brightness, duty);
  } else {
    systemState.ledBrightness = 0;
    systemState.memoryBrightness = lastActiveBrightness;

    // 1. Set PWM to 0
    ledcWrite(LEDC_CHANNEL, 0);

    // 2. Open Relay (Air-gap cutoff, Relay Active-LOW: HIGH = OFF)
    digitalWrite(PIN_RELAY, HIGH);
    Serial.printf("[SMART_LAMP] OFF (Relay OPEN 0W Cutoff, Mem=%u%%)\r\n", lastActiveBrightness);
  }
}

void setRelayState(bool on) {
  applySmartLamp(on, on ? lastActiveBrightness : 0);
}

void setDimmerBrightness(uint8_t percent) {
  if (percent > 0) {
    applySmartLamp(true, percent);
  } else {
    applySmartLamp(false, 0);
  }
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
  digitalWrite(PIN_BUZZER, LOW);

  // Self-test Blue LED: blink 3 times quickly to verify circuit & pin
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_LED_CLOUD, HIGH);
    delay(80);
    digitalWrite(PIN_LED_CLOUD, LOW);
    delay(80);
  }

  // Initialize LEDC PWM for Dimmer LED (GPIO 18)
  ledcSetup(LEDC_CHANNEL, LEDC_FREQ_HZ, LEDC_RES_BITS);
  ledcAttachPin(PIN_LED_DIMMER, LEDC_CHANNEL);
  applySmartLamp(false, 0); // Default OFF on boot
}

void setupNetwork() {
  displayManager.showBootScreen("Connecting WiFi...");

  // 1. Reset Wi-Fi STA to clear any previous dangling connection attempts
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  // 2. Load stored config from Flash NVS
  DeviceConfig stored;
  const bool hasStored = configStore.load(stored);
  if (hasStored) {
    Serial.printf("[NVS] Loaded Server config: %s:%u%s\r\n",
                  stored.serverHost.c_str(), stored.serverPort, stored.serverPath.c_str());
  }

#ifdef WOKWI_SIMULATION
  if (WOKWI_PRECONFIG_ENABLED) {
    Serial.printf("[WIFI] Connecting preconfigured SSID: %s\r\n", WOKWI_PRECONFIG_SSID);
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
#endif

  // 3. Real Hardware: WiFiManager captive portal with NVS Persistence
  Serial.println(F("[WIFI] Starting WiFiManager..."));

  WiFiManager wm;

  char serverHostParam[128];
  char serverPortParam[8];
  char serverPathParam[64];

  if (hasStored && !stored.serverHost.isEmpty()) {
    strncpy(serverHostParam, stored.serverHost.c_str(), sizeof(serverHostParam) - 1);
    snprintf(serverPortParam, sizeof(serverPortParam), "%u", stored.serverPort);
    strncpy(serverPathParam, stored.serverPath.c_str(), sizeof(serverPathParam) - 1);
  } else {
    strncpy(serverHostParam, WOKWI_PRECONFIG_SERVER_HOST, sizeof(serverHostParam) - 1);
    snprintf(serverPortParam, sizeof(serverPortParam), "%u", WOKWI_PRECONFIG_SERVER_PORT);
    strncpy(serverPathParam, "/ws/devices", sizeof(serverPathParam) - 1);
  }
  serverHostParam[sizeof(serverHostParam) - 1] = '\0';
  serverPortParam[sizeof(serverPortParam) - 1] = '\0';
  serverPathParam[sizeof(serverPathParam) - 1] = '\0';

  WiFiManagerParameter custom_server_host("host", "Server Host (Cloudflare)", serverHostParam, 128);
  WiFiManagerParameter custom_server_port("port", "Server Port", serverPortParam, 8);
  WiFiManagerParameter custom_server_path("path", "WebSocket Path", serverPathParam, 64);

  wm.addParameter(&custom_server_host);
  wm.addParameter(&custom_server_port);
  wm.addParameter(&custom_server_path);

  static bool shouldSaveCustomConfig = false;
  wm.setSaveConfigCallback([]() {
    shouldSaveCustomConfig = true;
  });

  wm.setAPCallback([](WiFiManager *myWiFiManager) {
    Serial.printf("[WM] SoftAP Started: %s (IP: %s)\r\n",
                  myWiFiManager->getConfigPortalSSID().c_str(),
                  WiFi.softAPIP().toString().c_str());
    displayManager.showBootScreen("WiFi AP Portal\nSSID: ESP32-Hub-Setup\nIP: 192.168.4.1");
    // Turn ON Blue LED to visually indicate Portal / Setup mode is active!
    digitalWrite(PIN_LED_CLOUD, HIGH);
  });

  // CRITICAL FIX: Set connectTimeout to 25s so ESP32 waits for DHCP & 4-way WPA2 handshake!
  wm.setConnectTimeout(25);
  wm.setConfigPortalTimeout(180); // 3 minutes timeout

  // Attempt autoConnect or start Captive Portal
  if (wm.autoConnect("ESP32-Hub-Setup")) {
    systemState.wifiConnected = true;
    systemState.ipAddress = WiFi.localIP().toString();

    if (shouldSaveCustomConfig) {
      DeviceConfig toSave;
      toSave.wifiSsid = WiFi.SSID();
      toSave.wifiPassword = WiFi.psk();
      toSave.serverHost = custom_server_host.getValue();
      toSave.serverPort = static_cast<uint16_t>(atoi(custom_server_port.getValue()));
      toSave.serverPath = custom_server_path.getValue();
      toSave.valid = true;

      configStore.save(toSave);
      deviceConfig = toSave;
      Serial.printf("[NVS] Saved configuration to Flash: %s:%u%s\r\n",
                    deviceConfig.serverHost.c_str(), deviceConfig.serverPort, deviceConfig.serverPath.c_str());
    } else {
      deviceConfig.wifiSsid = WiFi.SSID();
      deviceConfig.serverHost = serverHostParam;
      deviceConfig.serverPort = static_cast<uint16_t>(atoi(serverPortParam));
      deviceConfig.serverPath = serverPathParam;
      deviceConfig.valid = true;
    }

    Serial.printf("[WIFI] Connected! IP: %s (RSSI: %d dBm)\r\n",
                  systemState.ipAddress.c_str(), WiFi.RSSI());
  } else {
    Serial.println(F("[WIFI] Portal timeout or connection failed. Running offline."));
    digitalWrite(PIN_LED_CLOUD, LOW);
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
  // 1. Button Mode (GPIO 4) -> Short Press: Switch OLED Page | Long Press (>5s): Factory Reset WiFi & NVS
  const int readBtnMode = digitalRead(PIN_BTN_MODE);
  static uint32_t btnModePressStart = 0;
  static bool btnModeHeld = false;

  if (readBtnMode != lastBtnModeState) {
    lastBtnModeDebounce = now;
  }
  if ((now - lastBtnModeDebounce) > DEBOUNCE_DELAY_MS) {
    static int debouncedBtnMode = HIGH;
    if (readBtnMode != debouncedBtnMode) {
      debouncedBtnMode = readBtnMode;
      if (debouncedBtnMode == LOW) {
        btnModePressStart = now;
        btnModeHeld = false;
      } else {
        // Released
        if (!btnModeHeld && (now - btnModePressStart < 5000)) {
          displayManager.nextPage();
        }
      }
    }

    // Check if held for 5 seconds -> Factory Reset
    if (debouncedBtnMode == LOW && !btnModeHeld && (now - btnModePressStart >= 5000)) {
      btnModeHeld = true;
      Serial.println(F("[FACTORY RESET] Button 1 held for 5s! Erasing WiFi & Server settings..."));
      displayManager.showBootScreen("FACTORY RESET\nErasing Config...");
      for (int i = 0; i < 5; i++) {
        digitalWrite(PIN_LED_CLOUD, HIGH); delay(80);
        digitalWrite(PIN_LED_CLOUD, LOW); delay(80);
      }
      WiFiManager wm;
      wm.resetSettings();
      configStore.clear();
      delay(1000);
      ESP.restart();
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
      cloudClient.reportRelayState(systemState.lampOn);
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
  if (systemState.cloudOnline) {
    digitalWrite(PIN_LED_CLOUD, HIGH); // Solid ON: Fully connected to Cloud WebSocket
  } else if (wifiOk) {
    // Wi-Fi STA Connected, waiting/retrying Cloud WebSocket: gentle blink (500ms)
    digitalWrite(PIN_LED_CLOUD, ((now / 500) % 2 == 0) ? HIGH : LOW);
  } else {
    digitalWrite(PIN_LED_CLOUD, LOW);
  }
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
