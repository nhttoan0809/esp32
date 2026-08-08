# Blink LED — Project PlatformIO đầu tiên

Chương trình nhỏ nhất để làm quen ESP32: bật/tắt LED tích hợp mỗi 1 giây.

## Yêu cầu

- VS Code + extension **PlatformIO IDE** (đã cài)
- Board **ESP32 DevKit** (hiện chưa có — chưa upload được)

## Build

1. Mở thư mục `blink` này trong VS Code (`File > Open Folder`)
2. Chờ PlatformIO nhận diện project (~vài giây)
3. Bấm nút **Build (✓)** ở thanh trạng thái màu xanh phía dưới
4. Thấy `SUCCESS` trong terminal là xong

Lần build đầu tiên sẽ tự tải framework espressif32 (~300MB) nên có thể mất vài phút.

## Upload (khi có board)

1. Cắm board vào máy bằng cáp USB
2. Kiểm tra VS Code nhận cổng (bấm **→ Upload** trên thanh trạng thái)
3. LED tích hợp trên board nháy nhịp 1 giây

> Nếu Upload lỗi liên quan cổng USB: cần cài driver USB-UART
> (CH340 hoặc CP210x) tuỳ chip trên board.

## Cấu trúc

```
blink/
├── platformio.ini   # Cấu hình: board esp32dev, framework arduino
├── src/
│   └── main.cpp     # Code: setup() + loop() — Arduino-style
└── README.md
```

## Tham khảo

- [PlatformIO docs](https://docs.platformio.org/)
- [Arduino core cho ESP32](https://github.com/espressif/arduino-esp32)
