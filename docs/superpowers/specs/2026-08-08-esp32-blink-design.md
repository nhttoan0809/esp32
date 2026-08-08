# Design: ESP32 Blink LED — Project PlatformIO đầu tiên

**Ngày:** 2026-08-08
**Trạng thái:** Đã duyệt

## Mục đích

Dự án nhỏ nhất để người mới học ESP32 làm quen với:
- Cấu trúc một project PlatformIO chuẩn trong VS Code
- Thao tác Build/Upload từ thanh trạng thái PlatformIO
- Cảm giác "code chạy trên phần cứng" khi có board

## Phần cứng

- **Board:** ESP32 DevKit (chip ESP32-WROOM-32) — bảng mạch chuẩn 38 chân.
  *Lưu ý: hiện chưa có board vật lý; project được tạo trước, upload khi có board.*
- **LED tích hợp:** đa số board DevKit có LED ở GPIO2; dùng macro `LED_BUILTIN`
  của framework Arduino để tự động chọn đúng chân.

## Công cụ

- VS Code + extension **PlatformIO IDE** (đã cài, Core 6.1.19 hoạt động)
- Framework: **Arduino** (`framework = arduino`) — API `setup()`/`loop()` đơn giản,
  phù hợp người mới. (Framework thay thế: ESP-IDF — mạnh hơn nhưng phức tạp hơn,
  không cần thiết cho bước đầu.)

## Cấu trúc dự án

```
esp32-learning/
└── blink/
    ├── platformio.ini        # Cấu hình: board, framework, baud rate serial
    ├── src/
    │   └── main.cpp          # Code Blink LED
    └── README.md             # Hướng dẫn build/upload (tiếng Việt)
```

## platformio.ini

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
```

## src/main.cpp — hành vi

- `setup()`: khởi tạo chân LED là output.
- `loop()`: bật LED 1 giây → tắt LED 1 giây → lặp vô hạn.
- Không cần thêm thư viện nào; không có linh kiện ngoài.

## Thành công

- Người dùng mở thư mục `blink` trong VS Code
- Bấm **Build (✓)** trên thanh trạng thái PlatformIO → build thành công
- (Khi có board:) cắm USB → bấm **Upload (→)** → LED nháy nhịp 1 giây
- (Khi có board:) có thể mở Serial Monitor để xem log

## Ngoài phạm vi (sẽ làm khi có board)

- Upload firmware lên board
- Cài driver USB-UART (CH340 / CP210x) nếu macOS chưa nhận
- Các chương trình tiếp theo (Serial print, WiFi, cảm biến...)
