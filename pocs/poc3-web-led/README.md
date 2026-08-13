# POC 3 — Điều khiển LED qua ESP32 HTTP server

Tiến độ, phần đã xác minh và bước tiếp theo được ghi tại [STATUS.md](STATUS.md).

Project PlatformIO độc lập này phục vụ frontend HTML/CSS/JavaScript trực tiếp
từ flash (`PROGMEM`). Frontend đọc trạng thái GPIO2 bằng API trước khi hiển thị
và chỉ cập nhật sau response thành công.

## API

| Method | Path | Response |
|---|---|---|
| `GET` | `/` | `200 text/html` |
| `GET` | `/api/led` | `200 application/json` |
| `POST` | `/api/led/on` | `200 application/json`, GPIO2 `HIGH` |
| `POST` | `/api/led/off` | `200 application/json`, GPIO2 `LOW` |
| bất kỳ | route khác | `404 application/json` |

Các endpoint `on` và `off` đặt trạng thái đích nên có tính idempotent.

## Build và chạy trên Wokwi

```bash
pio run -d pocs/poc3-web-led -e esp32dev
```

Nếu `pio` không có trong `PATH`, dùng
`$HOME/.platformio/penv/bin/pio`. Sau đó mở
`pocs/poc3-web-led/diagram.json`, chọn đúng
`pocs/poc3-web-led/wokwi.toml` bằng lệnh **Wokwi: Select Config File**, rồi chạy
**Wokwi: Start Simulator**.

Firmware mặc định dùng access point mở `Wokwi-GUEST`. Private IoT Gateway sẽ
forward HTTP port 80 của ESP32 tới <http://localhost:8180>. Với Wokwi for VS
Code 3.6.0 đang dùng trong repository, gateway là WebAssembly component được
bundle trong extension và tự khởi động từ `[[net.forward]]`; không cần chạy
gateway standalone ở port 9011 hay chọn lệnh `Enable Private Gateway`.

Xem [tài liệu Private Gateway](docs/PRIVATE-GATEWAY.md) để hiểu đường đi của
request, cấu hình đúng phiên bản, kiểm chứng runtime và chẩn đoán port/target.

Khi simulator và Private Gateway đang chạy:

```bash
curl -i http://localhost:8180/api/led
curl -i -X POST http://localhost:8180/api/led/on
curl -i http://localhost:8180/api/led
curl -i -X POST http://localhost:8180/api/led/off
curl -i http://localhost:8180/not-found
```

Quan sát đồng thời LED trong simulator và log request trong **Wokwi Terminal**.
RFC2217 cho Serial tự động nằm ở `localhost:4000`.

## Board thật và secrets

Sao chép `include/secrets.example.h` thành `include/secrets.h`, rồi thay SSID và
password. `include/secrets.h` đã được Git ignore và không được commit. Mở URL
mà firmware in qua Serial, ví dụ `http://192.168.1.50/`.

Server POC không có TLS, authentication hoặc authorization; không expose trực
tiếp ra Internet.

## Nguồn chính thức

- [Arduino-ESP32 2.0.17 HelloServer](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WebServer/examples/HelloServer/HelloServer.ino)
- [Arduino-ESP32 2.0.17 WebServer API](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WebServer/src/WebServer.h)
- [Arduino-ESP32 GPIO API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/gpio.html)
- [Wokwi project config và port forwarding](https://docs.wokwi.com/vscode/project-config)
- [Wokwi ESP32 Wi-Fi](https://docs.wokwi.com/guides/esp32-wifi)
- [Wokwi LED reference](https://docs.wokwi.com/parts/wokwi-led)
- [PlatformIO ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)
