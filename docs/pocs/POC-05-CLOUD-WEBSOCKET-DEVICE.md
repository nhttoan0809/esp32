# POC 5 — Provisioning và điều khiển thiết bị qua FastAPI WebSocket

## 1. Mục tiêu

POC này ghép các khả năng đã được tách riêng trong POC 1, 3 và 4 thành một
luồng IoT hoàn chỉnh:

1. ESP32 phát Wi-Fi SoftAP để người dùng mở portal cấu hình;
2. portal nhận Wi-Fi thực tế và địa chỉ server;
3. ESP32 kết nối Wi-Fi ở chế độ Station;
4. ESP32 duy trì một WebSocket client outbound tới FastAPI qua ngrok;
5. điện thoại mở dashboard trên FastAPI và gửi lệnh bật/tắt;
6. FastAPI chuyển lệnh tới ESP32 qua WebSocket;
7. ESP32 chỉ xác nhận sau khi đã cập nhật GPIO của `Real_Device`;
8. dashboard chỉ hiển thị thành công sau ACK từ ESP32.

POC không lưu `desired_state`, không queue lệnh khi thiết bị offline và không
tự áp lại lệnh cũ sau reboot. Server chỉ giữ `reported_state` gần nhất trong
RAM để hiển thị trạng thái đã được thiết bị xác nhận.

## 2. Quyết định phạm vi

### 2.1 Connection method

Phiên bản đầu dùng Wi-Fi SoftAP + HTTP portal. BLE chưa nằm trong phạm vi vì:

- basic HTML portal cần IP/HTTP, phù hợp trực tiếp với SoftAP;
- POC 4 đã có AP+STA, WebServer, Preferences và pending credential flow;
- Wokwi không mô phỏng Bluetooth;
- BLE provisioning cần GATT client hoặc ứng dụng/Web Bluetooth riêng.

Thiết kế module provisioning không được phụ thuộc vào cloud client để có thể
thêm BLE transport sau này, nhưng không tạo abstraction chung khi mới chỉ có
một implementation.

### 2.2 Command semantics

`PUT /api/devices/{device_id}/state` là lệnh trực tiếp, không phải cập nhật một
device shadow:

1. server kiểm tra WebSocket của device đang online;
2. server chỉ cho một command in-flight trên mỗi device;
3. server tạo `command_id`, gửi `set_state` và chờ tối đa 3 giây;
4. ESP32 đặt GPIO rồi trả `state_report` có cùng `command_id`;
5. server trả HTTP 200 cùng trạng thái đã xác nhận.

Nếu device offline, API trả `503 device_offline`. Nếu mất kết nối trong lúc
chờ, API trả `503 device_disconnected`. Nếu không nhận ACK đúng hạn, API trả
`504 device_ack_timeout`. Server không ghi nhớ và không phát lại lệnh thất bại.

### 2.3 Trạng thái khi mất mạng và reboot

- cold boot đặt `Real_Device` về OFF trước khi khởi tạo network;
- mất WebSocket tạm thời không tự đổi GPIO;
- reboot FastAPI làm mất state trong RAM; khi reconnect, ESP32 gửi trạng thái
  GPIO hiện tại để khôi phục `reported_state`;
- reconnect không làm server gửi một trạng thái mong muốn cũ xuống device.

## 3. Nền tảng và dependency

Giữ nguyên baseline repository:

```ini
[env:esp32dev]
platform = espressif32@7.0.1
board = esp32dev
framework = arduino
monitor_speed = 115200
build_flags = -DCORE_DEBUG_LEVEL=0
```

Phiên bản đã kiểm tra trước khi lập plan:

- PlatformIO Core 6.1.19;
- Espressif32 platform 7.0.1;
- Arduino-ESP32 2.0.17;
- Wokwi for VS Code 3.6.0;
- Wokwi CLI 0.26.1;
- Python 3.13.3.

Dependency firmware dự kiến, phải pin và compile-spike trước khi viết cloud
logic:

```ini
lib_deps =
  links2004/WebSockets@2.7.3
  bblanchon/ArduinoJson@7.4.3
```

Package `WebSockets` 2.7.3 trong PlatformIO registry đã được kiểm tra có các API
`beginSslWithCA`, `setExtraHeaders`, `setReconnectInterval` và
`enableHeartbeat`. Compile-spike phải xác nhận chúng tương thích với
Arduino-ESP32 2.0.17 trước khi tiếp tục.

Server dùng FastAPI + Uvicorn. Requirements phải pin exact version sau khi test
trên Python 3.13.3; baseline để bắt đầu là FastAPI 0.141.1 và Uvicorn 0.52.3.
Server in-memory chỉ chạy một Uvicorn worker.

## 4. Cấu trúc project

```text
pocs/poc5-cloud-device/
├── platformio.ini
├── wokwi.toml
├── diagram.json
├── include/
│   ├── app_config.h
│   ├── provisioning_page.h
│   ├── secrets.example.h
│   └── secrets.h                 # local, gitignored
├── src/
│   ├── main.cpp
│   ├── device_controller.cpp
│   ├── device_controller.h
│   ├── provisioning.cpp
│   ├── provisioning.h
│   ├── wifi_manager.cpp
│   ├── wifi_manager.h
│   ├── cloud_client.cpp
│   └── cloud_client.h
├── server/
│   ├── app/
│   │   ├── main.py
│   │   ├── models.py
│   │   ├── registry.py
│   │   └── static/dashboard.html
│   ├── tests/
│   ├── requirements.txt
│   └── .env.example
├── README.md
└── STATUS.md
```

Không đưa Wi-Fi password, device token, dashboard API key, ngrok authtoken hoặc
`.env` thật vào Git.

## 5. Pin map và LED semantics

Pin map mục tiêu cho ESP32 DevKit V1 (30 chân; phần tử Wokwi `board-esp32-devkit-v1`):

| Thành phần | GPIO | Điều kiện sáng |
|---|---:|---|
| `Led_Connection_Setup` | 18 | SoftAP và HTTP server đều start thành công |
| `Led_Connection_Success` | 19 | Có ít nhất một station đang nối SoftAP |
| `Led_Wifi_Connection_Success` | 21 | STA có IP hợp lệ |
| `Led_Server_Connection_Success` | 22 | WSS upgrade, auth và application hello hoàn tất |
| `Real_Device` | 23 | Trạng thái output đã áp dụng |
| setup/reset button | 25 | Giữ để vào setup hoặc factory reset |

Mọi output được đưa về LOW trước khi chuyển pin sang OUTPUT để tránh chớp lúc
boot. Diagram chỉ dùng identifier đã đối chiếu tài liệu Wokwi:
`board-esp32-devkit-v1`, `wokwi-led`, `wokwi-pushbutton` và part resistor chính
thức sau khi kiểm tra schema/docs của extension 3.6.0. UART0 tiếp tục nối TX/RX
chéo với `$serialMonitor`.

Board thật phải dùng điện trở hạn dòng cho LED. `Real_Device` trong POC chỉ là
LED; GPIO không được nối trực tiếp tới tải điện lưới.

## 6. Configuration model

Portal nhận:

- Wi-Fi SSID;
- Wi-Fi password;
- WSS host, ví dụ `<assigned-domain>.ngrok-free.app`;
- WSS port, mặc định và chỉ cho phép 443;
- WSS base path, mặc định `/ws/devices`.

Không cho nhập URL tùy ý nguyên chuỗi. Firmware validate riêng host, port và
path, từ chối scheme, query, fragment, control byte, IP literal và giá trị quá
dài. Production path chỉ dùng WSS.

`device_id` và `device_token` nằm trong `secrets.h` của POC và không cho sửa
trong portal. Giá trị mẫu dùng ID cố định `esp32-poc5` để khớp server local;
production nên provision ID duy nhất từ backend/manufacturing thay vì dùng
chung secret. Hai giá trị này không xuất hiện trong HTML, JSON status hoặc
Serial. CA root/bundle là build-time trust material, không nhận từ portal.

Pending config chỉ được commit sau các gate:

```text
STA_GOT_IP -> NTP_SYNCED -> TLS_OK -> WSS_AUTH_OK -> APP_HELLO_OK -> NVS_COMMIT
```

Config cũ không bị ghi đè khi pending config thất bại. POC dùng commit marker
như POC 4; two-slot NVS/CRC được để ngoài MVP nhưng phải ghi là giới hạn.

## 7. Firmware state machines

Không dùng một enum duy nhất để gộp mọi trạng thái. Các state độc lập gồm:

```text
Provisioning: disabled | ready | client_connected | applying | failed
Wi-Fi:        disconnected | connecting | got_ip | retry_wait
Cloud:        disabled | time_sync | connecting | authenticating | online | retry_wait
Device:       off | on
```

Luồng boot:

```text
BOOT
  ├── init GPIO, Real_Device OFF
  ├── config không hợp lệ -> start provisioning AP
  └── config hợp lệ -> connect STA
                         ├── success -> NTP -> WSS
                         └── retry có backoff
```

Recovery policy:

- nút nhấn ngắn: mở provisioning AP mà chưa xóa config;
- giữ 5 giây: xóa config sau debounce và confirmation log;
- sau khi cloud online ổn định, giữ AP thêm grace period rồi tắt;
- Wi-Fi lỗi ngắn chỉ retry, không mở AP ngay;
- quá ngưỡng lỗi cấu hình mới mở AP fallback nhưng vẫn giữ config cũ.

Tất cả scheduler dùng `millis()` và xử lý rollover; không có `while` chờ Wi-Fi,
NTP, HTTP hoặc WebSocket. Callback không thực hiện NVS hay network operation
dài.

## 8. Retry, heartbeat và TLS

Wi-Fi và cloud có retry scheduler riêng:

```text
1s -> 2s -> 4s -> 8s -> 16s -> 30s -> 60s cap
```

Thêm jitter nhỏ để nhiều device không reconnect đồng thời. Backoff reset chỉ
sau một khoảng kết nối ổn định, không ngay khi TCP vừa mở.

WebSocket heartbeat:

- ping interval 20 giây;
- pong timeout 10 giây;
- disconnect sau 2 timeout;
- application `hello/ready` có timeout riêng.

TLS yêu cầu NTP sync trước khi connect, dùng CA root/bundle với
`beginSslWithCA`; không dùng `setInsecure()` trong trạng thái cuối. Device auth
dùng header `Authorization: Bearer ...` qua `setExtraHeaders`; buffer header
phải sống lâu hơn WebSocket client vì API giữ con trỏ.

## 9. Protocol contract

### 9.1 Device hello

Ngay sau WebSocket connect, ESP32 gửi:

```json
{
  "v": 1,
  "type": "hello",
  "device_id": "esp32-a1b2c3",
  "firmware": "poc5-0.1.0",
  "reported": { "on": false }
}
```

Server validate identity từ token và path, sau đó trả:

```json
{ "v": 1, "type": "ready", "device_id": "esp32-a1b2c3" }
```

Server chỉ đánh dấu online và ESP32 chỉ bật LED server sau `ready`.

### 9.2 Direct command

```json
{
  "v": 1,
  "type": "set_state",
  "command_id": "uuid",
  "device_id": "esp32-a1b2c3",
  "on": true
}
```

ESP32 validate version, type, ID, UUID, boolean và message size; đặt GPIO rồi
trả:

```json
{
  "v": 1,
  "type": "state_report",
  "command_id": "uuid",
  "device_id": "esp32-a1b2c3",
  "on": true
}
```

`set_state` là idempotent: command trùng vẫn đặt cùng output và trả lại report.
ACK sai device/command ID không được hoàn tất HTTP request đang chờ.

## 10. FastAPI contract

| Method | Path | Kết quả |
|---|---|---|
| `GET` | `/health` | process readiness, không cần device online |
| `GET` | `/dashboard` | basic mobile-friendly HTML |
| `GET` | `/api/devices/{id}` | online, reported state, last seen |
| `PUT` | `/api/devices/{id}/state` | gửi command và chờ ACK |
| `WS` | `/ws/devices/{id}` | kết nối outbound của ESP32 |
| `GET` | `/docs` | Swagger cho REST endpoints |

Swagger/OpenAPI không mô tả protocol WebSocket, nên message contract ở tài
liệu này và được cover bằng integration tests riêng.

Server registry in-memory phải xử lý:

- authenticated connection mới thay connection cũ cùng device ID;
- connection generation để callback cũ không đánh dấu connection mới offline;
- một `asyncio.Lock` và pending Future trên mỗi device;
- cleanup pending command khi disconnect/timeout;
- `reported_state = unknown` cho tới hello/report đầu tiên;
- tuyệt đối không log Authorization header hoặc token.

Dashboard và REST dùng một dashboard API key từ environment. HTML có thể public
nhưng API không public; key chỉ giữ trong memory của trang cho POC. Dashboard
chỉ đổi UI sau HTTP 200 và hiển thị rõ offline/timeout/error.

## 11. Wokwi configuration và giới hạn

Dự kiến:

```toml
[wokwi]
version = 1
elf = ".pio/build/esp32dev/firmware.elf"
firmware = ".pio/build/esp32dev/firmware.bin"
rfc2217ServerPort = 4005

[[net.forward]]
from = "localhost:8185"
to = "target:80"
```

Wokwi kiểm chứng được:

- GPIO/LED/button và Serial;
- portal/API qua Private Gateway forward;
- STA tới `Wokwi-GUEST`;
- outbound WSS tới ngrok;
- dashboard command -> server -> ESP32 -> Real_Device -> ACK;
- ngrok/FastAPI stop/start và WebSocket retry.

Wokwi không kiểm chứng được điện thoại thật join SoftAP. Request qua
`localhost:8185` không tương đương một Wi-Fi station và không đủ để pass
`Led_Connection_Success`. Gate này bắt buộc chạy trên ESP32 thật.

## 12. Kế hoạch triển khai theo gate

### Gate 0 — Dependency và protocol spike

- scaffold project tối thiểu;
- pin WebSockets/ArduinoJson;
- compile `beginSslWithCA`, Authorization header và heartbeat trên core 2.0.17;
- viết protocol examples và error mapping;
- không viết provisioning/cloud state machine trước khi gate compile pass.

### Gate 1 — Fake server độc lập

- FastAPI health, dashboard, registry, REST command và device WebSocket;
- fake WebSocket device trong pytest;
- test success ACK, offline 503, disconnect 503, timeout 504, malformed ACK,
  duplicate connection và concurrent command;
- chạy một worker và xác nhận Swagger cho REST.

### Gate 2 — Hardware layer

- diagram với bốn status LED, Real_Device và setup button;
- `DeviceController` đặt output an toàn và report trạng thái thực;
- JSON parse, build, artifact và lint pass;
- quan sát từng LED/Real_Device bằng một diagnostic state sequence tạm, sau đó
  gỡ sequence khỏi thiết kế cuối.

### Gate 3 — Provisioning

- port logic đã kiểm chứng từ POC 4 thay vì copy mù;
- thêm server host/port/path validation;
- pending config, old-config fallback, reset button và AP grace period;
- đảm bảo password/token không xuất hiện trong log/response;
- test credential sai, config invalid và reset.

### Gate 4 — Wi-Fi, NTP và WSS client

- non-blocking Wi-Fi reconnect;
- time-sync gate;
- WSS CA verification, Authorization, hello/ready, heartbeat và retry;
- LED Wi-Fi và LED server phản ánh state riêng;
- server restart không làm firmware reset hoặc đổi Real_Device.

### Gate 5 — Command/ACK end-to-end

- dashboard PUT -> WSS command -> GPIO -> state report -> HTTP 200;
- offline/timeout không đổi dashboard thành công;
- command trùng idempotent;
- reconnect gửi trạng thái hiện tại nhưng không nhận replay command cũ.

### Gate 6 — Final Wokwi verification

- parse JSON;
- build đúng environment;
- kiểm tra ELF/BIN;
- lint và phân loại error/warning/info;
- chạy simulator với tab hiển thị;
- thu Serial qua Wokwi Terminal/RFC2217;
- quan sát Real_Device sáng/tắt từ dashboard;
- stop/start server để quan sát retry và recovery;
- gỡ mọi helper/diagnostic part tạm rồi chạy lại toàn bộ gate.

### Gate 7 — Board thật bắt buộc

- điện thoại join SoftAP và xác nhận LED connection;
- mở `192.168.4.1`, thử credential sai/đúng;
- xác nhận ba lớp độc lập: Wi-Fi, WSS, Real_Device;
- power cycle và kiểm tra NVS;
- ngắt router, restart ngrok/FastAPI và quan sát reconnect;
- nhấn ngắn mở setup, giữ 5 giây factory reset;
- kiểm tra dashboard từ mạng ngoài Wi-Fi của ESP32.

### Gate 8 — Documentation và cleanup

- cập nhật `scripts/poc.sh` cho POC 5 và port 4005;
- README hướng dẫn local server, ngrok, Wokwi và board thật;
- STATUS chỉ đánh dấu các gate có evidence thực tế;
- ghi phiên bản, command, Serial markers, warning còn lại và giới hạn;
- kiểm tra Git không chứa secret hoặc artifact tạm.

## 13. Test matrix tối thiểu

| Trường hợp | Kết quả bắt buộc |
|---|---|
| Chưa có config | AP + portal lên, setup LED sáng |
| Điện thoại join AP | connection LED sáng trên board thật |
| Wi-Fi credential sai | không commit, portal còn dùng được |
| Wi-Fi đúng, server URL sai | Wi-Fi LED sáng, server LED tắt, config không commit |
| Token sai | WSS bị từ chối, không log token |
| Server online | hello/ready pass, server LED sáng |
| Bật/tắt | HTTP chỉ 200 sau GPIO + matching ACK |
| Device offline | REST 503, không queue |
| ACK timeout | REST 504, reported state không bị giả cập nhật |
| FastAPI restart | ESP32 reconnect và report state hiện tại |
| Ngắt Wi-Fi | LED Wi-Fi/server tắt đúng thứ tự, firmware vẫn responsive |
| Reboot ESP32 | Real_Device OFF, tự reconnect từ config đã commit |
| Factory reset | xóa config, boot lại vào provisioning |

## 14. Definition of Done

POC chỉ hoàn thành khi có đủ evidence:

- server unit/integration tests pass;
- PlatformIO build pass và ELF/BIN tồn tại;
- Wokwi lint không có error; mọi warning/info được giải thích;
- Serial thực tế có marker provisioning, Wi-Fi, WSS, command và ACK;
- dashboard điều khiển được Real_Device trong simulator;
- board thật xác nhận phone join SoftAP và LED connection;
- retry server/Wi-Fi đã được gây lỗi và quan sát recovery;
- không có secret trong Git;
- README và STATUS phản ánh đúng phần đã/chưa xác minh.

Build success một mình không đủ để đánh dấu hoàn thành.

## 15. Nguồn chính thức dùng cho thiết kế

- [ESP32-DevKitC V4](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html)
- [Arduino-ESP32 Wi-Fi API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html)
- [Arduino-ESP32 Preferences](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html)
- [Espressif Unified Provisioning](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/provisioning/provisioning.html)
- [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets)
- [FastAPI WebSockets](https://fastapi.tiangolo.com/advanced/websockets/)
- [ngrok WebSockets](https://ngrok.com/docs/using-ngrok-with/websockets)
- [Wokwi ESP32 Wi-Fi](https://docs.wokwi.com/guides/esp32-wifi)
- [Wokwi project config](https://docs.wokwi.com/vscode/project-config)
- [Wokwi diagram format](https://docs.wokwi.com/diagram-format)
- [Wokwi pushbutton](https://docs.wokwi.com/parts/wokwi-pushbutton)
- [Wokwi LED](https://docs.wokwi.com/parts/wokwi-led)
- [PlatformIO ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)
