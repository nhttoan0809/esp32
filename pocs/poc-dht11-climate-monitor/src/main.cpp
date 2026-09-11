#include <Arduino.h>
#include <DHT.h>

/**
 * POC: DHT11 Climate Monitor & Mold Alert System
 * Board: ESP32 DevKit V1 (30 chân)
 * 
 * Sơ đồ chân:
 * - DHT Data   -> GPIO 19 (1-Wire Single Bus)
 * - LED Green  -> GPIO 23 (Comfort: 20-30°C, 40-70% RH)
 * - LED Yellow -> GPIO 25 (Mold Alert: RH >= 75%)
 * - LED Red    -> GPIO 26 (Heat Alert: T >= 35°C hoặc T <= 16°C)
 */

// Định nghĩa chân
static const uint8_t PIN_DHT_DATA    = 19;
static const uint8_t PIN_LED_COMFORT = 23; // Xanh lá
static const uint8_t PIN_LED_MOLD    = 25; // Vàng (Cảnh báo nồm ẩm)
static const uint8_t PIN_LED_HEAT    = 26; // Đỏ (Cảnh báo quá nhiệt)

// Trên Wokwi mô phỏng dùng DHT22, trên board thật dùng DHT11
// Thư viện Adafruit hỗ trợ chuyển đổi linh hoạt
#if defined(WOKWI_SIMULATION) || !defined(REAL_HARDWARE_DHT11)
  #define SENSOR_DHT_TYPE DHT22
#else
  #define SENSOR_DHT_TYPE DHT11
#endif

static DHT dht(PIN_DHT_DATA, SENSOR_DHT_TYPE);

static unsigned long lastSampleTime = 0;
static const unsigned long SAMPLE_INTERVAL_MS = 2000; // Tần số lấy mẫu an toàn: 2.0s

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.println(F("[SYSTEM] POC DHT11 Climate Monitor Initialized"));
  Serial.printf("[CONFIG] DHT Data: GPIO %u (Type: %s)\n", PIN_DHT_DATA, (SENSOR_DHT_TYPE == DHT22 ? "DHT22/Wokwi" : "DHT11"));
  Serial.println(F("[CONFIG] LED Comfort: GPIO 23 | Mold: GPIO 25 | Heat: GPIO 26"));
  Serial.println(F("=================================================="));

  pinMode(PIN_LED_COMFORT, OUTPUT);
  pinMode(PIN_LED_MOLD, OUTPUT);
  pinMode(PIN_LED_HEAT, OUTPUT);

  // Tắt toàn bộ LED ban đầu
  digitalWrite(PIN_LED_COMFORT, LOW);
  digitalWrite(PIN_LED_MOLD, LOW);
  digitalWrite(PIN_LED_HEAT, LOW);

  dht.begin();
}

void loop() {
  unsigned long now = millis();

  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature(); // Độ C

    // Kiểm tra tính toàn vẹn dữ liệu
    if (isnan(humidity) || isnan(temperature)) {
      Serial.printf("[%6lu ms] [ERROR] Không đọc được dữ liệu từ cảm biến DHT!\n", now);
      return;
    }

    // Tính toán chỉ số nhiệt cảm nhận (Heat Index)
    float heatIndex = dht.computeHeatIndex(temperature, humidity, false);

    bool isComfort = false;
    bool isMoldAlert = false;
    bool isHeatAlert = false;

    // Phân loại mức độ tiểu khí hậu
    if (temperature >= 35.0f || temperature <= 16.0f) {
      isHeatAlert = true;
    }

    if (humidity >= 75.0f) {
      isMoldAlert = true;
    }

    if (!isHeatAlert && !isMoldAlert && temperature >= 20.0f && temperature <= 30.0f && humidity >= 40.0f && humidity <= 70.0f) {
      isComfort = true;
    }

    // Điều khiển đèn LED tương ứng
    digitalWrite(PIN_LED_COMFORT, isComfort ? HIGH : LOW);
    digitalWrite(PIN_LED_MOLD, isMoldAlert ? HIGH : LOW);
    digitalWrite(PIN_LED_HEAT, isHeatAlert ? HIGH : LOW);

    // Xác định nhãn trạng thái
    const char* statusStr = "MODERATE";
    if (isComfort) statusStr = "COMFORT [OK]";
    else if (isHeatAlert && isMoldAlert) statusStr = "EXTREME DANGER";
    else if (isHeatAlert) statusStr = "HEAT ALERT [HOT/COLD]";
    else if (isMoldAlert) statusStr = "MOLD ALERT [HUMID]";

    Serial.printf("[%6lu ms] Temp: %4.1f°C | Humid: %4.1f%% | HeatIdx: %4.1f°C | Status: %s\n",
                  now,
                  temperature,
                  humidity,
                  heatIndex,
                  statusStr);
  }
}
