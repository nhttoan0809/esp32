#include <Arduino.h>

// Định nghĩa các chân LED khớp với diagram.json
constexpr uint8_t LED_YELLOW_PIN = 18;
constexpr uint8_t LED_GREEN_PIN = 19;

void setup() {
  Serial.begin(115200);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);

  // In banner khởi động rõ ràng
  Serial.println();
  Serial.println("=================================================");
  Serial.println("🚀 ESP32 DevKit V1 - Dual LED & Serial Log Demo");
  Serial.println("=================================================");
  Serial.printf("[SETUP] GPIO%u: LED Vàng (Yellow) [Qua trở 220Ω]\r\n", LED_YELLOW_PIN);
  Serial.printf("[SETUP] GPIO%u: LED Xanh lá (Green) [Qua trở 220Ω]\r\n", LED_GREEN_PIN);
  Serial.println("-------------------------------------------------");
}

void loop() {
  const uint32_t now = millis();

  // Pha 1: Bật LED Vàng (GPIO18), Tắt LED Xanh (GPIO19)
  digitalWrite(LED_YELLOW_PIN, HIGH);
  digitalWrite(LED_GREEN_PIN, LOW);
  Serial.printf("[%6lu ms] [LED STATUS] 🟡 GPIO%u (Yellow): ON  | 🟢 GPIO%u (Green): OFF\r\n",
                now, LED_YELLOW_PIN, LED_GREEN_PIN);
  delay(1000);

  // Pha 2: Tắt LED Vàng (GPIO18), Bật LED Xanh (GPIO19)
  digitalWrite(LED_YELLOW_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, HIGH);
  Serial.printf("[%6lu ms] [LED STATUS] 🟡 GPIO%u (Yellow): OFF | 🟢 GPIO%u (Green): ON\r\n",
                millis(), LED_YELLOW_PIN, LED_GREEN_PIN);
  delay(1000);
}
