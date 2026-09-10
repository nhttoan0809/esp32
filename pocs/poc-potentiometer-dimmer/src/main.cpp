#include <Arduino.h>

/**
 * ==============================================================================
 * 🌟 POC: Potentiometer LED Dimmer (Điều Chỉnh Độ Sáng LED Bằng Biến Trở)
 * ==============================================================================
 * Phần cứng sử dụng (ESP32 DevKit V1 30-pin & Kit tiêu chuẩn):
 * - Biến trở xoay 10kΩ: Chân 1 nối 3V3, Chân 3 nối GND, Chân giữa (Wiper) nối GPIO 34 (ADC1).
 * - Điện trở hạn dòng: 220Ω (mắc nối tiếp vào Anode LED Đỏ từ GPIO 18).
 * - LED Đỏ: Cathode nối GND chung.
 *
 * Thông số kỹ thuật đã tính toán:
 * - Dòng điện cực đại của LED (100% PWM): I_LED = (3.3V - 2.0V) / 220Ω ≈ 5.91 mA.
 * - Nguồn cấp biến trở: 3.3V (dòng tĩnh 0.33 mA, công suất tiêu tán 1.09 mW).
 * - Dải điều khiển biến trở: 0Ω -> 10kΩ (tương ứng 0.0V -> 3.3V, Duty 0% -> 100%).
 * ==============================================================================
 */

// Cấu hình chân phần cứng
constexpr uint8_t POT_ADC_PIN = 34;   // Kênh ADC1_CH6 (Input-only, không bị nhiễu Wi-Fi/Strapping)
constexpr uint8_t LED_PIN     = 18;   // Ngõ ra LEDC PWM nối qua trở thuần 220Ω

// Cấu hình bộ tạo xung phần cứng LEDC PWM
constexpr uint8_t  PWM_CHANNEL    = 0;       // Kênh PWM 0
constexpr uint32_t PWM_FREQ       = 5000;    // Tần số 5 kHz (mắt người không thấy nhấp nháy)
constexpr uint8_t  PWM_RESOLUTION = 12;      // Độ phân giải 12-bit (0 - 4095, khớp 1:1 với ADC 12-bit)
constexpr uint32_t PWM_MAX_DUTY   = 4095;    // (1 << 12) - 1

// Ngưỡng lọc vùng chết (Dead-zone Thresholds) loại bỏ nhiễu sàn & bão hòa ADC
constexpr uint16_t ADC_DEADZONE_LOW  = 35;    // Dưới 35 (~0.03V) ép tắt ngắt hoàn toàn (Duty = 0)
constexpr uint16_t ADC_DEADZONE_HIGH = 4050;  // Trên 4050 (~3.25V) ép sáng 100% tối đa (Duty = 4095)

// Hệ số lọc làm mịn tín hiệu EMA (Exponential Moving Average)
constexpr float EMA_ALPHA = 0.25f;
static float s_emaFilteredAdc = 0.0f;

// Thời gian chu kỳ in Serial Monitor
static uint32_t s_lastLogTime = 0;
constexpr uint32_t LOG_INTERVAL_MS = 150;

/**
 * @brief Tạo chuỗi thanh tiến trình trực quan ASCII
 */
void getProgressBar(char* buffer, size_t bufferSize, float percentage) {
  constexpr uint8_t BAR_LENGTH = 10;
  uint8_t filledCount = (uint8_t)roundf((percentage / 100.0f) * BAR_LENGTH);
  if (filledCount > BAR_LENGTH) filledCount = BAR_LENGTH;

  size_t idx = 0;
  buffer[idx++] = '[';
  for (uint8_t i = 0; i < BAR_LENGTH; ++i) {
    if (i < filledCount) {
      // Ký tự block đặc (UTF-8: 0xE2 0x96 0x88 -> "█")
      buffer[idx++] = '#';
    } else {
      buffer[idx++] = '-';
    }
  }
  buffer[idx++] = ']';
  buffer[idx] = '\0';
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // Cấu hình chân ADC
  analogReadResolution(12);
  pinMode(POT_ADC_PIN, INPUT);

  // Cấu hình bộ điều chế độ rộng xung LEDC PWM
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0); // Mặc định ban đầu tắt LED

  // Lấy mẫu khởi tạo cho bộ lọc EMA
  s_emaFilteredAdc = (float)analogRead(POT_ADC_PIN);

  // In banner thông số kỹ thuật rõ ràng
  Serial.println();
  Serial.println("==================================================================");
  Serial.println("💡 ESP32 POTENTIOMETER LED DIMMER - SPECIFICATION & CONTROL DEMO");
  Serial.println("==================================================================");
  Serial.printf("[SETUP] GPIO%u (ADC1) : Biến trở 10kΩ (Nguồn cấp 3.3V, dải 0 - 3.3V)\r\n", POT_ADC_PIN);
  Serial.printf("[SETUP] GPIO%u (LEDC) : LED Đỏ qua trở thuần 220Ω (5 kHz, 12-bit PWM)\r\n", LED_PIN);
  Serial.println("[SETUP] I_LED Max     : ~5.91 mA (ở 100% Duty Cycle)");
  Serial.println("[SETUP] Dead-zones    : ADC <= 35 -> TẮT HẲN | ADC >= 4050 -> SÁNG NHẤT");
  Serial.println("------------------------------------------------------------------");
}

void loop() {
  // 1. Đọc giá trị ADC thô từ biến trở (12-bit: 0 - 4095)
  uint16_t rawAdc = analogRead(POT_ADC_PIN);

  // 2. Lọc làm mịn tín hiệu chống rung giật (EMA Filter)
  s_emaFilteredAdc = (EMA_ALPHA * (float)rawAdc) + ((1.0f - EMA_ALPHA) * s_emaFilteredAdc);
  uint16_t filteredAdc = (uint16_t)roundf(s_emaFilteredAdc);

  // 3. Xử lý vùng chết & Ánh xạ sang PWM Duty Cycle
  uint32_t duty = 0;
  if (filteredAdc <= ADC_DEADZONE_LOW) {
    duty = 0; // Đảm bảo tắt ngắt hoàn toàn 0% dòng điện
  } else if (filteredAdc >= ADC_DEADZONE_HIGH) {
    duty = PWM_MAX_DUTY; // Đảm bảo đạt độ sáng tối đa 100%
  } else {
    // Nội suy tuyến tính dải giữa
    duty = (uint32_t)map(filteredAdc, ADC_DEADZONE_LOW, ADC_DEADZONE_HIGH, 0, PWM_MAX_DUTY);
  }

  // 4. Xuất xung PWM điều khiển bóng đèn LED
  ledcWrite(PWM_CHANNEL, duty);

  // 5. Tính toán các chỉ số điện học và in Serial Monitor định kỳ
  uint32_t now = millis();
  if (now - s_lastLogTime >= LOG_INTERVAL_MS) {
    s_lastLogTime = now;

    float percentage = ((float)duty / (float)PWM_MAX_DUTY) * 100.0f;
    float voltageIn  = ((float)filteredAdc / 4095.0f) * 3.3f;
    // Dòng điện LED tức thời: (3.3V - 2.0V) / 220Ω * (Duty / 4095)
    float currentMa  = (duty == 0) ? 0.0f : (1.3f / 220.0f * 1000.0f * ((float)duty / (float)PWM_MAX_DUTY));

    char progressBar[16];
    getProgressBar(progressBar, sizeof(progressBar), percentage);

    const char* statusStr = "DIMMING";
    if (duty == 0) {
      statusStr = "OFF 🌑";
    } else if (duty == PWM_MAX_DUTY) {
      statusStr = "MAX 🌕";
    }

    Serial.printf("[%6lu ms] %s %5.1f%% | ADC:%4u | Vin:%4.2fV | Duty:%4u | I_LED:%4.2fmA | [%s]\r\n",
                  now, progressBar, percentage, filteredAdc, voltageIn, (uint16_t)duty, currentMa, statusStr);
  }

  delay(15); // Chu kỳ trễ lấy mẫu mượt mà ~66Hz
}
