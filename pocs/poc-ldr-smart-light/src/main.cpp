#include <Arduino.h>

/**
 * POC: LDR Smart Light Controller (Module LDR 3-Pin DO-GND-VCC)
 * Board: ESP32 DevKit V1 (30 chân)
 * 
 * Sơ đồ chân phần cứng thực tế:
 * - LDR Module Pin 1 (Trái): DO  -> GPIO 35 (ADC1_CH7, Input-Only)
 * - LDR Module Pin 2 (Giữa): GND -> GND chung
 * - LDR Module Pin 3 (Phải): VCC -> 3V3 ESP32
 * - LED Chiếu sáng: Anode (+) qua trở 220 Ohm -> GPIO 18 (LEDC PWM), Cathode (-) -> GND
 * 
 * Cơ chế hoạt động:
 * - Module 3 chân sử dụng IC so sánh LM393 và biến trở vi chỉnh (Trimpot màu xanh)
 *   để phân biệt Sáng / Tối trực tiếp thành mức logic Digital (DO).
 * - Time-based Hysteresis / Stability Filter: Trạng thái Sáng/Tối phải duy trì liên tục
 *   đủ 1500ms mới kích hoạt chuyển trạng thái, loại bỏ hoàn toàn hiện tượng nhấp nháy
 *   khi có bóng mờ lướt qua hoặc ánh sáng dao động mấp mé ngưỡng.
 * - Smooth PWM Fading: Đèn LED sáng dần (Fade In) và mờ dần (Fade Out) trong 1.0 giây
 *   thông qua LEDC PWM (5 kHz, 12-bit) mang lại cảm giác êm dịu và thẩm mỹ.
 */

// Định nghĩa chân
static const uint8_t PIN_LDR_DO = 35;
static const uint8_t PIN_LED_LIGHT = 18;

// Cực tính ngõ ra LM393:
// Thông thường: Khi trời tối / che LDR -> DO = HIGH (LED tín hiệu trên module tắt)
//              Khi trời sáng -> DO = LOW (LED tín hiệu trên module sáng)
static const uint8_t DARK_ACTIVE_LEVEL = HIGH;

// Cấu hình LEDC PWM (5 kHz, 12-bit)
static const uint8_t LEDC_CHANNEL = 0;
static const uint32_t LEDC_FREQ_HZ = 5000;
static const uint8_t LEDC_RESOLUTION_BITS = 12; // 0 - 4095
static const uint32_t PWM_MAX_DUTY = 4095;

// Cấu hình thời gian
static const unsigned long STABILITY_WINDOW_MS = 1500; // Thời gian lọc chống chập chờn (1.5s)
static const unsigned long FADE_DURATION_MS = 1000;     // Thời gian chuyển sáng dần/tối dần (1.0s)
static const unsigned long LOG_INTERVAL_MS = 500;       // Chu kỳ in Serial log (500ms)

// Trạng thái FSM của đèn chiếu sáng
enum LightState {
  LIGHT_OFF,
  LIGHT_FADING_IN,
  LIGHT_ON,
  LIGHT_FADING_OUT
};

// Biến trạng thái
static LightState currentLightState = LIGHT_OFF;
static uint32_t currentDuty = 0;

static int stablePinState = -1;
static int lastRawPinState = -1;
static unsigned long pinStateChangedTime = 0;
static unsigned long fadeStartTime = 0;
static unsigned long lastLogTime = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F("[SYSTEM] POC LDR Smart Light Initialized"));
  Serial.println(F("[HARDWARE] Sensor: 3-Pin LDR Module (DO - GND - VCC)"));
  Serial.println(F("[CONFIG] DO Pin: GPIO 35 (Input-Only LM393 TTL)"));
  Serial.println(F("[CONFIG] LED Pin: GPIO 18 (LEDC PWM 5kHz 12-bit)"));
  Serial.printf("[CONFIG] Stability Window: %lu ms | Fade Duration: %lu ms\n",
                STABILITY_WINDOW_MS, FADE_DURATION_MS);
  Serial.println(F("=================================================="));

  // Cấu hình chân ngõ vào
  pinMode(PIN_LDR_DO, INPUT);

  // Cấu hình LEDC PWM cho đèn LED
  ledcSetup(LEDC_CHANNEL, LEDC_FREQ_HZ, LEDC_RESOLUTION_BITS);
  ledcAttachPin(PIN_LED_LIGHT, LEDC_CHANNEL);
  ledcWrite(LEDC_CHANNEL, 0);

  // Đọc khởi tạo trạng thái ban đầu
  int initialRead = digitalRead(PIN_LDR_DO);
  stablePinState = initialRead;
  lastRawPinState = initialRead;
  pinStateChangedTime = millis();

  if (stablePinState == DARK_ACTIVE_LEVEL) {
    currentLightState = LIGHT_ON;
    currentDuty = PWM_MAX_DUTY;
    ledcWrite(LEDC_CHANNEL, currentDuty);
    Serial.println(F("[BOOT] Initial Environment: DARK -> Light set to ON"));
  } else {
    currentLightState = LIGHT_OFF;
    currentDuty = 0;
    ledcWrite(LEDC_CHANNEL, currentDuty);
    Serial.println(F("[BOOT] Initial Environment: BRIGHT -> Light set to OFF"));
  }
}

void loop() {
  unsigned long now = millis();

  // 1. Đọc ngõ ra số từ module LDR
  int rawDo = digitalRead(PIN_LDR_DO);

  // 2. Bộ lọc ổn định thời gian (Time-based Stability Filter / Debounce)
  if (rawDo != lastRawPinState) {
    lastRawPinState = rawDo;
    pinStateChangedTime = now;
  }

  // Nếu tín hiệu giữ nguyên trạng thái liên tục vượt quá STABILITY_WINDOW_MS
  if ((now - pinStateChangedTime >= STABILITY_WINDOW_MS) && (rawDo != stablePinState)) {
    stablePinState = rawDo;

    if (stablePinState == DARK_ACTIVE_LEVEL) {
      // Xác nhận trời tối ổn định -> Kích hoạt Bật đèn sáng dần
      currentLightState = LIGHT_FADING_IN;
      fadeStartTime = now;
      Serial.printf("[EVENT %6lu ms] >>> DUSK CONFIRMED! Fading Light IN.\n", now);
    } else {
      // Xác nhận trời sáng ổn định -> Kích hoạt Tắt đèn mờ dần
      currentLightState = LIGHT_FADING_OUT;
      fadeStartTime = now;
      Serial.printf("[EVENT %6lu ms] >>> DAWN CONFIRMED! Fading Light OUT.\n", now);
    }
  }

  // 3. FSM Điều khiển Fade In / Fade Out mượt mà qua PWM
  switch (currentLightState) {
    case LIGHT_FADING_IN: {
      unsigned long elapsed = now - fadeStartTime;
      if (elapsed >= FADE_DURATION_MS) {
        currentDuty = PWM_MAX_DUTY;
        currentLightState = LIGHT_ON;
        Serial.printf("[EVENT %6lu ms] >>> Light reached FULL BRIGHTNESS.\n", now);
      } else {
        currentDuty = (PWM_MAX_DUTY * elapsed) / FADE_DURATION_MS;
      }
      ledcWrite(LEDC_CHANNEL, currentDuty);
      break;
    }

    case LIGHT_FADING_OUT: {
      unsigned long elapsed = now - fadeStartTime;
      if (elapsed >= FADE_DURATION_MS) {
        currentDuty = 0;
        currentLightState = LIGHT_OFF;
        Serial.printf("[EVENT %6lu ms] >>> Light COMPLETELY TURNED OFF.\n", now);
      } else {
        currentDuty = PWM_MAX_DUTY - ((PWM_MAX_DUTY * elapsed) / FADE_DURATION_MS);
      }
      ledcWrite(LEDC_CHANNEL, currentDuty);
      break;
    }

    case LIGHT_ON:
      currentDuty = PWM_MAX_DUTY;
      ledcWrite(LEDC_CHANNEL, currentDuty);
      break;

    case LIGHT_OFF:
    default:
      currentDuty = 0;
      ledcWrite(LEDC_CHANNEL, currentDuty);
      break;
  }

  // 4. In thông tin định kỳ mỗi 500ms
  if (now - lastLogTime >= LOG_INTERVAL_MS) {
    lastLogTime = now;
    uint8_t percent = (currentDuty * 100) / PWM_MAX_DUTY;

    // Chuỗi trạng thái FSM
    const char* stateStr = "OFF";
    if (currentLightState == LIGHT_FADING_IN) stateStr = "FADE-IN";
    else if (currentLightState == LIGHT_ON) stateStr = "ON ";
    else if (currentLightState == LIGHT_FADING_OUT) stateStr = "FADE-OUT";

    // Thanh visual bar 10 ký tự
    char bar[11];
    uint8_t barLength = percent / 10;
    for (uint8_t i = 0; i < 10; i++) {
      bar[i] = (i < barLength) ? '#' : '-';
    }
    bar[10] = '\0';

    const char* envStr = (rawDo == DARK_ACTIVE_LEVEL) ? "DARK  " : "BRIGHT";

    Serial.printf("[%6lu ms] DO:%d (%s) | Light:%-8s | Duty:%4u [%s] %3u%%\n",
                  now,
                  rawDo,
                  envStr,
                  stateStr,
                  currentDuty,
                  bar,
                  percent);
  }

  delay(20); // Chu kỳ lấy mẫu 20ms
}
