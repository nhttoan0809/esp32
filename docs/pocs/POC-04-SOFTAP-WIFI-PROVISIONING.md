# POC 4 — ESP32 SoftAP và Wi-Fi provisioning bằng web form

## 1. Mục tiêu

POC này chứng minh ESP32 có thể đóng vai trò là một **cổng cấu hình thiết bị**:

1. ESP32 tạo một Wi-Fi SoftAP riêng để điện thoại hoặc máy tính kết nối;
2. ESP32 cấp địa chỉ IP cho client và phục vụ một trang HTML tại
   `http://192.168.4.1`;
3. người dùng nhập SSID và password của Wi-Fi thực tế;
4. frontend gửi credentials tới API trên ESP32;
5. ESP32 giữ SoftAP hoạt động và đồng thời thử kết nối Wi-Fi thực tế bằng
   interface Station;
6. chỉ sau khi nhận địa chỉ IP thành công, ESP32 mới lưu credentials vào NVS;
7. sau khi reboot, ESP32 đọc cấu hình đã lưu và kết nối lại;
8. nếu credentials sai, cổng cấu hình vẫn hoạt động để người dùng thử lại.

## 2. Phạm vi của từ “gateway”

Trong POC này, ESP32 là **provisioning/configuration gateway**: nó cung cấp một
điểm truy cập và web UI để cấu hình chính ESP32 hoặc thiết bị chứa ESP32.

POC không biến ESP32 thành router, repeater hoặc Internet gateway cho điện thoại:

- không bật NAT;
- không forward packet giữa interface AP và STA;
- client nối vào SoftAP có thể báo `No Internet`;
- mục tiêu là truyền credentials vào thiết bị, không chia sẻ Internet cho client.

Nếu cần Wi-Fi repeater/NAT router, đó phải là một POC khác với routing, DHCP,
DNS, firewall và security riêng.

## 3. Nền tảng mục tiêu

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

POC dùng các thư viện có sẵn trong Arduino-ESP32:

- `WiFi` cho SoftAP, Station và Wi-Fi events;
- `WebServer` cho HTML và HTTP API;
- `Preferences` để lưu cấu hình nhỏ trong NVS;
- `HTTPClient` cho upstream connectivity probe sau khi STA có IP.

Không cần thư viện provisioning hoặc web server bên thứ ba.

## 4. Kiến trúc AP+STA

```text
Điện thoại/laptop
      │
      │ kết nối Wi-Fi ESP32-SETUP-<suffix>
      │ HTTP http://192.168.4.1
      ▼
┌──────────────────── ESP32 ────────────────────┐
│                                              │
│  SoftAP interface             STA interface  │
│  192.168.4.1                  DHCP từ router │
│       │                              │       │
│       ▼                              ▼       │
│  WebServer :80       Wi-Fi router/hotspot    │
│       │                              │       │
│       ├── HTML form                  └── Internet probe
│       ├── provisioning APIs                  │
│       └── status API                         │
│                                              │
│  Preferences/NVS: credentials đã xác minh    │
└──────────────────────────────────────────────┘
```

Firmware chạy ở chế độ:

```cpp
WiFi.mode(WIFI_AP_STA);
```

ESP32 chỉ có một radio Wi-Fi. Trong AP+STA, SoftAP và STA phải dùng chung
channel. Khi STA kết nối router ở channel khác, SoftAP có thể chuyển channel và
client cấu hình có thể mất kết nối trong chốc lát. Frontend phải chịu được việc
reload hoặc reconnect.

## 5. SoftAP của thiết bị

Tên SoftAP nên có suffix theo thiết bị để tránh trùng khi có nhiều ESP32:

```text
ESP32-SETUP-A1B2C3
```

Suffix có thể lấy từ ba byte cuối của Wi-Fi MAC. Không dùng toàn bộ MAC làm
thông tin xác thực.

Cấu hình POC:

```cpp
WiFi.softAP(apSsid, apPassword, 1, false, 2);
```

- IP mặc định: `192.168.4.1`;
- channel khởi tạo: 1;
- SSID không ẩn;
- tối đa hai client;
- SoftAP dùng password WPA2 dài ít nhất 8 ký tự.

Không tạo SoftAP mở trong thiết kế mặc định. Với sản phẩm thật, password setup
nên khác nhau cho từng thiết bị và được in trên nhãn hoặc QR code. Password dùng
chung như `configure-me` chỉ phù hợp cho POC trong môi trường kiểm soát.

## 6. Trạng thái provisioning

Firmware dùng state machine thay vì chờ kết nối ngay bên trong HTTP handler:

```text
BOOT
  │
  ├── start AP+STA + HTTP server
  │
  ├── không có credentials ─────────────> PROVISIONING
  │                                         │
  └── có credentials ──> CONNECTING <───────┤ POST credentials
                           │                 │
                  ┌────────┴────────┐        │
                  ▼                 ▼        │
             STA_GOT_IP          TIMEOUT     │
                  │                 │        │
                  ▼                 ▼        │
        VERIFY_INTERNET          FAILED ─────┘
                  │
             HTTP probe 200
                  │
                  ▼
          SAVE_NVS → CONNECTED
```

Nguyên tắc:

- HTTP handler chỉ validate, copy credentials vào vùng nhớ pending và trả
  `202 Accepted`;
- `loop()` hoặc Wi-Fi event cập nhật tiến trình kết nối;
- không block web server trong 10–30 giây;
- chỉ lưu pending credentials sau `STA_GOT_IP` và upstream probe thành công;
- credentials cũ trong NVS được giữ nguyên cho đến khi cấu hình mới được xác
  minh;
- khi timeout hoặc sai password, xóa pending credentials khỏi RAM nhưng giữ
  SoftAP và web server;
- nếu đang thay một cấu hình cũ, firmware có thể reconnect cấu hình cũ sau khi
  cấu hình mới thất bại.

POC giữ SoftAP hoạt động kể cả sau khi STA kết nối để dễ quan sát và reset.
Trong sản phẩm thật, nên tắt SoftAP sau một khoảng grace period và chỉ bật lại
bằng nút vật lý hoặc một quy trình recovery được xác định rõ.

## 7. Web frontend

Trang `GET /` được nhúng trong firmware bằng `PROGMEM` và gồm:

- tên thiết bị và địa chỉ SoftAP;
- form nhập SSID;
- input password với `type="password"`;
- nút `Kết nối`;
- trạng thái `Chưa cấu hình`, `Đang kết nối`, `Đã kết nối` hoặc `Thất bại`;
- STA IP khi kết nối thành công;
- nút `Xóa cấu hình`;
- thông báo rằng SoftAP không cung cấp Internet cho client.

Frontend gửi form dưới dạng:

```http
POST /api/wifi/configure HTTP/1.1
Content-Type: application/x-www-form-urlencoded

ssid=HomeWifi&password=example-password
```

Sau response `202`, JavaScript poll `GET /api/status` mỗi giây. Frontend và API
cùng origin `http://192.168.4.1`, do đó không cần CORS.

Password không được:

- đưa vào query string;
- trả lại trong HTML hoặc JSON;
- ghi ra Serial;
- lưu trong biến JavaScript lâu hơn request cần thiết.

Response chứa `Cache-Control: no-store` để hạn chế browser cache trang và trạng
thái provisioning.

## 8. HTTP API contract

| Method | Path | Chức năng | Response chính |
|---|---|---|---|
| `GET` | `/` | Trả HTML/CSS/JS | `200 text/html` |
| `GET` | `/api/status` | Trạng thái AP, STA và provisioning | `200 application/json` |
| `POST` | `/api/wifi/configure` | Validate và bắt đầu thử credentials | `202 application/json` |
| `POST` | `/api/wifi/reset` | Xóa credentials đã lưu, ngắt STA | `200 application/json` |
| bất kỳ | route khác | Không tìm thấy | `404 application/json` |

`POST /api/wifi/configure` không trả thành công kết nối ngay lập tức. Response
chỉ xác nhận request đã được chấp nhận:

```json
{
  "state": "connecting"
}
```

`GET /api/status` khi thành công:

```json
{
  "state": "connected",
  "ap": {
    "ssid": "ESP32-SETUP-A1B2C3",
    "ip": "192.168.4.1",
    "clients": 1
  },
  "sta": {
    "ssid": "HomeWifi",
    "ip": "192.168.1.52",
    "rssi": -48
  },
  "upstream": {
    "reachable": true,
    "status": 200
  }
}
```

Response không bao giờ chứa password.

Mã lỗi dự kiến:

- `400`: thiếu SSID hoặc credential không hợp lệ;
- `409`: một lần thử kết nối khác đang chạy;
- `500`: không thể khởi tạo Wi-Fi hoặc ghi NVS;
- trạng thái `failed` kèm mã lỗi tổng quát khi authentication/DHCP/timeout thất
  bại, nhưng không echo password.

## 9. Validation đầu vào

POC giới hạn:

- SSID từ 1 đến 32 byte;
- password rỗng cho mạng open, hoặc 8–63 ký tự cho WPA/WPA2 Personal;
- không hỗ trợ WPA2-Enterprise trong form cơ bản;
- giới hạn kích thước toàn bộ request;
- chỉ cho một connection attempt tại một thời điểm;
- timeout kết nối hữu hạn, ví dụ 20 giây;
- retry có khoảng nghỉ, không gọi `WiFi.begin()` trong tight loop.

Kiểm tra độ dài phải được thực hiện sau khi form URL decoding. SSID hoặc
password có ký tự đặc biệt phải được gửi bằng encoding chuẩn của HTML form.

## 10. Lưu credentials bằng Preferences/NVS

Sau khi kết nối và upstream probe thành công, firmware lưu dữ liệu nhỏ trong
namespace `wifi_cfg`:

```text
key: valid  → bool
key: ssid   → String
key: pass   → String
```

Luồng ghi:

1. mở namespace read-write;
2. ghi `ssid` và `pass`;
3. ghi `valid=true` cuối cùng như commit marker;
4. đóng Preferences;
5. không log giá trị `pass`.

Khi boot, chỉ sử dụng credentials nếu `valid=true`. Endpoint reset đặt
`valid=false`, xóa `ssid`/`pass`, gọi disconnect cho STA nhưng không tắt SoftAP.

NVS giữ dữ liệu sau reset và mất điện, nhưng dữ liệu không tự động được mã hóa
chỉ vì dùng Preferences. Với production cần xem xét NVS encryption, secure boot,
flash encryption và quy trình factory reset phù hợp.

## 11. Upstream connectivity probe

`STA_GOT_IP` chứng minh authentication và DHCP thành công, nhưng chưa chắc mạng
có Internet. Để POC có bằng chứng mạnh hơn, ESP32 gọi lại endpoint public từ
POC 1:

```http
GET http://jsonplaceholder.typicode.com/todos/1
```

Chỉ khi nhận HTTP `200`, trạng thái `upstream.reachable` mới là `true` và
credentials mới được commit vào NVS.

Nếu mục tiêu sản phẩm chỉ cần LAN nội bộ và không cần Internet, bước probe phải
được thay bằng health endpoint thuộc chính hệ thống đó; không nên dùng public
API làm điều kiện provisioning trong production.

## 12. Log Serial dự kiến

```text
PROVISIONING_AP_STARTED ssid=ESP32-SETUP-A1B2C3 ip=192.168.4.1
HTTP_SERVER_STARTED port=80
AP_CLIENT_CONNECTED clients=1
WIFI_CONFIG_RECEIVED ssid=HomeWifi password=<redacted>
STA_CONNECTING ssid=HomeWifi timeout_ms=20000
STA_GOT_IP ssid=HomeWifi ip=192.168.1.52 rssi=-48
UPSTREAM_PROBE status=200
CREDENTIALS_SAVED namespace=wifi_cfg
PROVISIONING_STATE connected
```

Khi password sai:

```text
WIFI_CONFIG_RECEIVED ssid=HomeWifi password=<redacted>
STA_CONNECTING ssid=HomeWifi timeout_ms=20000
STA_CONNECTION_FAILED reason=auth timeout_or_error=<code>
PROVISIONING_STATE failed ap_available=true
```

Không in password, kể cả trong debug build.

## 13. Cấu trúc project dự kiến

```text
pocs/poc4-softap-provisioning/
├── platformio.ini
├── wokwi.toml
├── diagram.json
├── include/
│   ├── provisioning_page.h
│   └── provisioning_config.h
├── src/
│   └── main.cpp
└── README.md
```

`provisioning_page.h` chứa HTML/CSS/JS trong `PROGMEM`.
`provisioning_config.h` chứa SoftAP settings dành cho POC, không chứa password
của Wi-Fi thực tế.

## 14. Kiểm thử trên Wokwi

Wokwi mô phỏng Wi-Fi cho ESP32, nhưng tài liệu chính thức không cung cấp cách để
Wi-Fi adapter của điện thoại/máy tính tham gia trực tiếp vào SoftAP RF do ESP32
mô phỏng phát ra. Vì vậy Wokwi không phải cổng nghiệm thu end-to-end cho bước
“client nhìn thấy và join hotspot ESP32”.

Wokwi vẫn có thể dùng cho các gate một phần:

- build firmware và lint diagram;
- quan sát Serial;
- dùng Private Gateway forward browser tới HTTP server trên ESP32;
- submit `Wokwi-GUEST` với password rỗng để kiểm tra STA connection;
- hoặc thêm part chính thức `wokwi-wifi-ap` làm router thực tế giả lập. Custom
  Wi-Fi Access Point yêu cầu Wokwi Hobby+ hoặc Pro.

Port forwarding cho web form:

```toml
[[net.forward]]
from = "localhost:8181"
to = "target:80"
```

Sau đó có thể mở `http://localhost:8181`, nhưng đường này bypass việc thiết bị
host thực sự join SSID SoftAP. Kết quả chỉ được ghi là partial simulation.

## 15. Kiểm thử bắt buộc trên board thật

Cần router Wi-Fi thật, hoặc một điện thoại thứ hai làm hotspot. Một điện thoại
thường không thể vừa phát hotspot thực tế vừa tham gia SoftAP của ESP32, nên
không dùng cùng một thiết bị cho cả hai vai trò.

Luồng nghiệm thu:

1. Flash firmware vào ESP32 classic.
2. Mở Serial Monitor ở 115200.
3. Xác nhận `PROVISIONING_AP_STARTED` và IP `192.168.4.1`.
4. Trên điện thoại/laptop, tìm đúng `ESP32-SETUP-<suffix>`.
5. Kết nối bằng setup password.
6. Mở `http://192.168.4.1` và thấy form do ESP32 phục vụ.
7. Nhập sai password router; xác nhận trạng thái `failed` và form vẫn truy cập
   được.
8. Nhập đúng credentials.
9. Quan sát `STA_GOT_IP` và upstream HTTP `200` trên Serial.
10. Xác nhận status API có STA IP nhưng không có password.
11. Reboot ESP32 và xác nhận tự kết nối lại bằng credentials trong NVS.
12. Gọi reset API; reboot và xác nhận thiết bị trở lại trạng thái provisioning.

Điện thoại có thể tự bỏ mạng Wi-Fi “không có Internet”. Khi kiểm thử, cần tắt
tính năng tự chuyển mạng hoặc chọn giữ kết nối với SoftAP ESP32.

## 16. Tiêu chí nghiệm thu

POC chỉ hoàn thành khi có bằng chứng runtime trên board thật:

- điện thoại/laptop nhìn thấy và join được SoftAP;
- SoftAP cấp IP cho client;
- browser tải HTML form từ `192.168.4.1`;
- ESP32 nhận SSID nhưng không log password;
- credential sai không làm mất cổng cấu hình;
- credential đúng tạo `STA_GOT_IP`;
- upstream probe trả `200`;
- status API trả đúng AP IP, STA IP và trạng thái;
- credentials tồn tại sau reboot;
- reset xóa cấu hình và phục hồi provisioning mode;
- client SoftAP không được mô tả sai là đã có Internet qua ESP32.

Build success hoặc browser truy cập qua Wokwi port forwarding không đủ để kết
luận POC hoàn thành.

## 17. Cổng kiểm chứng

### Static, build và artifact gates

```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc4-softap-provisioning/diagram.json", "utf8"))'

pio run -d pocs/poc4-softap-provisioning -e esp32dev

test -f pocs/poc4-softap-provisioning/.pio/build/esp32dev/firmware.elf
test -f pocs/poc4-softap-provisioning/.pio/build/esp32dev/firmware.bin

pio pkg list -d pocs/poc4-softap-provisioning -e esp32dev
```

Nếu `wokwi-cli` có sẵn:

```bash
cd pocs/poc4-softap-provisioning
wokwi-cli lint
```

### Board runtime gate

```bash
pio device list

pio run -d pocs/poc4-softap-provisioning -e esp32dev \
  -t upload --upload-port <PORT>

pio device monitor -p <PORT> -b 115200
```

API có thể được kiểm tra từ client đang nối SoftAP:

```bash
curl -i http://192.168.4.1/api/status

curl -i -X POST http://192.168.4.1/api/wifi/configure \
  -H 'Content-Type: application/x-www-form-urlencoded' \
  --data-urlencode 'ssid=HomeWifi' \
  --data-urlencode 'password=<PASSWORD>'

curl -i http://192.168.4.1/api/status
```

Không ghi password thật vào shell history trong môi trường dùng chung. Khi test
thủ công, ưu tiên nhập password trong HTML form.

## 18. Security và hướng nâng cấp

POC này chứng minh luồng chức năng, chưa phải production provisioning. Trước khi
đưa vào sản phẩm cần xem xét:

- setup password hoặc proof-of-possession riêng cho từng thiết bị;
- tự tắt SoftAP sau provisioning;
- nút vật lý để bật lại provisioning hoặc factory reset;
- mã hóa NVS, secure boot và flash encryption;
- chống CSRF, rate limiting và giới hạn số lần thử;
- HTTPS hoặc provisioning protocol có mã hóa ở tầng ứng dụng;
- captive portal DNS nếu muốn tự mở trang cấu hình;
- xử lý enterprise Wi-Fi nếu sản phẩm yêu cầu;
- không phụ thuộc public API để xác nhận mạng production.

ESP-IDF 4.4 cung cấp provisioning manager chính thức với SoftAP transport,
HTTP server và các security scheme. Đây là hướng nâng cấp phù hợp khi POC Arduino
tự xây đã chứng minh được luồng sản phẩm.

## 19. Nguồn chính thức

- [Arduino-ESP32 Wi-Fi API: AP và STA modes](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html)
- [WiFiAccessPoint example đúng Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WiFi/examples/WiFiAccessPoint/WiFiAccessPoint.ino)
- [Wi-Fi events example đúng Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WiFi/examples/WiFiClientEvents/WiFiClientEvents.ino)
- [WebServer API đúng Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/WebServer/src/WebServer.h)
- [Arduino-ESP32 Preferences/NVS](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html)
- [ESP-IDF 4.4 Wi-Fi Provisioning](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/provisioning/wifi_provisioning.html)
- [Espressif SoftAP explanation](https://docs.espressif.com/projects/esp-techpedia/en/latest/esp-friends/get-started/case-study/wifi-examples/softap-example.html)
- [Wokwi ESP32 Wi-Fi Networking](https://docs.wokwi.com/guides/esp32-wifi)
- [Wokwi Wi-Fi Access Point part](https://docs.wokwi.com/parts/wokwi-wifi-ap)
- [Wokwi VS Code port forwarding](https://docs.wokwi.com/vscode/project-config)
- [PlatformIO ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)

## 20. Trạng thái

Firmware, frontend, API, NVS state machine và cấu hình Wokwi đã được triển khai;
build/lint pass. Chưa có bằng chứng runtime trên firmware cuối hoặc board ESP32
thật, nên chưa xác minh end-to-end. Xem
`pocs/poc4-softap-provisioning/STATUS.md`.
