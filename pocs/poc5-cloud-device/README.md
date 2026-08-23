# POC 5 — ESP32 provisioning và điều khiển qua WebSocket

POC này triển khai luồng IoT thực tế tối thiểu: ESP32 mở SoftAP/portal để nhận
Wi-Fi và public server, kết nối outbound bằng WSS, rồi nhận lệnh bật/tắt
`Real_Device` từ dashboard FastAPI. Server chuyển lệnh trực tiếp và chỉ trả HTTP
200 sau khi ESP32 đã đặt GPIO rồi ACK; không có `desired_state`, queue hay replay.

Trạng thái kiểm chứng hiện tại nằm tại [STATUS.md](STATUS.md); thiết kế và protocol
chi tiết nằm tại [../../docs/pocs/POC-05-CLOUD-WEBSOCKET-DEVICE.md](../../docs/pocs/POC-05-CLOUD-WEBSOCKET-DEVICE.md).

## Thành phần và pin

Project khóa `esp32dev`, Arduino-ESP32 2.0.17 trên Espressif32 platform 7.0.1,
Serial 115200. Các dependency firmware được pin tại `platformio.ini`.

| GPIO | Thành phần | Ý nghĩa khi sáng |
|---:|---|---|
| 18 | vàng | SoftAP và HTTP portal đang hoạt động |
| 19 | xanh dương | Có client thật đang join SoftAP |
| 21 | xanh lá | ESP32 STA có IP từ Wi-Fi thực tế |
| 22 | trắng | WSS đã upgrade và hoàn tất `hello/ready` |
| 23 | đỏ, `Real_Device` | Output thiết bị đang ON |
| 25 | nút setup/reset | nhấn ngắn mở portal; giữ ít nhất 5 giây để reset |

LED rời trên board thật cần điện trở hạn dòng. GPIO chỉ mô phỏng tín hiệu điều
khiển; tuyệt đối không nối trực tiếp tải điện lưới vào ESP32.

## Chạy FastAPI local

Từ thư mục `server`:

```bash
python3 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
cp .env.example .env
```

Shell không tự đọc `.env`; hãy export biến bằng cách an toàn phù hợp môi trường,
hoặc dùng trực tiếp ba biến dưới đây với giá trị riêng:

```bash
export POC5_DASHBOARD_API_KEY='<dashboard-key-rieng>'
export POC5_DEVICE_TOKENS_JSON='{"esp32-poc5":"<device-token-rieng>"}'
.venv/bin/python -m uvicorn app.main:app --host 127.0.0.1 --port 8000
```

Các URL local:

- dashboard: `http://127.0.0.1:8000/dashboard`;
- Swagger REST: `http://127.0.0.1:8000/docs`;
- health: `http://127.0.0.1:8000/health`.

Registry nằm trong RAM nên chỉ chạy **một Uvicorn worker**. Restart server làm
mất snapshot nhưng không làm mất output ESP32; device reconnect và gửi trạng
thái hiện tại trong `hello`.

## Public server bằng ngrok

Sau khi Uvicorn chạy ở port 8000, mở tunnel HTTPS:

```bash
ngrok http 8000
```

Nếu tài khoản có static domain:

```bash
ngrok http 8000 --url https://<assigned-domain>.ngrok-free.app
```

Portal ESP32 chỉ nhận **host**, không nhận scheme hay URL đầy đủ. Ví dụ:

```text
Server host: <assigned-domain>.ngrok-free.app
Server port: 443
Server path: /ws/devices
```

Firmware ghép thành
`wss://<host>:443/ws/devices/esp32-poc5`, xác minh TLS bằng ISRG Root X1, rồi
gửi Bearer token. Khi domain/certificate provider thay đổi, phải kiểm tra lại
certificate chain và cập nhật trust anchor trước khi deploy.

## Đồng bộ secret firmware và server

Copy file mẫu rồi thay token; file thật đã được gitignore:

```bash
cp include/secrets.example.h include/secrets.h
```

`DEVICE_ID` và `DEVICE_TOKEN` phải trùng key/value trong
`POC5_DEVICE_TOKENS_JSON`. Không nhập device token trong portal. Giá trị mặc
định `*-change-me` chỉ giúp chạy local và không phù hợp khi public tunnel.

## Build và kiểm tra tĩnh

Từ repository root:

```bash
./scripts/poc.sh 5 verify
```

Lệnh trên parse `diagram.json`, build đúng environment `esp32dev`, xác nhận ELF
và BIN, rồi lint diagram. Có thể chạy riêng:

```bash
./scripts/poc.sh 5 build
./scripts/poc.sh 5 artifacts
./scripts/poc.sh 5 lint
```

Server test:

```bash
cd pocs/poc5-cloud-device/server
.venv/bin/python -m pytest -q
```

## Provision trên Wokwi

1. Build firmware, chọn `wokwi.toml` bằng **Wokwi: Select Config File** và start
   simulator; giữ tab simulator hiển thị.
2. Xem Serial ở terminal tích hợp `Wokwi Terminal`, hoặc RFC2217 port 4005.
3. Với Wokwi Private Gateway, mở `http://localhost:8185` để thấy portal.
4. Nhập SSID `Wokwi-GUEST`, password rỗng và host ngrok; chờ lần lượt marker
   `WIFI_CONNECTED`, `TIME_SYNCED`, `WSS_AUTHENTICATED`, `CONFIG_COMMITTED`.
5. Mở dashboard public, nhập API key và device ID `esp32-poc5`, rồi bật/tắt.
   Serial phải có `COMMAND_RECEIVED`, `DEVICE_STATE_APPLIED`,
   `COMMAND_ACK_SENT`, đồng thời LED đỏ phải đổi trạng thái.

Wokwi host forward chỉ đưa HTTP tới ESP32; nó không làm một Wi-Fi station join
SoftAP mô phỏng. Vì vậy LED GPIO19 và luồng điện thoại join AP chỉ được nghiệm
thu hợp lệ trên board thật.

Nếu có `WOKWI_CLI_TOKEN`, có thể chạy:

```bash
./scripts/poc.sh 5 simulate --expect-text HTTP_SERVER_STARTED
./scripts/poc.sh 5 serial --lines 30
```

## Provision trên board thật

```bash
pio device list
pio run -d pocs/poc5-cloud-device -e esp32dev \
  -t upload --upload-port <PORT>
pio device monitor -p <PORT> -b 115200
```

Sau boot chưa có config, join `ESP32-SETUP-<suffix>` bằng password
`configure-me`, mở `http://192.168.4.1`, rồi nhập Wi-Fi và server. Điện thoại có
thể báo “No Internet” vì SoftAP này chỉ là cổng cấu hình, không phải router.

Nghiệm thu tối thiểu phải gồm credential sai/đúng, power cycle, ngắt Wi-Fi,
restart FastAPI/ngrok, nhấn ngắn mở setup, giữ nút 5 giây factory reset và truy
cập dashboard từ mạng ngoài. Config mới chỉ được commit sau khi Wi-Fi, NTP,
TLS, WSS và application `hello/ready` đều thành công; nếu thất bại firmware thử
quay về config cũ.

## Giới hạn có chủ đích

- NVS dùng commit marker, chưa có two-slot/CRC transaction.
- Device ID/token là build-time per-device secret; chưa có manufacturing flow,
  token rotation hay secure element.
- Setup password mẫu dùng chung; production cần password riêng/QR,
  proof-of-possession, CSRF/rate limit, flash encryption và secure boot.
- TLS root đang dành cho chain đã kiểm tra của ngrok; chưa có OTA CA rotation.
- Dashboard giữ API key trong memory của browser; đây là POC, không phải user
  authentication hoàn chỉnh.
- Snapshot FastAPI in-memory và chỉ hỗ trợ một process/worker.

## Nguồn chính thức

- [Arduino-ESP32 Wi-Fi API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html)
- [Arduino-ESP32 Preferences](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/preferences.html)
- [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets)
- [FastAPI WebSockets](https://fastapi.tiangolo.com/advanced/websockets/)
- [FastAPI testing](https://fastapi.tiangolo.com/tutorial/testing/)
- [ngrok WebSockets](https://ngrok.com/docs/using-ngrok-with/websockets)
- [Wokwi ESP32 Wi-Fi](https://docs.wokwi.com/guides/esp32-wifi)
- [Wokwi VS Code project config](https://docs.wokwi.com/vscode/project-config)
- [PlatformIO ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)
