// Chương trình đầu tiên: nháy LED trên board

#include <Arduino.h> // nạp các hàm có sẵn của framework Arduino

void setup()
{
    pinMode(2, OUTPUT);   // "cấu hình" chân LED thành ngõ ra
    Serial.begin(115200); // mở "kênh nói chuyện" với máy tính
}

void loop()
{
    digitalWrite(2, HIGH);          // bật LED
    delay(1000);                    // chờ 1 giây
    digitalWrite(2, LOW);           // tắt LED
    delay(1000);                    // chờ 1 giây
    Serial.println("Hello ESP32!"); // in chữ ra Serial Monitor
}
