/**
 * @file main.cpp
 * @brief Chương trình nháy đèn LED cơ bản (Blink LED) trên ESP32 DevKit V1
 * @author toannguyen
 */

#include <Arduino.h>

// Định nghĩa chân GPIO kết nối với LED
// GPIO 2 là chân LED tích hợp (Onboard LED) trên ESP32 DevKit V1
// và cũng được nối ra LED rời ngoài trên mạch Wokwi
constexpr uint8_t LED_PIN = 2;

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
    pinMode(LED_PIN, OUTPUT);

    Serial.println("========================================");
    Serial.println("   ESP32 DevKit V1 - Basic LED Blink    ");
    Serial.println("========================================");
    Serial.printf("[INIT] LED configured on GPIO %d\n", LED_PIN);
    Serial.printf("[INIT] Blink interval: %d ms\n", BLINK_INTERVAL_MS);
}

void loop()
{
    blinkCount++;

    // 1. Bật đèn LED (mức logic HIGH - 3.3V)
    digitalWrite(LED_PIN, HIGH);
    Serial.printf("[%lu ms] Cycle #%u - [LED] State: ON\n", (unsigned long)millis(), blinkCount);
    delay(BLINK_INTERVAL_MS);

    // 2. Tắt đèn LED (mức logic LOW - 0V)
    digitalWrite(LED_PIN, LOW);
    Serial.printf("[%lu ms] Cycle #%u - [LED] State: OFF\n", (unsigned long)millis(), blinkCount);
    delay(BLINK_INTERVAL_MS);
}

