# POC MOB-03: Bàn Xoay Quét 3D & Chụp Ảnh Sản Phẩm Chính Xác (Precision Stepper Turntable)

Hệ thống bàn xoay tự động góc 360° chính xác cao phục vụ phòng lab chụp ảnh sản phẩm đa góc để dựng mô hình 3D (Photogrammetry), tạo ảnh xoay 360° thương mại điện tử và kiểm thử cảm biến xoay tròn góc.

Điều khiển bằng **ESP32 DevKit V1 (30 chân)** kết hợp động cơ bước **28BYJ-48 + ULN2003 Driver**, bàn phím ma trận **4x4 Keypad**, màn hình **OLED SSD1306 0.96"**, đồng hồ thời gian thực **RTC DS1307/DS3231** và **Relay kích màn trập máy ảnh**.

---

## 1. Danh Sách Linh Kiện Phần Cứng

| STT | Tên Linh Kiện / Module | Số Lượng | Điện Áp Hoạt Động | Chức Năng Kỹ Thuật |
|:---:|---|:---:|:---:|---|
| 1 | Bo mạch **ESP32 DevKit V1 (30 chân)** | 1 | 5V (USB) / 3.3V Logic | Vi điều khiển trung tâm xử lý logic và tính xung bước |
| 2 | Động cơ bước **28BYJ-48 (5V)** | 1 | 5V DC | Dẫn động đĩa xoay với hộp giảm tốc 1:64 |
| 3 | Mạch đệm công suất **ULN2003 Driver** | 1 | 5V DC (Nguồn ngoài) | Đệm dòng kích 4 cuộn pha động cơ bước |
| 4 | **Bàn phím ma trận 4x4 (Matrix Keypad)** | 1 | 3.3V Logic | Nhập số góc quay, chọn preset số ảnh, Start/Stop/Cancel |
| 5 | Màn hình **OLED 0.96" I2C (SSD1306)** | 1 | 3.3V DC | Hiển thị góc quay, số ảnh, trạng thái màn trập, giờ RTC |
| 6 | Module **RTC I2C (DS1307 / DS3231)** | 1 | 3.3V / 5V DC | Đồng hồ thời gian thực, lưu timestamp từng ảnh chụp |
| 7 | Module **Relay 1 kênh 5V (Optocoupler)** | 1 | 5V DC (Cuộn hút) | Giả lập nút bấm kích màn trập máy ảnh (Shutter Trigger) |
| 8 | Còi báo **Active Buzzer** (hoặc LED đơn) | 1 | 3.3V DC | Bíp âm thanh phản hồi khi bấm phím và chụp ảnh |
| 9 | Đĩa xoay bàn chụp (Mica / Carton / In 3D) | 1 | Cơ khí | Gắn trực tiếp lên trục vát 5mm của động cơ 28BYJ-48 |

---

## 2. Bảng Ánh Xạ Chân Chuẩn Hóa (Pinout Mapping Table)

Tuân thủ nghiêm ngặt **Quy tắc 7 (Hardware Pinout Verification Rule)** trong `AGENTS.md`. Toàn bộ các chân GPIO được phân bổ 100% Boot-Safe (không dùng GPIO 12 strapping, không dùng các chân chỉ ngõ vào GPIO 34–39 cho ma trận phím):

| Chân ESP32 DevKit V1 | Ký Hiệu In Trên Bo Module Thật | Chân Tương Ứng Wokwi (`diagram.json`) | Chức Năng Kỹ Thuật & Lưu Ý An Toàn |
|---|---|---|---|
| **GPIO 21** | `SDA` (OLED & RTC Module) | `oled:SDA` & `rtc:SDA` | Tuyến dữ liệu I2C dùng chung cho OLED SSD1306 và RTC DS1307 |
| **GPIO 22** | `SCL` (OLED & RTC Module) | `oled:SCL` & `rtc:SCL` | Tuyến xung nhịp I2C dùng chung cho OLED SSD1306 và RTC DS1307 |
| **3V3** | `VCC` / `VDD` (OLED 0.96") | `oled:VCC` | Nguồn nuôi màn hình OLED (3.3V chuẩn an toàn logic) |
| **VIN (5V)** | `VCC` (RTC) & `VCC` (Relay) | `rtc:5V` & `relay:VCC` | Cấp nguồn 5V cho mạch RTC và cuộn hút Relay |
| **GND** | `GND` / `-` (Toàn bộ thiết bị) | `esp:GND.1` & `esp:GND.2` | Mass chung toàn hệ thống (ESP32, RTC, OLED, Relay, Motor) |
| **GPIO 19** | `IN1` (Bo đệm ULN2003) | `stepper:A+` | Xung điều khiển cuộn pha A động cơ bước |
| **GPIO 18** | `IN2` (Bo đệm ULN2003) | `stepper:A-` | Xung điều khiển cuộn pha B động cơ bước |
| **GPIO 5** | `IN3` (Bo đệm ULN2003) | `stepper:B+` | Xung điều khiển cuộn pha C động cơ bước |
| **GPIO 17** | `IN4` (Bo đệm ULN2003) | `stepper:B-` | Xung điều khiển cuộn pha D động cơ bước |
| **GPIO 23** | `IN` (Module Relay 1 kênh) | `relay:IN` | Kích cuộn hút Relay đóng tiếp điểm Shutter (Active LOW) |
| **GPIO 2** | `+` (Active Buzzer / LED) | `buzzer:2` | Còi bíp âm báo trạng thái & LED tích hợp trên board |
| **GPIO 13** | `R1` (Hàng 1 Bàn phím 4x4) | `keypad:R1` | Quét ma trận Hàng 1 (Phím 1, 2, 3, A) - OUTPUT |
| **GPIO 14** | `R2` (Hàng 2 Bàn phím 4x4) | `keypad:R2` | Quét ma trận Hàng 2 (Phím 4, 5, 6, B) - OUTPUT |
| **GPIO 27** | `R3` (Hàng 3 Bàn phím 4x4) | `keypad:R3` | Quét ma trận Hàng 3 (Phím 7, 8, 9, C) - OUTPUT |
| **GPIO 4** | `R4` (Hàng 4 Bàn phím 4x4) | `keypad:R4` | Quét ma trận Hàng 4 (Phím *, 0, #, D) - OUTPUT |
| **GPIO 26** | `C1` (Cột 1 Bàn phím 4x4) | `keypad:C1` | Đọc ma trận Cột 1 (Phím 1, 4, 7, *) - INPUT_PULLUP nội bộ |
| **GPIO 25** | `C2` (Cột 2 Bàn phím 4x4) | `keypad:C2` | Đọc ma trận Cột 2 (Phím 2, 5, 8, 0) - INPUT_PULLUP nội bộ |
| **GPIO 33** | `C3` (Cột 3 Bàn phím 4x4) | `keypad:C3` | Đọc ma trận Cột 3 (Phím 3, 6, 9, #) - INPUT_PULLUP nội bộ |
| **GPIO 32** | `C4` (Cột 4 Bàn phím 4x4) | `keypad:C4` | Đọc ma trận Cột 4 (Phím A, B, C, D) - INPUT_PULLUP nội bộ |

> ⚠️ **CẢNH BÁO NGUỒN CẤP ĐỘNG CƠ BƯỚC (Power Supply Safety):**
> * Tuyệt đối **không** cấp nguồn động lực cho chân `+` của bo ULN2003 từ chân `3V3` của ESP32!
> * Có thể cấp nguồn từ chân `VIN` (nếu nguồn cắm cổng USB đạt dòng $\ge 1.5\text{A}$) hoặc tốt nhất dùng **nguồn adapter 5V riêng** và **nối chung GND** với ESP32.

---

## 3. Đấu Nối Kích Màn Trập Máy Ảnh (Camera Shutter Wiring)

Hầu hết các dòng máy ảnh DSLR / Mirrorless (Canon, Sony, Nikon, Fujifilm) đều có cổng giắc cắm dây bấm mềm Remote Shutter (chuẩn giắc TRS 2.5mm hoặc 3.5mm):

```
       Cổng giắc âm máy ảnh (2.5mm / 3.5mm TRS)
       ┌───────────────────────────────┐
       │   TIP    : Shutter (Chụp)     ├──► Nối cọc NO (Normally Open) trên Relay
       │   RING   : Focus (Lấy nét)    ├──► Nối chung với TIP hoặc bỏ trống
       │   SLEEVE : GND (Mass máy ảnh) ├──► Nối cọc COM (Common) trên Relay
       └───────────────────────────────┘
```

* **Nguyên lý kích:** Khi Relay được kích hoạt (GPIO 23 = LOW), tiếp điểm `COM` và `NO` đóng lại $\rightarrow$ chân Shutter nối tắt vào GND máy ảnh $\rightarrow$ Máy ảnh lập tức chụp 1 bức ảnh.
* **Cách ly quang hoàn toàn:** Tiếp điểm cơ khí của Relay cách ly điện áp 100% giữa mạch vi điều khiển ESP32 và máy ảnh, đảm bảo an toàn tuyệt đối cho cổng điều khiển của máy ảnh đắt tiền.

---

## 4. Hướng Dẫn Vận Hành Hệ Thống

### 4.1 Ý Nghĩa Các Phím Trên Bàn Phím Ma Trận 4x4
* `[ A ]`: **Chế độ Chụp 360° Tự Động (Auto Photogrammetry Mode):**
  * Nhấn `1`: Preset 8 ảnh (bước góc $45^\circ$).
  * Nhấn `2`: Preset 12 ảnh (bước góc $30^\circ$).
  * Nhấn `3`: Preset 24 ảnh (bước góc $15^\circ$).
  * Nhấn `4`: Preset 36 ảnh (bước góc $10^\circ$).
  * Nhấn `0`: Tự nhập số khung hình tùy ý ($4 – 360$) và nhấn `#` để bắt đầu.
* `[ B ]`: **Chế độ Xoay Góc Tự Chọn (Manual Angle Mode):**
  * Gõ số góc mong muốn ($0 – 360^\circ$) trên bàn phím số và nhấn `#`. Động cơ sẽ quay êm ái đến đúng góc chỉ định.
  * Khi đã ở góc chỉ định, nhấn `#` một lần nữa để kích chụp thủ công 1 phát ảnh.
* `[ C ]`: **Chế độ Bàn Xoay Trưng Bày Liên Tục (Continuous Showroom Spin):**
  * Đĩa xoay liên tục êm ái.
  * Nhấn `1`: Đảo chiều quay Thuận (CW) / Nghịch (CCW).
  * Nhấn `2`: Tăng tốc độ quay (+100 steps/sec).
  * Nhấn `3`: Giảm tốc độ quay (-100 steps/sec).
* `[ D ]`: **Chế độ Chẩn Đoán & Thông Tin (Diagnostics & Info):**
  * Hiển thị trạng thái RTC DS1307/DS3231, giờ thực tế, vị trí bước hiện tại.
  * Nhấn `1`: Thử nghiệm đóng ngắt Relay (Click test).
* `[ # ]`: Phím **Xác nhận (ENTER)** hoặc **Kích chụp thủ công (Manual Shoot)**.
* `[ * ]`: Phím **HỦY / DỪNG KHẨN CẤP (EMERGENCY STOP):**
  * Trong mọi chế độ xoay, nhấn `*` sẽ lập tức hãm dừng động cơ, ngắt điện 4 cuộn dây motor và mở Relay.
  * Khi ở màn hình chính, nhấn `*` sẽ đưa bàn xoay về vạch gốc $0.0^\circ$ (Homing).

---

## 5. Quy Trình Kiểm Chứng Kỹ Thuật (CLI Verification)

Thực hiện lần lượt các bước kiểm tra theo quy chuẩn PlatformIO và Wokwi:

```bash
# 1. Kiểm tra cú pháp sơ đồ Wokwi
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc-mob-03-precision-stepper-turntable/diagram.json", "utf8"))' && echo "Diagram JSON OK"

# 2. Biên dịch firmware PlatformIO
pio run -d pocs/poc-mob-03-precision-stepper-turntable -e esp32dev

# 3. Xác nhận binary artifact
test -f pocs/poc-mob-03-precision-stepper-turntable/.pio/build/esp32dev/firmware.bin && echo "Firmware BIN OK"
test -f pocs/poc-mob-03-precision-stepper-turntable/.pio/build/esp32dev/firmware.elf && echo "Firmware ELF OK"

# 4. Nạp code lên board thật (macOS)
pio run -d pocs/poc-mob-03-precision-stepper-turntable -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# 5. Mở Serial Monitor theo dõi log
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```
