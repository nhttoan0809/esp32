#include <Arduino.h>

/**
 * POC: IR Obstacle Avoidance Barrier & Contactless Object Counter
 * Board: ESP32 DevKit V1 (30 chân)
 * 
 * Sơ đồ chân:
 * - IR Sensor OUT -> GPIO 27 (External Interrupt, Active LOW)
 * - Indicator LED -> GPIO 13 (Qua trở 220 Ohm)
 * - Active Buzzer -> GPIO 14 (Còi bíp phản hồi âm thanh)
 */

static const uint8_t PIN_IR_IN  = 27;
static const uint8_t PIN_LED    = 13;
static const uint8_t PIN_BUZZER = 14;

// Lockout Dead-time (Thời gian khóa chống kích hoạt kép do mép rung)
static const unsigned long LOCKOUT_DEADTIME_MS = 1000;
static const unsigned long LED_PULSE_DURATION_MS = 400;
static const unsigned long BEEP_DURATION_MS = 40;

// Biến chia sẻ giữa ISR và Loop
static volatile bool isTriggered = false;
static volatile unsigned long lastInterruptTime = 0;

static uint32_t objectCount = 0;
static unsigned long ledOffTime = 0;
static unsigned long buzzerOffTime = 0;
static unsigned long lastHeartbeatTime = 0;

// Trình phục vụ ngắt phần cứng (Hardware ISR)
void IRAM_ATTR isrObstacleDetected() {
  unsigned long now = millis();
  if (now - lastInterruptTime >= LOCKOUT_DEADTIME_MS) {
    lastInterruptTime = now;
    isTriggered = true;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F("[SYSTEM] POC IR Obstacle Barrier Initialized"));
  Serial.println(F("[CONFIG] IR Input: GPIO 27 (Interrupt FALLING, Active LOW)"));
  Serial.println(F("[CONFIG] LED: GPIO 13 | Buzzer: GPIO 14"));
  Serial.printf("[CONFIG] Dead-time Lockout: %lu ms\n", LOCKOUT_DEADTIME_MS);
  Serial.println(F("=================================================="));

  pinMode(PIN_IR_IN, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);

  // Gắn ngắt ngoài cạnh xuống (FALLING)
  attachInterrupt(digitalPinToInterrupt(PIN_IR_IN), isrObstacleDetected, FALLING);
}

void loop() {
  unsigned long now = millis();

  // Xử lý sự kiện ngắt từ cảm biến
  if (isTriggered) {
    isTriggered = false;
    objectCount++;

    // Bật LED và Buzzer
    digitalWrite(PIN_LED, HIGH);
    digitalWrite(PIN_BUZZER, HIGH);

    ledOffTime = now + LED_PULSE_DURATION_MS;
    buzzerOffTime = now + BEEP_DURATION_MS;

    Serial.printf("[EVENT %6lu ms] >>> OBSTACLE DETECTED! Total Count: %u\n", now, objectCount);
  }

  // Tắt Buzzer sau thời gian bíp ngắn
  if (buzzerOffTime > 0 && now >= buzzerOffTime) {
    digitalWrite(PIN_BUZZER, LOW);
    buzzerOffTime = 0;
  }

  // Tắt LED sau thời gian hiển thị
  if (ledOffTime > 0 && now >= ledOffTime) {
    digitalWrite(PIN_LED, LOW);
    ledOffTime = 0;
  }

  // Định kỳ in heartbeat thông báo hệ thống đang canh gác
  if (now - lastHeartbeatTime >= 3000) {
    lastHeartbeatTime = now;
    int pinRaw = digitalRead(PIN_IR_IN);
    Serial.printf("[%6lu ms] [HEARTBEAT] Sensor Pin:%s | Total Objects Passed: %u\n",
                  now,
                  (pinRaw == LOW ? "DETECTED (LOW)" : "CLEAR (HIGH)"),
                  objectCount);
  }
}
