# Hướng dẫn Nạp code & Vận hành trên Board Thật (macOS)

Tài liệu này ghi nhận quy trình thực tế từ lúc cắm cáp USB vào máy Mac cho đến khi nạp firmware thành công và theo dõi log Serial trên board **ESP32 DevKit V1 30-pin**.

---

## 1. Tóm tắt các bước thực hiện

```text
[Cắm cáp Data USB] ──► [Nhận diện cổng /dev/cu.*] ──► [Vào Bootloader: Giữ BOOT, nhấn EN] ──► [pio run -t upload] ──► [pio device monitor]
```

---

## 2. Chuẩn bị cáp và Nhận diện cổng Serial

### 2.1 Kiểm tra cáp USB
- **Bắt buộc dùng cáp truyền dữ liệu (Data Cable)** có đủ đường tín hiệu D+ / D-.
- Dấu hiệu nhận biết cáp sạc-only (Charge-only): Cắm board vào máy tính nhưng đèn LED nguồn trên board sáng mà lệnh `pio device list` hoàn toàn không hiện cổng mới.

### 2.2 Driver chip USB-to-UART trên macOS
ESP32 DevKit V1 thường sử dụng một trong các chip cầu nối USB-to-UART sau:
| Chip in trên IC gần cổng USB | Tên driver trên macOS | Tên cổng Serial sinh ra (`/dev/cu.*`) | Nguồn tải Driver (nếu chưa nhận) |
|---|---|---|---|
| **CP2102 / CP2104** (Silicon Labs) | Silicon Labs VCP Driver | `/dev/cu.usbserial-XXXX` hoặc `/dev/cu.SLAB_USBtoUART` | [Silicon Labs CP210x Drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) |
| **CH340 / CH340G / CH341** (WCH) | WCH CH34x Driver | `/dev/cu.wchusbserialXXXX` hoặc `/dev/cu.usbmodemXXXX` | [WCH CH341SER Drivers](https://www.wch.cn/downloads/CH341SER_ZIP.html) |
| **FT232R / FT232H** (FTDI) | macOS tích hợp sẵn | `/dev/cu.usbserial-XXXX` | Có sẵn trong macOS (AppleFTDI) |

### 2.3 Lệnh kiểm tra cổng trên Mac
```bash
pio device list
# hoặc dùng lệnh hệ thống:
ls -l /dev/cu.usb* /dev/cu.wch* /dev/cu.SLAB* 2>/dev/null
```

---

## 3. Quy trình đưa ESP32 vào chế độ nạp (Bootloader / Download Mode)

Một số mạch nạp tự động (auto-reset circuit) bằng 2 transistor DTR/RTS có thể hoạt động không ổn định trên một số hub USB hoặc máy Mac. Khi gặp lỗi nạp `A fatal error occurred: Failed to connect to ESP32: No serial data received`, hãy kích hoạt chế độ nạp thủ công:

1. **Bước 1:** Nhấn và **giữ im nút BOOT** (nút bên phải cổng USB, đôi khi in là `IO0`).
2. **Bước 2:** Trong khi vẫn đang giữ BOOT, **nhấn và thả nút EN / RST** (nút bên trái) một lần.
3. **Bước 3:** **Thả nút BOOT**. Chip ESP32 lúc này đã vào trạng thái ROM Bootloader sẵn sàng nhận code.
4. **Bước 4:** Thực thi lệnh nạp code:
   ```bash
   pio run -t upload --upload-port /dev/cu.usbserial-XXXX
   ```

---

## 4. Theo dõi Serial Monitor

Sau khi nạp xong, ESP32 sẽ tự khởi động lại. Chạy lệnh mở Serial Monitor với baud rate tiêu chuẩn 115200:
```bash
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```
- **Thoát Monitor:** Nhấn `Ctrl + ]`.
- **Nếu thấy xuất hiện ký tự rác:** Kiểm tra lại `monitor_speed = 115200` trong `platformio.ini` và baud rate trong code `Serial.begin(115200)`.

---

## 5. Cấp nguồn an toàn cho Board thật

| Tình huống | Nguồn cấp khuyến nghị | Lưu ý |
|---|---|---|
| **Nạp code + Debug** | Cáp USB cắm trực tiếp vào cổng USB máy tính | Cổng USB máy Mac cấp dòng 5V (tối đa 1.5A–3A), dư sức đáp ứng mức đỉnh Wi-Fi của ESP32 (~350mA). |
| **Vận hành độc lập (Demo)** | Củ sạc USB 5V (≥ 1A) hoặc sạc dự phòng | Không dùng sạc dự phòng có tính năng tự ngắt thông minh khi dòng quá nhỏ (Auto-shutoff). |
| **Nguồn ngoài qua chân VIN** | Nguồn 5V DC ổn áp | Cấp vào chân `VIN` và `GND`. Không cấp quá 6V. |

---

## 6. Bảng chẩn đoán sự cố thường gặp (Troubleshooting)

| Triệu chứng | Nguyên nhân có thể | Cách xử lý |
|---|---|---|
| `pio device list` không thấy cổng | Cáp sạc-only, hub USB lỗi, thiếu driver | Đổi cáp dữ liệu xịn, cắm trực tiếp cổng Mac, cài lại driver CP210x/CH340 và reboot. |
| `A fatal error occurred: Timed out waiting for packet header` | Chưa vào Bootloader mode | Giữ BOOT, nhấn EN, thả BOOT rồi bấm nạp lại ngay. Có thể hàn tụ 10µF giữa EN và GND để auto-reset ổn định hơn. |
| Monitor không in gì sau khi nạp | Chip đang ở chế độ Bootloader hoặc nguồn yếu | Nhấn nút EN/RST trên board một lần để chip khởi động vào firmware chính. |
| Muốn xoá sạch cấu hình Wi-Fi / NVS cũ | NVS partition còn lưu cache | Chạy lệnh `pio run -t erase --upload-port <PORT>` rồi nạp lại. |
