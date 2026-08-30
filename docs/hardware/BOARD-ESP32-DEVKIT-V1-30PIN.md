# ESP32 DevKit V1 (30 chân) — Sơ đồ chân và Tài liệu phần cứng

Tài liệu này ghi nhận đặc tính phần cứng, sơ đồ pinout và các lưu ý kỹ thuật đối với board mạch chính của dự án: **ESP32 DevKit V1 (phiên bản 30 chân)**.

---

## 1. Thông số tổng quan

- **Vi điều khiển:** ESP32-WROOM-32 (Tensilica Xtensa Dual-Core 32-bit LX6, xung nhịp lên đến 240 MHz).
- **Bộ nhớ:** 520 KB SRAM, 4 MB SPI Flash tích hợp.
- **Kết nối không dây:** Wi-Fi 802.11 b/g/n (2.4 GHz) & Bluetooth v4.2 BR/EDR / BLE.
- **Điện áp hoạt động:** 3.3V logic (nguồn cấp qua cổng USB 5V qua IC hạ áp 3.3V tích hợp AMS1117/LM1117 hoặc chân VIN 5V).
- **Dòng điện ngõ ra tối đa mỗi GPIO:** Khuyến nghị < 12mA (tối đa tuyệt đối 40mA per pin, tổng toàn chip < 200mA).
- **Mã phần tử Wokwi tương ứng:** `board-esp32-devkit-v1` (hoặc `board-esp32-devkit-c-v4`).

---

## 2. Sơ đồ chân (Pinout Map 30-Pin)

Board có 2 hàng chân (mỗi hàng 15 chân). Nhìn từ trên xuống, cổng Micro-USB / USB-C hướng xuống dưới:

```text
                  ┌──────────────────┐
                  │   ESP32-WROOM    │
                  │   ANTENNA PCB    │
                  │                  │
            EN  ──┤ 1              30├──  D23 (VSPI MOSI / GPIO23)
      (VP) D36  ──┤ 2              29├──  D22 (I2C SCL / GPIO22)
      (VN) D39  ──┤ 3              28├──  TX0 (UART0 TX / GPIO1)
           D34  ──┤ 4              27├──  RX0 (UART0 RX / GPIO3)
           D35  ──┤ 5              26├──  D21 (I2C SDA / GPIO21)
           D32  ──┤ 6              25├──  D19 (VSPI MISO / GPIO19)
           D33  ──┤ 7              24├──  D18 (VSPI SCK / GPIO18)
           D25  ──┤ 8              23├──  D5  (VSPI CS / GPIO5)
           D26  ──┤ 9              22├──  TX2 (UART2 TX / GPIO17)
           D27  ──┤ 10             21├──  RX2 (UART2 RX / GPIO16)
           D14  ──┤ 11             20├──  D4  (ADC2_CH0 / GPIO4)
           D12  ──┤ 12             19├──  D2  (LED tích hợp / GPIO2)
           D13  ──┤ 13             18├──  D15 (ADC2_CH3 / GPIO15)
           GND  ──┤ 14             17├──  GND (Mass chung)
           VIN  ──┤ 15             16├──  3V3 (Nguồn ra 3.3V)
                  └──────[ USB ]─────┘
```

---

## 3. Phân loại và chức năng chi tiết các chân

### 3.1 Chân chỉ có thể làm ngõ vào (Input-Only Pins)
- **GPIO 34, 35, 36 (VP), 39 (VN)**:
  - Chỉ hỗ trợ chế độ `INPUT`.
  - **Không có** điện trở kéo lên/kéo xuống nội bộ (`INPUT_PULLUP` / `INPUT_PULLDOWN` không hoạt động).
  - Không thể cấu hình làm `OUTPUT`. Thích hợp nhất làm chân đọc Analog (ADC1) hoặc cảm biến ngoài có mạch kéo sẵn.

### 3.2 Strapping Pins (Chân cấu hình khởi động - Cần đặc biệt lưu ý)
Các chân này quyết định chế độ boot của chip khi cấp nguồn hoặc nhấn Reset:
- **GPIO 0**: Phải ở mức `LOW` để vào Bootloader mode (nhấn giữ nút BOOT); mức `HIGH` khi boot bình thường.
- **GPIO 2**: Phải ở mức `LOW` hoặc thả nổi khi boot; có nối với LED xanh tích hợp trên nhiều board.
- **GPIO 12 (MTDI)**: Nếu bị kéo `HIGH` khi boot có thể làm sai điện áp cấp Flash (1.8V thay vì 3.3V) khiến chip bị treo khởi động.
- **GPIO 15**: Phải ở mức `HIGH` trong khi boot để bật debug output từ ROM.

> ⚠️ **Quy tắc vàng:** Tránh dùng các chân Strapping (đặc biệt là GPIO 0, 2, 12, 15) cho các tải ngoài có thể vô tình kéo điện áp lên/xuống khi mạch khởi động.

### 3.3 Kênh Analog-to-Digital Converter (ADC)
- **ADC1 (8 kênh):** GPIO 36 (VP), 39 (VN), 34, 35, 32, 33, 25, 26.
  - Hoạt động bình thường **kể cả khi đang bật Wi-Fi**.
- **ADC2 (10 kênh):** GPIO 4, 0, 2, 15, 13, 12, 14, 27, 25, 26.
  - ⚠️ **Hạn chế:** ADC2 **bị vô hiệu hoá** khi Wi-Fi driver đang chạy. Luôn ưu tiên dùng **ADC1** để đọc cảm biến analog.

### 3.4 Giao tiếp ngoại vi mặc định
| Chuẩn giao tiếp | Chân mặc định | Ghi chú |
|---|---|---|
| **UART0** | TX0 (GPIO1), RX0 (GPIO3) | Kênh nạp firmware và Serial Monitor chính qua cổng USB |
| **UART2** | TX2 (GPIO17), RX2 (GPIO16) | Kênh Serial phụ tiện lợi để giao tiếp module GPS, GSM, v.v. |
| **I2C** | SDA (GPIO21), SCL (GPIO22) | Dùng cho màn hình OLED SSD1306, cảm biến RTC, IMU |
| **SPI (VSPI)**| MOSI (GPIO23), MISO (GPIO19), SCK (GPIO18), CS (GPIO5) | Dùng cho thẻ nhớ SD, màn hình TFT, module RFID |
| **DAC (Digital to Analog)** | DAC1 (GPIO25), DAC2 (GPIO26) | Xuất điện áp tương tự thực (8-bit) |
| **Touch Pins (Cảm ứng điện dung)** | T0(GPIO4), T2(GPIO2), T3(GPIO15), T4(GPIO13), T5(GPIO12), T6(GPIO14), T7(GPIO27), T8(GPIO33), T9(GPIO32) | Đọc cảm ứng chạm không cần nút vật lý |

---

## 4. Bảng tra cứu GPIO an toàn khi thiết kế mạch

| GPIO | An toàn dùng làm Output? | An toàn dùng làm Input? | Ghi chú |
|---|:---:|:---:|---|
| **GPIO 1, 3** | ❌ (Tránh) | ❌ (Tránh) | Chân USB TX/RX Serial; dùng sẽ mất Serial log |
| **GPIO 6 – 11** | ❌ (Cấm) | ❌ (Cấm) | Đã nối trực tiếp với SPI Flash nội bộ; dùng sẽ gây crash ngay lập tức |
| **GPIO 34, 35, 36, 39** | ❌ | ✅ (Tốt) | Chỉ Input, không có pull-up nội, thuộc ADC1 |
| **GPIO 0, 2, 12, 15** | ⚠️ (Cẩn thận) | ⚠️ (Cẩn thận) | Strapping pins, kiểm tra mức logic khi khởi động |
| **GPIO 4, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33** | ✅ (Tuyệt vời) | ✅ (Tuyệt vời) | Hoàn toàn tự do sử dụng cho GPIO thông thường |

---

## 5. Nguyên tắc cấp nguồn và bảo vệ
1. **Nguồn 3.3V Logic:** Mọi chân GPIO của ESP32 chạy ở mức 3.3V. **Không** cấp tín hiệu 5V trực tiếp vào GPIO (cần dùng cầu phân áp hoặc mạch chuyển mức logic - Logic Level Shifter).
2. **Hạn dòng LED:** Luôn mắc nối tiếp điện trở hạn dòng (khuyến nghị **220Ω** hoặc **330Ω**) cho mỗi LED nối với GPIO.
3. **Cấp nguồn VIN:** Chân VIN có thể nhận nguồn 5V từ cổng USB hoặc nguồn ngoài 5V ổn áp. Không cấp quá 6V vào chân VIN.
