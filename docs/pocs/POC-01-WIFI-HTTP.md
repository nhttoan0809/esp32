# POC 1 — ESP32 kết nối Wi-Fi và gọi HTTP API

## 1. Mục tiêu

POC này kiểm chứng ESP32 có thể:

1. kết nối vào một mạng Wi-Fi ở chế độ Station;
2. nhận địa chỉ IP qua DHCP;
3. gọi một HTTP API bằng phương thức `GET`;
4. in HTTP status và response body ra Serial;
5. phát hiện mất Wi-Fi, timeout hoặc server không phản hồi mà không làm firmware
   treo vĩnh viễn.

POC chỉ đọc dữ liệu từ server. Việc ESP32 tự mở HTTP server để nhận lệnh được
thực hiện trong POC 3.

## 2. Nền tảng mục tiêu

```ini
platform = espressif32@7.0.1
board = esp32dev
framework = arduino
monitor_speed = 115200
```

Phiên bản đã kiểm tra trong repository:

- PlatformIO Core 6.1.19;
- Espressif32 platform 7.0.1;
- Arduino-ESP32 core 2.0.17;
- Wokwi for VS Code 3.6.0;
- ESP32 Dev Module, chip ESP32 classic.

POC sử dụng các thư viện `WiFi` và `HTTPClient` có sẵn trong Arduino-ESP32,
không cần thêm `lib_deps`.

## 3. Kiến trúc

```text
ESP32 ── Wi-Fi ── HTTP GET ──> JSONPlaceholder public API
  │                                  │
  └──── Serial 115200 <──────────────┘
       HTTP status + response body
```

API công khai được chọn cho POC:

```http
GET http://jsonplaceholder.typicode.com/todos/1 HTTP/1.1
Accept: application/json
```

JSONPlaceholder là API giả lập công khai dành cho thử nghiệm. Endpoint này
không cần API key, request body hoặc mock server do repository tự chạy.

Response thành công dự kiến:

```http
HTTP/1.1 200 OK
Content-Type: application/json
```

```json
{
  "userId": 1,
  "id": 1,
  "title": "delectus aut autem",
  "completed": false
}
```

POC cố ý dùng HTTP để tập trung vào kết nối Wi-Fi, gửi request và đọc response.
Không dùng endpoint này cho dữ liệu nhạy cảm hoặc hệ thống production. HTTPS,
CA certificate và đồng bộ thời gian sẽ là bước nâng cấp riêng sau khi hoàn
thành luồng HTTP cơ bản.

## 4. Cấu hình Wi-Fi

### 4.1. Board ESP32 thật

ESP32 kết nối vào Wi-Fi nội bộ bằng SSID và password thật. Sau khi nhận IP,
ESP32 truy cập JSONPlaceholder qua Internet. Mạng nội bộ phải cho phép thiết bị
đi ra Internet và phân giải DNS.

Thông tin bí mật được đặt trong `include/secrets.h`, không commit vào Git:

```cpp
#pragma once

#define WIFI_SSID "your-wifi-name"
#define WIFI_PASSWORD "your-wifi-password"
```

Repository chỉ lưu `include/secrets.example.h` với giá trị mẫu.

### 4.2. Wokwi

Simulator kết nối vào access point ảo:

```cpp
WiFi.begin("Wokwi-GUEST", "", 6);
```

`Wokwi-GUEST` không phải Wi-Fi vật lý trong nhà hoặc văn phòng. Có hai đường
truy cập mạng khác với board thật. POC sử dụng Public Gateway mặc định để gọi
JSONPlaceholder trên Internet nên không cần Private Gateway hoặc server local.

Không gửi password, token hoặc dữ liệu nhạy cảm qua Wokwi Public Gateway.

## 5. Cấu trúc project dự kiến

```text
pocs/poc1-wifi-http/
├── platformio.ini
├── wokwi.toml
├── diagram.json
├── include/
│   └── secrets.example.h
├── src/
│   └── main.cpp
└── README.md
```

URL public API có thể là hằng số trong firmware:

```cpp
constexpr char API_URL[] =
    "http://jsonplaceholder.typicode.com/todos/1";
```

## 6. Luồng firmware

### `setup()`

1. Mở `Serial` ở 115200 baud.
2. Chọn cấu hình Wi-Fi cho board thật hoặc Wokwi.
3. Đặt ESP32 ở `WIFI_STA`.
4. Bắt đầu kết nối với timeout hữu hạn.
5. Khi thành công, in IP và RSSI; tuyệt đối không in password.

### `loop()`

1. Kiểm tra `WiFi.status()`.
2. Nếu mất mạng, thực hiện reconnect có khoảng nghỉ.
3. Khi đến chu kỳ request, tạo `HTTPClient` và đặt timeout.
4. Gọi `GET`.
5. In HTTP status và body khi thành công.
6. In chuỗi lỗi do `HTTPClient::errorToString()` khi thất bại.
7. Luôn gọi `http.end()` để giải phóng kết nối.
8. Lập lịch bằng `millis()` thay vì `delay()` dài.

Log dự kiến:

```text
WIFI_CONNECTING ssid=Wokwi-GUEST
WIFI_CONNECTED ip=10.10.0.2 rssi=-42
HTTP_REQUEST method=GET url=http://...
HTTP_RESPONSE status=200 content_type=application/json
HTTP_BODY {"userId":1,"id":1,"title":"delectus aut autem","completed":false}
```

Khi API hoặc Internet không hoạt động:

```text
HTTP_ERROR code=-1 message=connection refused
HTTP_RETRY_IN_MS 5000
```

## 7. Tiêu chí nghiệm thu

POC chỉ hoàn thành khi quan sát được output runtime, không chỉ build thành công:

- ESP32 in `WIFI_CONNECTED` cùng địa chỉ IP hợp lệ;
- request trả HTTP `200`;
- Serial hiển thị response JSON chứa `id`, `title` và `completed`;
- khi API hoặc Internet không phản hồi, firmware in lỗi rõ ràng và tiếp tục
  chu kỳ retry mà không reset;
- mất Wi-Fi và kết nối lại không làm vòng lặp bị treo;
- log không chứa Wi-Fi password;
- không cần khởi động mock server hoặc cài dependency phía máy tính.

## 8. Cổng kiểm chứng

```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc1-wifi-http/diagram.json", "utf8"))'

pio run -d pocs/poc1-wifi-http -e esp32dev

test -f pocs/poc1-wifi-http/.pio/build/esp32dev/firmware.elf
test -f pocs/poc1-wifi-http/.pio/build/esp32dev/firmware.bin
```

Nếu `wokwi-cli` có sẵn:

```bash
cd pocs/poc1-wifi-http
wokwi-cli lint
```

Sau đó phải start simulator, giữ tab hiển thị và đọc output thực tế qua Wokwi
Terminal hoặc RFC2217. Nếu kiểm thử board thật, dùng đúng cổng USB của board với
`pio device monitor` ở 115200 baud.

## 9. Nguồn chính thức

- [Arduino-ESP32 Wi-Fi API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html)
- [HTTPClient example của Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/HTTPClient/examples/BasicHttpClient/BasicHttpClient.ino)
- [JSONPlaceholder Guide](https://jsonplaceholder.typicode.com/guide/)
- [Wokwi ESP32 Wi-Fi Networking](https://docs.wokwi.com/guides/esp32-wifi)
- [Wokwi VS Code project config](https://docs.wokwi.com/vscode/project-config)
- [PlatformIO ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)

## 10. Trạng thái

Firmware, cấu hình Wokwi và HTTP client đã được triển khai. Build và lint pass;
runtime trước bản sửa CRLF đã quan sát Wi-Fi connect, HTTP 200 và JSON. Cần chạy
lại firmware cuối bằng Wokwi CLI và fault injection cho retry trước khi kết luận
nghiệm thu đầy đủ. Xem `pocs/poc1-wifi-http/STATUS.md`.
