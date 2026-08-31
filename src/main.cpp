/**
 * @file main.cpp
 * @brief Chương trình nháy đèn LED cơ bản (Blink LED) trên ESP32 DevKit V1
 * @author toannguyen
 */

#include <Arduino.h>

// Định nghĩa chân GPIO kết nối với LED
// GPIO 2 là chân LED tích hợp (Onboard LED) trên ESP32 DevKit V1
// và cũng được nối ra LED rời ngoài trên mạch Wokwi
constexpr uint8_t LED_PIN_1 = 18;
constexpr uint8_t LED_PIN_2 = 19;

// Thời gian trễ giữa các lần bật / tắt (milliseconds)
constexpr uint32_t BLINK_INTERVAL_MS = 1000;

// Biến đếm số chu kỳ nháy
uint32_t blinkCount = 0;

void setup()
{
    // Khởi tạo giao tiếp Serial với tốc độ baud 115200
    Serial.begin(115200);

    // Chờ một chút để Serial ổn định khi khởi động
    delay(500);

    // Cấu hình chân GPIO của LED làm ngõ ra (OUTPUT)
    pinMode(LED_PIN_1, OUTPUT);
    pinMode(LED_PIN_2, OUTPUT);

    Serial.println("========================================");
    Serial.println("   ESP32 DevKit V1 - Basic LED Blink    ");
    Serial.println("========================================");
    Serial.printf("[INIT] LED_PIN_1 configured on GPIO %d\n", LED_PIN_1);
    Serial.printf("[INIT] LED_PIN_2 configured on GPIO %d\n", LED_PIN_2);
    Serial.printf("[INIT] Blink interval: %d ms\n", BLINK_INTERVAL_MS);
}

void loop()
{
    blinkCount++;
    // Pha 1: LED 1 BẬT (Vàng), LED 2 TẮT (Xanh lá)
    digitalWrite(LED_PIN_1, HIGH);
    digitalWrite(LED_PIN_2, LOW);
    Serial.printf("[%lu ms] Cycle #%u - LED 1 (D%d): ON  | LED 2 (D%d): OFF\n", 
                  (unsigned long)millis(), blinkCount, LED_PIN_1, LED_PIN_2);
    // Serial.printf("[%lu ms] Cycle #%u - LED 1 (D%d): ON \n", 
    //               (unsigned long)millis(), blinkCount, LED_PIN_1);
    delay(BLINK_INTERVAL_MS);
    // Pha 2: LED 1 TẮT (Vàng), LED 2 BẬT (Xanh lá)
    digitalWrite(LED_PIN_1, LOW);
    digitalWrite(LED_PIN_2, HIGH);
    Serial.printf("[%lu ms] Cycle #%u - LED 1 (D%d): OFF | LED 2 (D%d): ON\n", 
                  (unsigned long)millis(), blinkCount, LED_PIN_1, LED_PIN_2);
    // Serial.printf("[%lu ms] Cycle #%u - LED 1 (D%d): OFF \n", 
    //               (unsigned long)millis(), blinkCount, LED_PIN_1);
    delay(BLINK_INTERVAL_MS);
}


