# POC 1 — ESP32 Wi-Fi Station và HTTP GET

Tiến độ, phần chưa xác minh và bước tiếp theo được ghi tại [STATUS.md](STATUS.md).

POC độc lập này kết nối ESP32 classic vào Wi-Fi ở chế độ Station, nhận IP qua
DHCP và gọi định kỳ:

```http
GET http://jsonplaceholder.typicode.com/todos/1
Accept: application/json
```

Firmware in trạng thái Wi-Fi, HTTP status, content type và response body qua
Serial 115200. Mọi lần chờ kết nối đều có timeout; khi mất Wi-Fi hoặc request
lỗi, vòng lặp tiếp tục và thử lại sau 5 giây.

> Endpoint dùng HTTP không mã hóa chỉ để kiểm chứng luồng mạng cơ bản. Không gửi
> password, token hoặc dữ liệu nhạy cảm qua endpoint hay Wokwi Public Gateway.

## Phiên bản mục tiêu

- PlatformIO Core 6.1.19
- Espressif32 platform 7.0.1
- Arduino-ESP32 2.0.17
- `board = esp32dev` (ESP32 classic)
- Wokwi for VS Code 3.6.0

`WiFi` và `HTTPClient` nằm sẵn trong Arduino-ESP32 nên project không có
`lib_deps`.

## Chạy bằng Wokwi

Nếu `include/secrets.h` không tồn tại, firmware tự dùng access point mở
`Wokwi-GUEST`, password rỗng và channel 6. Không cần mock server hay Private
Gateway.

```bash
pio run -d pocs/poc1-wifi-http -e esp32dev
```

Trong VS Code, mở `pocs/poc1-wifi-http/diagram.json`, chạy **Wokwi: Select
Config File** và chọn `pocs/poc1-wifi-http/wokwi.toml`, rồi chạy **Wokwi: Start
Simulator**. Giữ tab simulator hiển thị và đọc log trong terminal tích hợp tên
`Wokwi Terminal`. RFC2217 của POC này ở `localhost:4001`:

```python
import serial

port = serial.serial_for_url(
    "rfc2217://localhost:4001",
    baudrate=115200,
    timeout=5,
)
print(port.readline().decode(errors="replace"))
port.close()
```

Output thành công dự kiến có dạng:

```text
WIFI_CONNECTING ssid=Wokwi-GUEST
WIFI_CONNECTED ip=10.10.0.2 rssi=-42
HTTP_REQUEST method=GET url=http://jsonplaceholder.typicode.com/todos/1
HTTP_RESPONSE status=200 content_type=application/json; charset=utf-8
HTTP_BODY { ... "id": 1, ... "completed": false }
HTTP_NEXT_IN_MS 15000
```

Địa chỉ IP, RSSI, định dạng JSON và tham số của `Content-Type` có thể khác.

## Chạy trên board thật

Tạo file local từ template rồi thay SSID/password:

```bash
cp pocs/poc1-wifi-http/include/secrets.example.h \
  pocs/poc1-wifi-http/include/secrets.h
```

`include/secrets.h` đã được `.gitignore` loại trừ và firmware không in password.
Khi file này tồn tại, build dùng Wi-Fi thật thay cho `Wokwi-GUEST`. Xóa hoặc đổi
tên file trước khi build lại cho Wokwi.

```bash
pio run -d pocs/poc1-wifi-http -e esp32dev -t upload
pio device monitor -b 115200
```

Chỉ dùng `pio device monitor` với cổng USB của board thật; Serial ảo của Wokwi
được xem bằng `Wokwi Terminal` hoặc RFC2217 như trên.

## Hành vi lỗi

- Kết nối Wi-Fi quá 15 giây: `WIFI_CONNECT_TIMEOUT`, sau đó thử lại sau 5 giây.
- Mất Wi-Fi: `WIFI_DISCONNECTED`, sau đó bắt đầu lại kết nối có timeout.
- DNS, TCP hoặc HTTP timeout: `HTTP_ERROR code=... message=...`, sau đó request
  lại sau 5 giây.
- HTTP response khác 200 vẫn được in để chẩn đoán và được coi là thất bại.

`HTTPClient::end()` được gọi trên mọi nhánh sau khi tạo request. Các timeout
HTTP là 5 giây và lịch request dùng `millis()`; không có `delay()` dài làm treo
vòng lặp.

## Kiểm chứng tĩnh và build

Chạy từ repository root:

```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc1-wifi-http/diagram.json", "utf8"))'
pio run -d pocs/poc1-wifi-http -e esp32dev
test -f pocs/poc1-wifi-http/.pio/build/esp32dev/firmware.elf
test -f pocs/poc1-wifi-http/.pio/build/esp32dev/firmware.bin
```

Nếu đã cài Wokwi CLI:

```bash
cd pocs/poc1-wifi-http
wokwi-cli lint
```

Build thành công chưa đủ để nghiệm thu runtime. Cần quan sát `WIFI_CONNECTED`,
HTTP 200 và JSON trên Serial. Để kiểm tra retry, tạm ngắt Internet/gateway hoặc
dùng một endpoint không phản hồi, xác nhận log lỗi tiếp tục lặp mà board không
reset, rồi khôi phục cấu hình cuối cùng và build lại.

## Nguồn chính thức

- [Arduino-ESP32 Wi-Fi API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html)
- [WiFi STA example — Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WiFi/examples/WiFiClient/WiFiClient.ino)
- [HTTPClient example — Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/HTTPClient/examples/BasicHttpClient/BasicHttpClient.ino)
- [Wokwi ESP32 Wi-Fi Networking](https://docs.wokwi.com/guides/esp32-wifi)
- [Wokwi VS Code project config](https://docs.wokwi.com/vscode/project-config)
- [PlatformIO ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)
- [JSONPlaceholder guide](https://jsonplaceholder.typicode.com/guide/)
