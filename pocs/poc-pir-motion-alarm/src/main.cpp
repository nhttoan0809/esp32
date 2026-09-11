#include <Arduino.h>

/**
 * POC: PIR Motion Sensor Controller (Auto-Off Lighting & Security Alarm)
 * Board: ESP32 DevKit V1 (30 chân)
 * 
 * Sơ đồ chân:
 * - PIR OUT     -> GPIO 33 (Digital Input, 3.3V TTL)
 * - Mode Button -> GPIO 4  (INPUT_PULLUP, nhấn = LOW)
 * - Light LED   -> GPIO 21 (Qua trở 220 Ohm)
 * - Buzzer      -> GPIO 22 (Active Buzzer cảnh báo)
 * - PIR VCC     -> VIN (5V từ USB)
 */

static const uint8_t PIN_PIR_IN    = 33;
static const uint8_t PIN_BTN_MODE  = 4;
static const uint8_t PIN_LED_LIGHT = 21;
static const uint8_t PIN_BUZZER    = 22;

enum SystemMode {
  MODE_AUTO_LIGHT = 0,    // Chế độ đèn tự động tiết kiệm điện
  MODE_ARMED_SECURITY = 1 // Chế độ báo động an ninh
};

static SystemMode currentMode = MODE_AUTO_LIGHT;

// Thời gian cấu hình
static const unsigned long WARMUP_DURATION_MS = 10000; // 10s khởi động nhiệt ban đầu
static const unsigned long HOLD_TIME_MS       = 8000;  // 8s duy trì đèn sau khi hết người
static const unsigned long DEBOUNCE_MS        = 50;

static unsigned long bootTime = 0;
static unsigned long lastMotionTime = 0;
static unsigned long lastLogTime = 0;
static bool isLightOn = false;

// Quản lý nút bấm
static int lastButtonReading = HIGH;
static int buttonState = HIGH;
static unsigned long lastDebounceTime = 0;

// Biến chớp còi báo động
static unsigned long lastAlarmToggleTime = 0;
static bool alarmToggleState = false;

void chirpBuzzer(uint8_t count, uint16_t onMs, uint16_t offMs) {
  for (uint8_t i = 0; i < count; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(onMs);
    digitalWrite(PIN_BUZZER, LOW);
    if (i + 1 < count) delay(offMs);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  bootTime = millis();

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F("[SYSTEM] POC PIR Motion Alarm Initialized"));
  Serial.println(F("[CONFIG] PIR Input: GPIO 33 | PIR Power: VIN (5V)"));
  Serial.println(F("[CONFIG] Mode Button: GPIO 4 | LED: GPIO 21 | Buzzer: GPIO 22"));
  Serial.println(F("[CONFIG] Default Mode: AUTO-LIGHT (Press Button to toggle ARMED)"));
  Serial.println(F("=================================================="));

  pinMode(PIN_PIR_IN, INPUT);
  pinMode(PIN_BTN_MODE, INPUT_PULLUP);
  pinMode(PIN_LED_LIGHT, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_LED_LIGHT, LOW);
  digitalWrite(PIN_BUZZER, LOW);
}

void loop() {
  unsigned long now = millis();

  // 1. Xử lý giai đoạn làm nóng (Warm-up Period)
  if (now - bootTime < WARMUP_DURATION_MS) {
    unsigned long remainingSec = (WARMUP_DURATION_MS - (now - bootTime)) / 1000 + 1;
    if (now - lastLogTime >= 1000) {
      lastLogTime = now;
      Serial.printf("[%6lu ms] [WARMUP] Cảm biến PIR đang ổn định quang học... (%lu s còn lại)\n", now, remainingSec);
    }
    return;
  }

  // 2. Xử lý nút bấm chuyển đổi chế độ (Button Debounce)
  int reading = digitalRead(PIN_BTN_MODE);
  if (reading != lastButtonReading) {
    lastDebounceTime = now;
  }
  if ((now - lastDebounceTime) > DEBOUNCE_MS) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) { // Nhấn nút
        if (currentMode == MODE_AUTO_LIGHT) {
          currentMode = MODE_ARMED_SECURITY;
          Serial.printf("[EVENT %6lu ms] >>> MODE CHANGED: [ARMED SECURITY 🚨]\n", now);
          chirpBuzzer(2, 60, 60); // 2 tiếng bíp báo Arm
        } else {
          currentMode = MODE_AUTO_LIGHT;
          Serial.printf("[EVENT %6lu ms] >>> MODE CHANGED: [AUTO-LIGHT 💡]\n", now);
          chirpBuzzer(1, 150, 0); // 1 tiếng bíp dài báo Disarm
        }
      }
    }
  }
  lastButtonReading = reading;

  // 3. Đọc trạng thái cảm biến PIR
  bool motionDetected = (digitalRead(PIN_PIR_IN) == HIGH);

  // 4. Máy trạng thái điều khiển theo chế độ
  if (currentMode == MODE_AUTO_LIGHT) {
    // Tắt còi bảo đảm an toàn
    digitalWrite(PIN_BUZZER, LOW);

    if (motionDetected) {
      lastMotionTime = now;
      if (!isLightOn) {
        isLightOn = true;
        digitalWrite(PIN_LED_LIGHT, HIGH);
        Serial.printf("[EVENT %6lu ms] >>> MOTION DETECTED! Light ON (Keep-Alive started).\n", now);
      }
    } else {
      // Khi không còn chuyển động, kiểm tra khoảng thời gian Hold Time
      if (isLightOn) {
        if (now - lastMotionTime >= HOLD_TIME_MS) {
          isLightOn = false;
          digitalWrite(PIN_LED_LIGHT, LOW);
          Serial.printf("[EVENT %6lu ms] >>> TIMEOUT REACHED (%lu ms). Light OFF.\n", now, HOLD_TIME_MS);
        }
      }
    }
  } else { // MODE_ARMED_SECURITY
    if (motionDetected) {
      // Nhấp nháy còi và đèn cảnh báo liên tục (120ms nhịp)
      if (now - lastAlarmToggleTime >= 120) {
        lastAlarmToggleTime = now;
        alarmToggleState = !alarmToggleState;
        digitalWrite(PIN_LED_LIGHT, alarmToggleState ? HIGH : LOW);
        digitalWrite(PIN_BUZZER, alarmToggleState ? HIGH : LOW);
      }
    } else {
      digitalWrite(PIN_LED_LIGHT, LOW);
      digitalWrite(PIN_BUZZER, LOW);
      alarmToggleState = false;
    }
  }

  // 5. Định kỳ in log trạng thái
  if (now - lastLogTime >= 1500) {
    lastLogTime = now;
    unsigned long timeSinceLastMotion = (lastMotionTime > 0) ? (now - lastMotionTime) : 99999;
    Serial.printf("[%6lu ms] Mode:%-12s | PIR:%-8s | Light:%-3s | Idle:%5lu ms\n",
                  now,
                  (currentMode == MODE_AUTO_LIGHT ? "AUTO-LIGHT" : "ARMED"),
                  (motionDetected ? "MOTION" : "QUIET"),
                  (digitalRead(PIN_LED_LIGHT) ? "ON" : "OFF"),
                  timeSinceLastMotion);
  }

  delay(10);
}
