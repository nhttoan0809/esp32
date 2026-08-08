/*
 * Blink LED — chương trình "Hello World" của ESP32
 *
 * Bật LED tích hợp 1 giây, tắt 1 giây, lặp vô hạn.
 * LED_BUILTIN được framework Arduino tự chọn đúng chân cho từng board.
 */

#include <Arduino.h>

void setup() {
  // Khai báo chân LED là ngõ ra
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // Bật LED
  delay(1000);                      // Chờ 1 giây
  digitalWrite(LED_BUILTIN, LOW);   // Tắt LED
  delay(1000);                      // Chờ 1 giây
}
