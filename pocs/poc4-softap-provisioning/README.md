# POC 4 — ESP32 SoftAP Wi-Fi provisioning

Tiến độ, giới hạn kiểm chứng và bước tiếp theo được ghi tại
[STATUS.md](STATUS.md).

Project độc lập này biến ESP32 classic thành cổng cấu hình: thiết bị luôn phát
SoftAP `ESP32-SETUP-<suffix>`, phục vụ form tại `http://192.168.4.1`, đồng thời
dùng STA để thử Wi-Fi do người dùng nhập. Đây không phải router/repeater: không
có NAT hoặc packet forwarding và client SoftAP có thể báo **No Internet**.

## Nền tảng đã khóa

- PlatformIO Core 6.1.19
- Espressif32 platform 7.0.1
- Arduino-ESP32 2.0.17
- `board = esp32dev`, framework Arduino, Serial 115200
- Wokwi for VS Code 3.6.0

Project chỉ dùng `WiFi`, `WebServer`, `Preferences` và `HTTPClient` có sẵn trong
Arduino-ESP32; không có `lib_deps`. `CORE_DEBUG_LEVEL=0` được đặt rõ trong
`platformio.ini` vì parser WebServer 2.0.17 có verbose log chứa raw form fields.
Firmware cũng dùng raw-body callback giới hạn kích thước và không log body, nên
password không xuất hiện trong Serial, HTML, JSON hay query string.

## Cấu hình POC

Các hằng số nằm trong `include/provisioning_config.h`:

- setup password: `configure-me` (chỉ phù hợp POC có kiểm soát);
- SoftAP channel khởi tạo 1, không ẩn, tối đa hai client;
- body form tối đa 256 byte;
- SSID sau URL decode: 1–32 byte, không có control byte;
- password sau URL decode: rỗng cho mạng open hoặc 8–63 ký tự ASCII printable;
- timeout STA 20 giây; fallback sau 3 giây;
- upstream probe timeout 5 giây.

Trong sản phẩm thật, setup password phải riêng theo thiết bị và được cung cấp
qua nhãn/QR; cần thêm proof-of-possession, rate limit, CSRF protection, NVS/flash
encryption, secure boot và cơ chế recovery vật lý.

## Luồng firmware

`POST /api/wifi/configure` chỉ kiểm tra content type/kích thước, URL-decode và
validate hai field duy nhất, copy credentials vào vùng pending, xóa raw body rồi
trả `202`. Handler không chờ Wi-Fi, DHCP hay HTTP. Vòng `loop()` bắt đầu attempt
ở lượt kế tiếp và điều khiển state machine:

```text
provisioning -> connecting -> verifying -> connected
                      |             |
                      +-----------> failed -> stored-config fallback
```

Sau khi STA có IP, `HTTPClient::GET()` kiểm tra endpoint JSONPlaceholder. Probe
là lời gọi đồng bộ có timeout và có thể dừng `loop()`/web serving tối đa khoảng
5 giây, nhưng nó chỉ chạy sau khi configure handler đã trả `202`. Đây là giới
hạn có chủ đích của POC dùng thư viện đồng bộ.

Credentials pending chỉ được ghi sau DHCP **và** HTTP 200. Cấu hình cũ trong NVS
không bị đụng tới trong lúc thử pending; nếu pending thất bại, firmware xóa nó
khỏi trạng thái logic RAM, giữ AP/web server và reconnect cấu hình cũ sau 3 giây.

Khi pending đã được xác minh, luồng Preferences là:

1. ghi `valid=false`;
2. ghi `ssid` và `pass`, rồi đọc lại cả hai;
3. ghi `valid=true` cuối cùng làm commit marker.

Preferences commit từng key, nên đây không phải transaction nguyên tử nhiều
key. Mất điện trong bước ghi cuối có thể để marker `false`; lần boot sau firmware
sẽ vào provisioning thay vì dùng dữ liệu dở dang. Nếu NVS lỗi sau khi marker bị
hạ, fallback RAM có thể hoạt động trong boot hiện tại nhưng cấu hình cũ không
còn được bảo đảm qua reboot. Production nên dùng hai slot/version hoặc transaction
scheme mạnh hơn.

`WiFi.persistent(false)` ngăn Wi-Fi driver tự ghi pending credentials trước khi
probe. Endpoint reset đặt marker false, xóa hai key và disconnect STA nhưng giữ
SoftAP hoạt động.

## HTTP API

| Method | Path | Response |
|---|---|---|
| `GET` | `/` | `200 text/html`, frontend nằm trong flash `PROGMEM` |
| `GET` | `/api/status` | `200 application/json`, không có password |
| `POST` | `/api/wifi/configure` | `202`, hoặc `400`/`409` khi request sai/bận |
| `POST` | `/api/wifi/reset` | `200`, hoặc `500` nếu không cập nhật được NVS |
| bất kỳ | route khác | `404 application/json` |

Mọi response có `Cache-Control: no-store`. Frontend poll status mỗi giây, xóa
password input ngay khi tạo request và bắt lỗi fetch khi AP+STA đổi channel làm
client tạm mất kết nối.

## Build và kiểm tra tĩnh

Từ repository root:

```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc4-softap-provisioning/diagram.json", "utf8"))'
pio run -d pocs/poc4-softap-provisioning -e esp32dev
test -f pocs/poc4-softap-provisioning/.pio/build/esp32dev/firmware.elf
test -f pocs/poc4-softap-provisioning/.pio/build/esp32dev/firmware.bin
pio pkg list -d pocs/poc4-softap-provisioning -e esp32dev
```

Nếu có Wokwi CLI:

```bash
cd pocs/poc4-softap-provisioning
wokwi-cli lint
```

## Wokwi chỉ kiểm chứng một phần

Build firmware trước, chọn `pocs/poc4-softap-provisioning/wokwi.toml` bằng
**Wokwi: Select Config File**, rồi start simulator và giữ tab hiển thị. Serial
nằm trong terminal tích hợp `Wokwi Terminal`; RFC2217 dùng `localhost:4004`.
Private Gateway forward web server tới `http://localhost:8184`.

Qua form forwarded có thể submit `Wokwi-GUEST` với password rỗng để thử STA và
probe. Host truy cập port forward không có nghĩa host đã join SoftAP. Wokwi
không cho adapter Wi-Fi thật của điện thoại/laptop tham gia RF SoftAP mô phỏng;
do đó kết quả này chỉ là **partial simulation**. Diagram không thêm
`wokwi-wifi-ap`, vì custom AP là tính năng Hobby+/Pro và `Wokwi-GUEST` đủ cho
gate một phần.

## Nghiệm thu bắt buộc trên board thật

Build thành công hoặc truy cập port forward chưa hoàn thành POC. Cần ESP32
classic, một client join SoftAP và router/hotspot Internet riêng:

```bash
pio device list
pio run -d pocs/poc4-softap-provisioning -e esp32dev \
  -t upload --upload-port <PORT>
pio device monitor -p <PORT> -b 115200
```

Sau đó:

1. xác nhận Serial có `PROVISIONING_AP_STARTED` và `HTTP_SERVER_STARTED`;
2. join `ESP32-SETUP-<suffix>` bằng setup password và tải `192.168.4.1`;
3. nhập credential sai, thấy `failed` nhưng form/AP vẫn dùng được;
4. nhập credential đúng, thấy `STA_GOT_IP`, `UPSTREAM_PROBE status=200`, rồi
   `CREDENTIALS_SAVED`;
5. kiểm tra `/api/status` có AP/STA IP nhưng tuyệt đối không có password;
6. reboot, xác nhận tự reconnect từ NVS;
7. gọi reset, reboot và xác nhận trở lại provisioning.

Không đặt password Wi-Fi thật trực tiếp trong shell history ở máy dùng chung;
ưu tiên form HTML. Một điện thoại thường không nên vừa phát hotspot upstream
vừa làm client SoftAP ESP32.

## Nguồn chính thức

- [Arduino-ESP32 Wi-Fi API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html)
- [WiFiAccessPoint example 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WiFi/examples/WiFiAccessPoint/WiFiAccessPoint.ino)
- [Wi-Fi events example 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WiFi/examples/WiFiClientEvents/WiFiClientEvents.ino)
- [WebServer API 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WebServer/src/WebServer.h)
- [Preferences/NVS tutorial](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html)
- [Wokwi ESP32 Wi-Fi](https://docs.wokwi.com/guides/esp32-wifi)
- [Wokwi Wi-Fi AP part](https://docs.wokwi.com/parts/wokwi-wifi-ap)
- [Wokwi VS Code project config](https://docs.wokwi.com/vscode/project-config)
- [PlatformIO ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)
