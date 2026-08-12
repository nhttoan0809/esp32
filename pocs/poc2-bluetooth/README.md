# POC 2 — ESP32 BLE: phát hiện client kết nối

Project độc lập này biến ESP32 classic thành BLE GATT server. Firmware quảng
bá một service, ghi session connect/disconnect ra UART0 và cung cấp một
characteristic chỉ đọc có giá trị `ready`.

## Cấu hình cố định

| Thành phần | Giá trị |
|---|---|
| Board PlatformIO | `esp32dev` (ESP32 classic) |
| Platform | `espressif32@7.0.1` |
| Framework | Arduino-ESP32 2.0.17 |
| Serial | 115200 baud |
| Device name | `ESP32-POC2-BLE` |
| Service UUID | `3b8e21c0-56d6-4d4e-9c5b-1eb12a20c002` |
| Characteristic UUID | `3b8e21c1-56d6-4d4e-9c5b-1eb12a20c002` |
| Characteristic property/value | `READ` / `ready` |

Không dùng các UUID trên làm thông tin xác thực. Địa chỉ peer trong log cũng
chỉ phục vụ chẩn đoán session vì BLE client có thể dùng địa chỉ ngẫu nhiên.

## Compile gate

Chạy từ root repository:

```bash
pio run -d pocs/poc2-bluetooth -e esp32dev

test -f pocs/poc2-bluetooth/.pio/build/esp32dev/firmware.elf
test -f pocs/poc2-bluetooth/.pio/build/esp32dev/firmware.bin

pio pkg list -d pocs/poc2-bluetooth -e esp32dev
```

Nếu `pio` chưa có trong `PATH`, dùng
`${HOME}/.platformio/penv/bin/pio`. Package framework được PlatformIO hiển thị
dưới dạng `framework-arduinoespressif32 @ 3.20017...`; phần `20017` tương ứng
Arduino-ESP32 2.0.17.

## Runtime gate trên ESP32 thật

Bluetooth không được Wokwi mô phỏng, vì vậy POC này cố ý không có
`diagram.json` hoặc `wokwi.toml`. Build thành công không phải bằng chứng BLE
runtime. Cần ESP32 classic thật và một BLE scanner/GATT client:

```bash
pio device list

pio run -d pocs/poc2-bluetooth -e esp32dev \
  -t upload --upload-port <PORT>

pio device monitor -p <PORT> -b 115200
```

Nếu monitor được mở sau khi firmware đã boot, nhấn nút EN/reset trên board để
quan sát lại log khởi động. Sau đó:

1. Scan và lọc đồng thời theo tên `ESP32-POC2-BLE` và service UUID ở trên.
2. Connect; Serial phải có `BLE_CONNECTED` cùng peer, connection ID, address
   type và client count.
3. Đọc characteristic UUID ở trên và xác nhận giá trị UTF-8 là `ready`.
4. Disconnect; Serial phải có `BLE_DISCONNECTED`, reason và client count.
5. Chờ `BLE_ADVERTISING_RESTARTED`, scan/connect/read lại lần thứ hai.

Màn hình pairing của hệ điều hành không thay thế phép đọc GATT. Một phiên hoàn
chỉnh có dạng:

```text
BLE_READY name=ESP32-POC2-BLE service=3b8e21c0-56d6-4d4e-9c5b-1eb12a20c002
BLE_ADVERTISING
BLE_CONNECTED peer=AA:BB:CC:DD:EE:FF conn_id=0 addr_type=1 clients=1
BLE_DISCONNECTED conn_id=0 reason=19 clients=0
BLE_ADVERTISING_RESTARTED
```

`BLE_ADVERTISING_RESTARTED` xác nhận firmware đã gọi lại advertising sau thời
gian chờ 500 ms. Việc client scan và reconnect thành công mới xác nhận radio
thực sự quảng bá lại.

## Giới hạn Wokwi

Feature matrix chính thức của Wokwi đánh dấu Bluetooth là không hỗ trợ trên
ESP32. Trạng thái BLE khi chạy bằng Wokwi phải được ghi là
`N/A — unsupported`, không phải pass hoặc fail. Wokwi chỉ có thể hữu ích cho
những kiểm tra khác như boot/UART nếu project được bổ sung cấu hình mô phỏng về
sau.

## Nguồn chính thức

- [Arduino-ESP32 2.0.17 release](https://github.com/espressif/arduino-esp32/releases/tag/2.0.17)
- [BLE server example tại tag 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/BLE/examples/BLE_server/BLE_server.ino)
- [BLEServer callback API tại tag 2.0.17](https://github.com/espressif/arduino-esp32/blob/2.0.17/libraries/BLE/src/BLEServer.h)
- [ESP-IDF 4.4.7 GATT Server API](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/bluetooth/esp_gatts.html)
- [PlatformIO ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)
- [Wokwi ESP32 feature matrix](https://docs.wokwi.com/guides/esp32)
