# POC 3 — Web frontend điều khiển LED qua ESP32 HTTP server

## 1. Mục tiêu

POC này kiểm chứng một luồng điều khiển thiết bị hoàn chỉnh:

1. ESP32 kết nối Wi-Fi;
2. ESP32 mở HTTP server ở port 80;
3. ESP32 trả một trang web gồm HTML, CSS và JavaScript;
4. trang web có công tắc toggle;
5. JavaScript gọi HTTP API khi người dùng đổi công tắc;
6. ESP32 thay đổi GPIO2;
7. LED vật lý hoặc LED trong Wokwi thực sự bật/tắt;
8. frontend đọc lại trạng thái thật từ ESP32 thay vì tự giả định thành công.

## 2. Nền tảng mục tiêu

```ini
platform = espressif32@7.0.1
board = esp32dev
framework = arduino
monitor_speed = 115200
```

POC dùng các thành phần có sẵn trong Arduino-ESP32 2.0.17:

- `WiFi`;
- `WebServer`;
- GPIO API;
- `PROGMEM` để nhúng frontend vào firmware.

Không cần web framework, JSON parser hay asynchronous server bên thứ ba.

## 3. Kiến trúc

```text
Browser
  │
  ├── GET / ───────────────────────────┐
  ├── GET /api/led                     │
  ├── POST /api/led/on                 ▼
  └── POST /api/led/off         ESP32 WebServer :80
                                         │
                                         ├── Serial log
                                         └── GPIO2 ──> LED
```

Frontend và API được phục vụ từ cùng ESP32 và cùng origin. JavaScript dùng URL
tương đối, ví dụ:

```js
fetch('/api/led/on', { method: 'POST' });
```

Thiết kế này không cần CORS và không cần xử lý preflight.

## 4. API contract

| Method | Path | Chức năng | Response |
|---|---|---|---|
| `GET` | `/` | Trả frontend | `200 text/html` |
| `GET` | `/api/led` | Đọc trạng thái thật | `200 application/json` |
| `POST` | `/api/led/on` | Đặt GPIO2 `HIGH` | `200 application/json` |
| `POST` | `/api/led/off` | Đặt GPIO2 `LOW` | `200 application/json` |
| bất kỳ | route khác | Không tìm thấy | `404 application/json` |

Response trạng thái:

```json
{
  "device": "esp32dev",
  "led": {
    "pin": 2,
    "on": true
  }
}
```

Hai endpoint `on` và `off` là idempotent: gọi `on` nhiều lần vẫn giữ LED bật,
không làm trạng thái đảo qua lại ngoài ý muốn.

## 5. Frontend

Trang web tối thiểu gồm:

- tiêu đề thiết bị;
- badge Online/Offline;
- công tắc toggle;
- text `Đèn đang bật` hoặc `Đèn đang tắt`;
- vùng hiển thị lỗi request.

Luồng JavaScript:

1. Khi trang load, gọi `GET /api/led`.
2. Đồng bộ toggle theo response của ESP32.
3. Khi người dùng đổi toggle, disable control tạm thời.
4. Gửi `POST /api/led/on` hoặc `POST /api/led/off`.
5. Chỉ cập nhật trạng thái sau response `200`.
6. Nếu lỗi, hiển thị lỗi và gọi lại `GET /api/led` để phục hồi trạng thái thật.

Toggle cần có label và `aria-pressed` để có thể sử dụng bằng bàn phím và công
cụ hỗ trợ accessibility.

Frontend được nhúng trong `include/web_ui.h` dưới dạng raw string `PROGMEM`.
CSS nằm trong `<style>` và JavaScript nằm trong `<script>`. Cách này vẫn chạy
HTML/CSS/JS trong browser nhưng không cần LittleFS hoặc một filesystem image
riêng mà baseline Wokwi chưa cấu hình.

## 6. Firmware

### `setup()`

1. Đặt GPIO2 là output và khởi tạo `LOW`.
2. Mở Serial ở 115200.
3. Kết nối Wi-Fi ở `WIFI_STA`.
4. In địa chỉ IP sau khi kết nối.
5. Đăng ký frontend route và API routes.
6. Đăng ký JSON 404 handler.
7. Gọi `server.begin()`.

### `loop()`

Gọi liên tục:

```cpp
server.handleClient();
delay(2);
```

Không dùng delay dài vì `WebServer` cần được phục vụ thường xuyên.

Mỗi API handler phải:

1. thay đổi GPIO nếu cần;
2. cập nhật biến trạng thái trong firmware;
3. in request và trạng thái mới ra Serial;
4. trả JSON phản ánh trạng thái sau khi `digitalWrite()`.

Log dự kiến:

```text
WIFI_CONNECTED ip=10.10.0.2
HTTP_SERVER_STARTED port=80
HTTP_REQUEST method=GET path=/api/led status=200 led=off
HTTP_REQUEST method=POST path=/api/led/on status=200 led=on
HTTP_REQUEST method=POST path=/api/led/off status=200 led=off
```

## 7. Cấu trúc project dự kiến

```text
pocs/poc3-web-led/
├── platformio.ini
├── wokwi.toml
├── diagram.json
├── include/
│   ├── secrets.example.h
│   └── web_ui.h
├── src/
│   └── main.cpp
└── README.md
```

Project độc lập giúp Wokwi đọc đúng firmware của POC 3 và không ghi đè
`diagram.json` hiện có ở repository root.

## 8. Sơ đồ Wokwi

POC chỉ dùng các part và pin sau:

- `board-esp32-devkit-c-v4`;
- một `wokwi-led`;
- `esp:2` nối tới `led:A`;
- `led:C` nối tới `esp:GND`;
- `esp:TX` nối `$serialMonitor:RX`;
- `esp:RX` nối `$serialMonitor:TX`.

GPIO trong diagram phải khớp tuyệt đối với `LED_PIN = 2` trong source code.

## 9. Truy cập server trong Wokwi

ESP32 mô phỏng kết nối `Wokwi-GUEST` và lắng nghe port 80. Wokwi Public Gateway
không cho browser tạo incoming connection vào ESP32. Với Private Gateway, thêm
port forwarding vào `wokwi.toml`:

```toml
[[net.forward]]
from = "localhost:8180"
to = "target:80"
```

Sau khi simulator chạy và tab vẫn hiển thị, mở:

```text
http://localhost:8180
```

Private Gateway hiện yêu cầu gói Wokwi trả phí và không được hỗ trợ trên Safari.
Nếu không có Private Gateway, Wokwi Public Gateway không thể dùng để nghiệm thu
browser → ESP32 cho POC này. Trên board thật, mở trực tiếp địa chỉ IP mà ESP32
in ra Serial, ví dụ `http://192.168.1.50/`.

Không expose POC server trực tiếp ra Internet: thiết kế cơ bản này chưa có TLS,
authentication hoặc authorization.

## 10. Tiêu chí nghiệm thu

POC chỉ hoàn thành khi quan sát được cả HTTP output và LED thật sự đổi trạng
thái:

- browser tải được frontend từ ESP32;
- `GET /api/led` trả `on: false` sau boot;
- nút bật tạo HTTP `200` và response `on: true`;
- LED GPIO2 thực sự sáng;
- nút tắt tạo HTTP `200` và response `on: false`;
- LED GPIO2 thực sự tắt;
- Serial ghi nhận đúng các request và trạng thái;
- reload trang đọc lại đúng trạng thái hiện tại;
- route không tồn tại trả JSON `404`;
- khi request lỗi, frontend không hiển thị trạng thái thành công giả.

## 11. Cổng kiểm chứng

```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc3-web-led/diagram.json", "utf8"))'

pio run -d pocs/poc3-web-led -e esp32dev

test -f pocs/poc3-web-led/.pio/build/esp32dev/firmware.elf
test -f pocs/poc3-web-led/.pio/build/esp32dev/firmware.bin
```

Nếu `wokwi-cli` có sẵn:

```bash
cd pocs/poc3-web-led
wokwi-cli lint
```

Kiểm tra API sau khi simulator và Private Gateway đã chạy:

```bash
curl -i http://localhost:8180/api/led
curl -i -X POST http://localhost:8180/api/led/on
curl -i http://localhost:8180/api/led
curl -i -X POST http://localhost:8180/api/led/off
curl -i http://localhost:8180/api/led
curl -i http://localhost:8180/not-found
```

Kết quả JSON phải lần lượt phản ánh `false → true → false`. Đồng thời phải quan
sát LED trong simulator sáng rồi tắt và kiểm tra log trong Wokwi Terminal hoặc
qua RFC2217.

## 12. Nguồn chính thức

- [Arduino-ESP32 2.0.17 HelloServer](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WebServer/examples/HelloServer/HelloServer.ino)
- [Arduino-ESP32 2.0.17 WebServer API](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WebServer/src/WebServer.h)
- [Arduino-ESP32 GPIO API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/gpio.html)
- [Wokwi project config và port forwarding](https://docs.wokwi.com/vscode/project-config)
- [Wokwi ESP32 Wi-Fi Networking](https://docs.wokwi.com/guides/esp32-wifi)
- [Wokwi LED reference](https://docs.wokwi.com/parts/wokwi-led)
- [PlatformIO ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)

## 13. Trạng thái

Tài liệu thiết kế đã có. Frontend, firmware và API chưa được triển khai hoặc xác
minh runtime.
