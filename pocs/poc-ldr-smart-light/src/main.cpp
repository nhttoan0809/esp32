#include <Arduino.h>

/**
 * POC: LDR Smart Light Controller with Schmitt-Trigger Hysteresis & Auto-Dimmer
 * Board: ESP32 DevKit V1 (30 chân)
 * 
 * Sơ đồ chân:
 * - LDR AO -> GPIO 34 (ADC1_CH6, Input-Only)
 * - LDR DO -> GPIO 35 (ADC1_CH7, Input-Only)
 * - LED Light -> GPIO 18 (LEDC PWM qua trở 220 Ohm)
 */

// Định nghĩa chân
static const uint8_t PIN_LDR_AO = 34;
static const uint8_t PIN_LDR_DO = 35;
static const uint8_t PIN_LED_LIGHT = 18;

// Cấu hình LEDC PWM (5 kHz, 12-bit)
static const uint8_t LEDC_CHANNEL = 0;
static const uint32_t LEDC_FREQ_HZ = 5000;
static const uint8_t LEDC_RESOLUTION_BITS = 12; // 0 - 4095
static const uint32_t PWM_MAX_DUTY = 4095;

// Ngưỡng trễ Hysteresis chống chập chờn
// ADC dải 0 - 4095: Giá trị càng cao -> Ánh sáng càng tối
static const uint16_t THRESHOLD_DARK_ON   = 2800; // Trời tối vượt ngưỡng này -> Bật đèn
static const uint16_t THRESHOLD_BRIGHT_OFF = 2200; // Trời sáng dưới ngưỡng này -> Tắt đèn

// Biến trạng thái
static bool isLightOn = false;
static uint32_t currentDuty = 0;
static unsigned long lastLogTime = 0;
static const unsigned long LOG_INTERVAL_MS = 500;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F("[SYSTEM] POC LDR Smart Light Initialized"));
  Serial.println(F("[CONFIG] AO Pin: GPIO 34 (ADC1) | DO Pin: GPIO 35"));
  Serial.println(F("[CONFIG] LED Pin: GPIO 18 (LEDC PWM 5kHz 12-bit)"));
  Serial.printf("[CONFIG] Hysteresis: ON >= %u | OFF <= %u\n", THRESHOLD_DARK_ON, THRESHOLD_BRIGHT_OFF);
  Serial.println(F("=================================================="));

  // Cấu hình GPIO
  pinMode(PIN_LDR_AO, INPUT);
  pinMode(PIN_LDR_DO, INPUT);

  // Cấu hình LEDC PWM
  ledcSetup(LEDC_CHANNEL, LEDC_FREQ_HZ, LEDC_RESOLUTION_BITS);
  ledcAttachPin(PIN_LED_LIGHT, LEDC_CHANNEL);
  ledcWrite(LEDC_CHANNEL, 0);
}

void loop() {
  unsigned long now = millis();

  // Đọc cảm biến
  uint16_t rawAdc = analogRead(PIN_LDR_AO);
  int digitalState = digitalRead(PIN_LDR_DO);
  float voltage = (rawAdc * 3.3f) / 4095.0f;

  // Thuật toán Schmitt-Trigger Hysteresis
  if (!isLightOn) {
    if (rawAdc >= THRESHOLD_DARK_ON) {
      isLightOn = true;
      Serial.printf("[EVENT %6lu ms] >>> DUSK DETECTED! Turning Light ON.\n", now);
    }
  } else {
    if (rawAdc <= THRESHOLD_BRIGHT_OFF) {
      isLightOn = false;
      Serial.printf("[EVENT %6lu ms] >>> DAWN DETECTED! Turning Light OFF.\n", now);
    }
  }

  // Thuật toán Ambient Auto-Dimming
  if (isLightOn) {
    // Độ sáng tăng mượt mà khi môi trường càng tối dần
    long mappedDuty = map(rawAdc, THRESHOLD_BRIGHT_OFF, 4095, 800, PWM_MAX_DUTY);
    currentDuty = constrain(mappedDuty, 0, (long)PWM_MAX_DUTY);
    ledcWrite(LEDC_CHANNEL, currentDuty);
  } else {
    currentDuty = 0;
    ledcWrite(LEDC_CHANNEL, 0);
  }

  // Định kỳ in thông tin trạng thái
  if (now - lastLogTime >= LOG_INTERVAL_MS) {
    lastLogTime = now;
    uint8_t percent = (currentDuty * 100) / PWM_MAX_DUTY;

    // Thanh visual bar
    char bar[11];
    uint8_t barLength = percent / 10;
    for (uint8_t i = 0; i < 10; i++) {
      bar[i] = (i < barLength) ? '#' : '-';
    }
    bar[10] = '\0';

    Serial.printf("[%6lu ms] ADC:%4u | %1.2fV | DO:%d | State:%-3s | Duty:%4u [%s] %3u%%\n",
                  now,
                  rawAdc,
                  voltage,
                  digitalState,
                  isLightOn ? "ON " : "OFF",
                  currentDuty,
                  bar,
                  percent);
  }

  delay(20); // Chu kỳ lấy mẫu 20ms
}
