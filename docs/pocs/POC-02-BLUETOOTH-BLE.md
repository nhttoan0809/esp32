# POC 2 — ESP32 BLE: phát hiện client kết nối

## 1. Mục tiêu

POC này biến ESP32 thành một Bluetooth Low Energy GATT Server có thể:

1. quảng bá tên thiết bị và service UUID;
2. chấp nhận một BLE client kết nối;
3. in thông tin session ra Serial khi kết nối thành công;
4. cung cấp một characteristic có thể đọc với giá trị `ready`;
5. phát hiện disconnect và tự quảng bá lại để client kết nối lần nữa.

## 2. Lựa chọn BLE thay vì Bluetooth Classic

ESP32 classic hỗ trợ cả Bluetooth Classic và BLE. POC chọn BLE GATT Server vì:

- callback connect/disconnect khớp trực tiếp với yêu cầu;
- có thể kiểm thử bằng BLE GATT client trên Android, iOS và macOS;
- characteristic cung cấp một phép kiểm tra ứng dụng rõ ràng, không chỉ dừng ở
  việc thiết bị xuất hiện trong danh sách scan;
- Arduino-ESP32 2.0.17 đã cung cấp thư viện và ví dụ BLE tương ứng.

POC không dùng thư viện BLE bên thứ ba.

## 3. Giới hạn bắt buộc của Wokwi

Wokwi đánh dấu Bluetooth là không được hỗ trợ đối với ESP32, ESP32-S3, C3, C5
và C6 trong feature matrix chính thức.

Vì vậy:

- Wokwi không thể scan, advertising, connect hay kích hoạt callback BLE;
- build thành công hoặc nhìn thấy log boot không chứng minh Bluetooth hoạt động;
- nghiệm thu runtime bắt buộc phải dùng ESP32 classic thật;
- trạng thái Bluetooth trong Wokwi phải được báo là `N/A — unsupported`, không
  được báo pass.

## 4. Nền tảng mục tiêu

```ini
platform = espressif32@7.0.1
board = esp32dev
framework = arduino
monitor_speed = 115200
```

Phiên bản mục tiêu:

- PlatformIO Core 6.1.19;
- Arduino-ESP32 core 2.0.17;
- ESP32 classic `esp32dev`;
- framework Arduino.

Không đổi board sang ESP32-C3 hoặc ESP32-S3: các biến thể chip có khả năng
Bluetooth khác nhau và không được suy ra từ POC này.

## 5. Kiến trúc BLE

```text
Điện thoại/máy tính                 ESP32
BLE GATT client                     BLE GATT server
       │                                   │
       ├──── scan + connect ──────────────>│
       │                                   ├── Serial: BLE_CONNECTED
       ├──── read characteristic ─────────>│
       │<──────────── "ready" ─────────────┤
       ├──── disconnect ──────────────────>│
       │                                   ├── Serial: BLE_DISCONNECTED
       │                                   └── advertising restart
```

Thiết kế GATT tối thiểu:

- device name: `ESP32-POC2-BLE`;
- một service UUID cố định do project sở hữu;
- một characteristic có quyền `READ`;
- giá trị characteristic: `ready`.

Service UUID và characteristic UUID phải được ghi cố định trong README để BLE
client có thể lọc đúng thiết bị.

## 6. Cấu trúc project dự kiến

```text
pocs/poc2-bluetooth/
├── platformio.ini
├── src/
│   └── main.cpp
└── README.md
```

Không tạo sơ đồ Wokwi như một cổng nghiệm thu Bluetooth. Nếu sau này thêm
`diagram.json` và `wokwi.toml` để kiểm tra UART boot, README vẫn phải ghi rõ
Bluetooth runtime không được mô phỏng.

## 7. Thiết kế firmware

`setup()` dự kiến:

1. mở `Serial` ở 115200;
2. gọi `BLEDevice::init("ESP32-POC2-BLE")`;
3. tạo `BLEServer`;
4. đăng ký `BLEServerCallbacks`;
5. tạo service và characteristic;
6. đặt characteristic thành `ready`;
7. start service và advertising;
8. in `BLE_READY` và `BLE_ADVERTISING`.

Callback sử dụng overload có thông tin chi tiết trong core 2.0.17:

```cpp
void onConnect(BLEServer *, esp_ble_gatts_cb_param_t *);
void onDisconnect(BLEServer *, esp_ble_gatts_cb_param_t *);
```

Thông tin cần in khi kết nối:

- peer Bluetooth address từ `remote_bda`;
- connection ID;
- address type;
- số client hiện đang kết nối.

Thông tin cần in khi disconnect:

- connection ID;
- disconnect reason;
- số client còn lại;
- xác nhận advertising đã được khởi động lại.

Log dự kiến:

```text
BLE_READY name=ESP32-POC2-BLE service=<uuid>
BLE_ADVERTISING
BLE_CONNECTED peer=AA:BB:CC:DD:EE:FF conn_id=0 addr_type=1 clients=1
BLE_DISCONNECTED conn_id=0 reason=19 clients=0
BLE_ADVERTISING_RESTARTED
```

Điện thoại có thể sử dụng địa chỉ BLE ngẫu nhiên vì cơ chế privacy. Peer address
chỉ dùng để chẩn đoán session, không dùng làm identity hoặc authorization.

## 8. Client kiểm thử

Client cần có khả năng scan, connect và đọc GATT characteristic:

- Android hoặc iOS: một BLE scanner/GATT client;
- macOS: một BLE GATT client hoặc Web Bluetooth trên trình duyệt hỗ trợ;
- lọc theo device name và service UUID, không chỉ dựa vào tên hiển thị.

Màn hình pairing Bluetooth của hệ điều hành không phải bằng chứng đầy đủ cho một
GATT connection. Client phải đọc được characteristic `ready`.

## 9. Tiêu chí nghiệm thu

POC hoàn thành khi:

- firmware build bằng BLE library có sẵn trong Arduino-ESP32 2.0.17;
- client thấy đúng tên và service UUID;
- client kết nối thành công;
- Serial thật in `BLE_CONNECTED` cùng peer, connection ID và client count;
- client đọc được characteristic `ready`;
- Serial in disconnect reason khi client ngắt kết nối;
- ESP32 quảng bá lại;
- client reconnect thành công ít nhất hai chu kỳ.

Không được kết luận thành công chỉ từ compile hoặc output Wokwi.

## 10. Cổng kiểm chứng

### Compile gate

```bash
pio run -d pocs/poc2-bluetooth -e esp32dev

test -f pocs/poc2-bluetooth/.pio/build/esp32dev/firmware.elf
test -f pocs/poc2-bluetooth/.pio/build/esp32dev/firmware.bin

pio pkg list -d pocs/poc2-bluetooth -e esp32dev
```

Package list phải xác nhận Arduino-ESP32 2.0.17 trước khi dựa vào callback API
đã mô tả.

### Runtime gate trên board thật

```bash
pio device list

pio run -d pocs/poc2-bluetooth -e esp32dev \
  -t upload --upload-port <PORT>

pio device monitor -p <PORT> -b 115200
```

Trong lúc monitor đang mở:

1. xác nhận log `BLE_ADVERTISING`;
2. connect từ BLE client;
3. quan sát `BLE_CONNECTED`;
4. đọc characteristic `ready`;
5. disconnect và quan sát `BLE_DISCONNECTED`;
6. xác nhận advertising restart;
7. reconnect lần nữa.

## 11. Nguồn chính thức

- [Wokwi ESP32 simulation feature matrix](https://docs.wokwi.com/guides/esp32)
- [Arduino-ESP32 2.0.17 release](https://github.com/espressif/arduino-esp32/releases/tag/2.0.17)
- [BLE server example đúng tag 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/BLE/examples/BLE_server/BLE_server.ino)
- [BLE UART example đúng tag 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/BLE/examples/BLE_uart/BLE_uart.ino)
- [BLEServer callback API đúng tag 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/BLE/src/BLEServer.h)
- [ESP-IDF 4.4.7 GATT Server API](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/bluetooth/esp_gatts.html)
- [PlatformIO ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)

## 12. Trạng thái

Firmware BLE GATT server đã được triển khai và build pass. Wokwi không hỗ trợ
Bluetooth, còn máy kiểm chứng chưa có ESP32 qua USB, nên chưa có bằng chứng
runtime từ radio thật. Xem `pocs/poc2-bluetooth/STATUS.md`.
