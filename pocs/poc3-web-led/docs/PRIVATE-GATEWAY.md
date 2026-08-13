# Wokwi Private IoT Gateway cho POC 3

Tài liệu này mô tả đúng flow đã kiểm chứng với Wokwi for VS Code 3.6.0. Mục
tiêu là cho browser hoặc công cụ trên máy host gọi HTTP server port 80 nằm bên
trong ESP32 mô phỏng.

## Cơ chế

ESP32 kết nối access point mở `Wokwi-GUEST`. Đây là mạng mô phỏng, không phải
Wi-Fi LAN của máy host và không dùng SSID/password thật. Wokwi Public Gateway
cho ESP32 tạo kết nối ra Internet, nhưng không tạo đường incoming từ host vào
HTTP server mô phỏng. `[[net.forward]]` yêu cầu Private IoT Gateway mở đường
ngược đó:

```text
http://127.0.0.1:8180
  -> local TCP listener của extension
  -> WokwiGW WebAssembly được bundle trong Wokwi for VS Code 3.6.0
  -> private network của simulator
  -> target:80
  -> WebServer chạy trên ESP32
```

Frontend và API vẫn cùng origin vì browser tải trang tại `localhost:8180` và
JavaScript gọi URL tương đối như `/api/led/on`. Không cần CORS hay preflight.

## Cấu hình

`wokwi.toml` của POC chứa:

```toml
[wokwi]
version = 1
elf = ".pio/build/esp32dev/firmware.elf"
firmware = ".pio/build/esp32dev/firmware.bin"
rfc2217ServerPort = 4000

[[net.forward]]
from = "localhost:8180"
to = "target:80"
```

- `from` là listener chỉ trên máy local. Port 8180 phải chưa được process khác
  sử dụng.
- `to = "target:80"` chọn port 80 của MCU mục tiêu. Không thay `target` bằng IP
  DHCP được in ra Serial.
- RFC2217 port 4000 chỉ dùng cho UART/Serial; nó độc lập với HTTP port 8180.
- Không thêm `[net].gateway` cho flow này. Trường đó dùng khi chủ động kết nối
  tới một external WebSocket gateway khác.

## Bundled gateway và flow standalone cũ

| Flow | Wokwi VS Code 3.6.0 của repo | Hướng dẫn browser/standalone cũ |
|---|---|---|
| Gateway process | WebAssembly bundle trong extension | executable `wokwigw` riêng |
| Cách bật | tự động từ `[[net.forward]]` | chạy binary và chọn Enable |
| Port điều khiển | không có port 9011 riêng | gateway thường nghe port 9011 |
| Port HTTP local | do `from` quyết định, ở đây 8180 | ví dụ tài liệu thường dùng 9080 |

Repository phải tuân theo cột Wokwi VS Code 3.6.0. Changelog và source được cài
cục bộ là nguồn quyết định khi khác với ảnh chụp hoặc hướng dẫn của phiên bản
cũ. Trong lần kiểm chứng này, gateway hoạt động với license hiển thị là
`Community License`; không suy diễn yêu cầu gói trả phí từ flow browser cũ.

## Chạy và kiểm chứng

Build firmware trước:

```bash
$HOME/.platformio/penv/bin/pio run \
  -d pocs/poc3-web-led \
  -e esp32dev
```

Mở folder `pocs/poc3-web-led`, chạy **Wokwi: Start Simulator** và giữ tab
simulator hiển thị. Chờ Wokwi Terminal có:

```text
WIFI_CONNECTED ip=10.13.37.2
HTTP_SERVER_STARTED port=80
```

Sau đó kiểm tra:

```bash
curl http://127.0.0.1:8180/api/led
curl -X POST http://127.0.0.1:8180/api/led/on
curl -X POST http://127.0.0.1:8180/api/led/off
curl -i http://127.0.0.1:8180/not-found
```

Kết quả cần là `false -> true -> false`, route cuối trả HTTP 404, LED trong sơ
đồ sáng rồi tắt và Wokwi Terminal ghi nhận đủ request. Có thể mở
<http://127.0.0.1:8180> để kiểm tra frontend bằng browser.

## Kết quả đã quan sát

Ngày 2026-08-13, với PlatformIO Core 6.1.19, Espressif32 7.0.1,
Arduino-ESP32 2.0.17, PlatformIO IDE 3.3.4 và Wokwi Simulator 3.6.0:

```text
GET  /api/led     -> 200 {"device":"esp32dev","led":{"pin":2,"on":false}}
POST /api/led/on  -> 200 {"device":"esp32dev","led":{"pin":2,"on":true}}
POST /api/led/off -> 200 {"device":"esp32dev","led":{"pin":2,"on":false}}
GET  /not-found   -> 404 {"error":"not_found"}
```

LED mô phỏng đã được quan sát sáng sau `POST /api/led/on` và tắt sau
`POST /api/led/off`. Serial thực tế trong Wokwi Terminal cũng ghi các dòng
`HTTP_REQUEST` tương ứng.

## Chẩn đoán

Kiểm tra listener:

```bash
lsof -nP -iTCP:8180 -sTCP:LISTEN
lsof -nP -iTCP:4000 -sTCP:LISTEN
```

- Không có listener 8180: xác nhận đang dùng đúng `wokwi.toml`, firmware đã
  build và simulator được start từ folder POC 3.
- `address already in use`: dừng simulator/process đang giữ port hoặc chọn một
  local port trống trong `from`.
- `connection reset` sau restart nóng: kiểm tra Serial. Nếu ESP32 nhận `.3`
  thay vì target mặc định `.2`, dừng simulator, chạy **Developer: Reload
  Window**, rồi chạy lại **Wokwi: Start Simulator**.
- Request treo khi đổi tab/window: đưa tab Wokwi Simulator ra trước; simulator
  có thể pause khi tab bị ẩn.
- PlatformIO Serial Monitor mở `/dev/cu.*`, `/dev/tty*` hoặc `COM*`: đó là cổng
  thiết bị thật. Dùng Wokwi Terminal hoặc RFC2217 tại localhost:4000.

Listener hiện bind vào loopback và server không có TLS/authentication. Không
đổi sang địa chỉ public hoặc expose port này ra Internet.

## Nguồn chính thức và nguồn đúng phiên bản

- [Wokwi VS Code project config và `net.forward`](https://docs.wokwi.com/vscode/project-config)
- [Wokwi ESP32 Wi-Fi và Private Gateway](https://docs.wokwi.com/guides/esp32-wifi)
- package, changelog, schema và `dist/extension.js` của Wokwi Simulator 3.6.0
  đang cài trên máy kiểm chứng
- [PlatformIO ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)
