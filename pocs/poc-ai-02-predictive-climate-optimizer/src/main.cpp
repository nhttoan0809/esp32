#include <Arduino.h>
#include "config.h"
#include "sensors_manager.h"
#include "display_manager.h"
#include "relay_controller.h"
#include "cloud_client.h"
#include "failsafe.h"

// Kiểm tra xem có file cấu hình bí mật cục bộ không
#if __has_include("secrets.h")
  #include "secrets.h"
#else
  #define SECRET_WIFI_SSID    DEFAULT_WIFI_SSID
  #define SECRET_WIFI_PASS    DEFAULT_WIFI_PASS
  #define SECRET_SERVER_HOST  DEFAULT_WS_HOST
  #define SECRET_SERVER_PORT  DEFAULT_WS_PORT
#endif

// Khởi tạo các module
static SensorsManager sensors;
static DisplayManager display;
static RelayController relays;
static CloudClient cloud;
static FailsafeController failsafe;

// Quản lý nút bấm chuyển trang
static int lastButtonState = HIGH;
static unsigned long lastDebounceTime = 0;
static const unsigned long debounceDelay = 200;

// Chu kỳ gửi telemetry
static unsigned long lastTelemetrySendMs = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println(F("\n========================================================"));
  Serial.println(F("[AI-02] === ESP32 PREDICTIVE CLIMATE OPTIMIZER BOOTING ==="));
  Serial.println(F("[AI-02] Thiet bi: ESP32 DevKit V1 (30 chan)"));
  #if defined(WOKWI_SIMULATION)
  Serial.println(F("[AI-02] Che do: Wokwi Simulation Dual-Target"));
  #else
  Serial.println(F("[AI-02] Che do: Real Hardware Target"));
  #endif
  Serial.println(F("========================================================"));

  // 1. Khoi tao nut bam
  pinMode(PIN_BUTTON_PAGE, INPUT_PULLUP);

  // 2. Khoi tao Module Relay 2 kenh
  relays.begin();

  // 3. Khoi tao man hinh OLED SSD1306
  display.begin();

  // 4. Khoi tao cam bien DHT, LDR, RTC
  sensors.begin();

  // 5. Khoi tao ket noi Mang & Cloud
  cloud.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASS, SECRET_SERVER_HOST, SECRET_SERVER_PORT, DEFAULT_WS_PATH);

  Serial.println(F("[AI-02] BOOT_COMPLETE: He thong khoi dong hoan tat, bat dau vong lap giam sat."));
}

void loop() {
  unsigned long now = millis();

  // 1. Xu ly nut bam chuyen trang man hinh OLED
  int reading = digitalRead(PIN_BUTTON_PAGE);
  if (reading != lastButtonState) {
    lastDebounceTime = now;
  }
  if ((now - lastDebounceTime) > debounceDelay) {
    static int buttonState = HIGH;
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        display.togglePage();
      }
    }
  }
  lastButtonState = reading;

  // 2. Cap nhat cac cam bien (chu ky 2s)
  sensors.update();

  // 3. Duy tri ket noi WebSocket & xu ly goi tin den
  cloud.loop();

  // 4. Neu co quyet dinh / lenh moi tu mo hinh AI
  if (cloud.hasNewCommand()) {
    const AIDecisionData& ai = cloud.getLatestAIDecision();
    
    // Ap dung lenh dieu khien cho Relay
    relays.setFan(ai.relay1_fan);
    relays.setHeat(ai.relay2_heat);
    
    // Cap nhat LED chi thi theo trang thai AI
    bool isComfort = (strcmp(ai.comfort_status, "COMFORT") == 0);
    bool isWarning = (strcmp(ai.optimization_mode, "PRE_COOLING") == 0 ||
                      strcmp(ai.optimization_mode, "EMERGENCY_COOLING") == 0 ||
                      strcmp(ai.optimization_mode, "MOLD_VENTILATION") == 0);
    relays.updateLEDs(isComfort, isWarning);
    
    cloud.clearNewCommandFlag();
  }

  // 5. Giam sat an toan & Tu hanh cuc bo neu mat ket noi AI Server
  AIDecisionData aiDataCopy = cloud.getLatestAIDecision();
  failsafe.evaluate(sensors.getData(),
                    aiDataCopy,
                    relays,
                    cloud.getLastResponseTimeMs(),
                    cloud.isConnected());

  // 6. Gui telemetry dinh ky moi 5 giay
  if (now - lastTelemetrySendMs >= TELEMETRY_INTERVAL_MS || lastTelemetrySendMs == 0) {
    lastTelemetrySendMs = now;
    cloud.sendTelemetry(sensors.getData(), relays.isFanOn(), relays.isHeatOn());
  }

  // 7. Cap nhat hien thi man hinh OLED (chu ky 1s)
  display.update(sensors.getData(),
                 aiDataCopy,
                 relays.isFanOn(),
                 relays.isHeatOn());

  delay(10); // Nhuong CPU cho FreeRTOS va WiFi core
}
