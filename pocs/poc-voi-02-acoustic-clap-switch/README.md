# POC: VOI-02 Acoustic Clap Switch

Hệ thống công tắc kích hoạt bằng nhịp vỗ tay đa kênh (Multi-Channel Acoustic Clap Switch) điều khiển 2 phụ tải độc lập thông qua cảm biến âm thanh **HW-484 (LM393)**, **Relay 2 kênh 5V**, còi báo **Passive Buzzer** và màn hình **OLED 0.96" SSD1306** trên vi điều khiển **ESP32 DevKit V1 (30 chân)**.

---

## 1. Tính Năng Kỹ Thuật Nổi Bật

- **Điều khiển đa kênh rảnh tay (Hands-Free Multi-Channel Control):**
  - **1 tiếng vỗ tay ($N = 1$):** Đảo trạng thái Relay Kênh 1 (`CH1: TOGGLE` — Đèn chiếu sáng).
  - **2 tiếng vỗ tay nhịp nhanh ($N = 2$):** Đảo trạng thái Relay Kênh 2 (`CH2: TOGGLE` — Quạt làm mát).
  - **$\ge 3$ tiếng vỗ tay ($N \ge 3$):** Tắt toàn bộ thiết bị (`ALL OFF` — Master Switch an toàn).
- **Thuật toán khử nhiễu dội âm (Acoustic Debouncing Algorithm):**
  - Thời gian câm chặn dội âm (**Blanking Period**) $150\text{ms}$ triệt tiêu hoàn toàn hiện tượng rung màng mic và tiếng vang trong phòng kín.
  - Cửa sổ phân tích nhịp (**Detection Window**) $650\text{ms}$ linh hoạt tự động gia hạn sau mỗi tiếng vỗ kế tiếp.
- **Triệt tiêu phản hồi âm học cơ khí Relay (Relay Click Acoustic Lockout):**
  - Khi tiếp điểm relay đóng/ngắt, tiếng "tách" cơ học có thể làm mic kích hoạt nhầm. Firmware trang bị thời gian cách ly (**Cooldown Period**) $450\text{ms}$ khóa ngắt âm thanh và xóa cờ rác sau mỗi lần chuyển trạng thái relay.
- **Phản hồi âm học & hiển thị trực quan (Acoustic & Visual Feedback):**
  - Còi **Passive Buzzer** phát tiếng "Tick" xác nhận tức thì mỗi nhịp vỗ, phát giai điệu xác nhận thực thi lệnh hoặc âm trầm khi tắt toàn bộ.
  - Màn hình **OLED SSD1306** hiển thị thước đo mức âm thanh (Sound VU Meter), thanh đếm ngược cửa sổ nhịp (Countdown Bar), trạng thái kênh tải và banner thông báo.

---

## 2. Phần Cứng Yêu Cầu (Hardware Requirements)

| Linh kiện | Số lượng | Ghi chú kỹ thuật |
|---|:---:|---|
| **ESP32 DevKit V1 (30 chân)** | 1 | Vi điều khiển trung tâm (Dual-Core 240MHz, tích hợp ADC1 và ngắt ngoài) |
| **Cảm biến Âm thanh HW-484 (LM393)** | 1 | Module mic điện dung + IC so sánh LM393 + chiết áp vi chỉnh độ nhạy |
| **Module Relay 2 kênh 5V** | 1 | Optocoupler cách ly quang, tiếp điểm NO/NC/COM, kích Active LOW |
| **Màn hình OLED 0.96" SSD1306** | 1 | Giao tiếp I2C monochrome 128×64 pixels (địa chỉ `0x3C`) |
| **Còi báo Passive Buzzer** | 1 | Còi thụ động phát âm tần số đa sắc qua xung PWM (LEDC) |
| **LED Xanh lá (Status CH1)** | 1 | Báo trạng thái Relay Kênh 1 |
| **LED Xanh dương (Status CH2)** | 1 | Báo trạng thái Relay Kênh 2 |
| **Điện trở hạn dòng 220Ω** | 2 | Mắc nối tiếp bảo vệ 2 LED |
| **Breadboard MB102 & Cáp Jumper** | 1 | Bo mạch cắm thử nghiệm và dây nối Đực-Đực, Đực-Cái |

---

## 3. Sơ Đồ Kết Nối Mạch (Wiring Diagram)

### 3.1 Sơ đồ Phía Điều Khiển DC (Low-Voltage DC Side)

```text
                        ESP32 DevKit V1 (30 chân)
                        ┌───────────────────────┐
                        │                       │
      [HW-484 DO] ◄─────┤ GPIO 17 (TX2)         │
      [HW-484 AO] ◄─────┤ GPIO 34 (D34 / ADC1)  │
                        │                       │
    [OLED 0.96" SDA] ◄──┤ GPIO 21 (D21 / SDA)   │
    [OLED 0.96" SCL] ◄──┤ GPIO 22 (D22 / SCL)   │
                        │                       │
     [Relay IN1: CH1] ◄─┤ GPIO 26 (D26)         │ (Active LOW)
     [Relay IN2: CH2] ◄─┤ GPIO 25 (D25)         │ (Active LOW)
                        │                       │
   [Passive Buzzer +] ◄─┤ GPIO 19 (D19)         │ (PWM Tone)
                        │                       │
  [LED CH1]──[220Ω] ◄───┤ GPIO 13 (D13)         │
  [LED CH2]──[220Ω] ◄───┤ GPIO 14 (D14)         │
                        │                       │
     Nguồn cấp 3.3V ────┤ 3V3                   │ ──► VCC (HW-484, OLED)
     Nguồn cấp 5.0V ────┤ VIN                   │ ──► VCC (Module Relay 5V)
          Nối đất ──────┤ GND                   │ ──► GND (Chung toàn hệ thống)
                        └───────────────────────┘
```

### 3.2 Sơ đồ Phụ Tải Phía AC / DC Ngoài (Tải Relay An Toàn)

```text
[Nguồn Tải 12V/24V hoặc Đèn/Quạt]
 │
 ├── Dây Dương / Pha (L) ──────► Relay 1 COM
 │                                   ↕ Tiếp điểm NO
 │                               Relay 1 NO ──────► [Tải Đèn CH1] ──► Dây Âm / N
 │
 └── Dây Dương / Pha (L) ──────► Relay 2 COM
                                     ↕ Tiếp điểm NO
                                 Relay 2 NO ──────► [Tải Quạt CH2] ──► Dây Âm / N
```

---

## 4. Bảng Ánh Xạ Chân (Hardware Pinout Map)

Bảng đối chiếu 4 cột bắt buộc theo quy chuẩn **Rule 7** của `AGENTS.md`:

| Chân ESP32 DevKit V1 | Ký hiệu in Module thực tế | Chân mô phỏng Wokwi (`diagram.json`) | Chức năng kỹ thuật & Ghi chú an toàn |
|---|:---:|:---:|---|
| **GPIO 17** | `DO` (Digital Out) | `btn_clap:1.r` (Pushbutton to GND) | Ngõ vào ngắt ngoài `FALLING`. Kéo xuống `LOW` khi micro thu được âm thanh vượt ngưỡng. |
| **GPIO 34** | `AO` (Analog Out) | `pot_sound:SIG` (Potentiometer) | Ngõ vào ADC1 (Input-Only). Đọc biên độ âm thanh liên tục dải 0–3.3V cho thước đo VU Meter OLED. |
| **GPIO 21** | `SDA` | `oled:SDA` | Kênh dữ liệu I2C truyền hình ảnh đồ họa tới màn hình OLED SSD1306. |
| **GPIO 22** | `SCL` | `oled:SCL` | Xung nhịp I2C Clock đồng bộ màn hình OLED (100kHz / 400kHz). |
| **GPIO 26** | `IN1` | `relay1:IN` | Kích cuộn hút Relay Kênh 1 (Active LOW: `LOW` = Đóng tiếp điểm BẬT tải, `HIGH` = TẮT). |
| **GPIO 25** | `IN2` | `relay2:IN` | Kích cuộn hút Relay Kênh 2 (Active LOW: `LOW` = Đóng tiếp điểm BẬT tải, `HIGH` = TẮT). |
| **GPIO 19** | `+` (Signal) | `bz1:2` | Ngõ ra xung PWM LEDC điều chế tần số âm điệu phản hồi cho còi Passive Buzzer. |
| **GPIO 13** | Anode (+) | `r1:1` ── `led1:A` | Đèn LED báo trạng thái Kênh 1 (qua trở $220\Omega$). Tránh chân strapping. |
| **GPIO 14** | Anode (+) | `r2:1` ── `led2:A` | Đèn LED báo trạng thái Kênh 2 (qua trở $220\Omega$). Tránh chân strapping. |
| **3V3** | `VCC` / `+` | `esp:3V3` | Nguồn cấp 3.3V DC dòng thấp cho cảm biến HW-484 và OLED SSD1306. |
| **VIN** | `VCC` | `esp:VIN` | Nguồn 5V DC lấy từ cổng USB cấp cho cuộn hút cuộn dây Relay 5V. |
| **GND** | `GND` / `-` | `esp:GND.1` & `esp:GND.2` | Nối đất Mass chung cho toàn bộ mạch điều khiển và ngoại vi. |

---

## 5. Máy Trạng Thái & Logic Điều Khiển (FSM Logic)

```text
       [ Khởi động / Sẵn sàng ]
                   │
                   ▼
             [ STATE_IDLE ] ◄────────────────────────────────────────┐
                   │                                                 │
            (Tiếng vỗ tay #1)                                        │
                   │                                                 │
                   ▼                                                 │
          [ STATE_LISTENING ] ──(Hết Window 650ms)──► [ STATE_EXECUTING ]
              │       ▲                                      │
       (Tiếng vỗ #2, #3...)                                  │ (Đóng/Ngắt Relay)
              │       │                                      ▼
              └── Ghi nhận ──┘                       [ STATE_COOLDOWN (450ms) ]
                                                             │
                                                  (Khóa mic triệt tiêu tiếng
                                                   tách cơ khí của Relay)
```

| Số tiếng vỗ ($N$) | Hành động Relay | Phản hồi Còi Passive Buzzer | Hiển thị OLED |
|:---:|---|---|---|
| **$N = 1$** | Đảo trạng thái Relay 1 (`CH1: TOGGLE`) | 2 bíp xác nhận (1800Hz $\rightarrow$ 2400Hz) | `CH1 [LIGHT]: ON/OFF` |
| **$N = 2$** | Đảo trạng thái Relay 2 (`CH2: TOGGLE`) | 3 bíp nhịp cao (2000Hz $\rightarrow$ 2500Hz $\rightarrow$ 3000Hz) | `CH2 [ FAN ]: ON/OFF` |
| **$N \ge 3$** | Tắt toàn bộ cả 2 Relay (`ALL OFF`) | Âm trầm giảm dần (2400Hz $\rightarrow$ 1800Hz $\rightarrow$ 1100Hz) | `ALL DEVICES OFF` |

---

## 6. Hướng Dẫn Cân Chỉnh Độ Nhạy HW-484 Thực Tế (Trimpot Tuning Guide)

Module cảm biến âm thanh HW-484 có sẵn 1 chiết áp xoay vi chỉnh (Trimpot màu xanh dương) nối vào IC LM393:

1. **Chuẩn bị:** Cấp nguồn ESP32 và quan sát 2 LED trên module HW-484:
   - LED Đỏ (Power LED): Luôn sáng khi có nguồn 3.3V.
   - LED Xanh lá (Digital Out LED): Sáng khi chân `DO` bị kéo xuống `LOW` (có âm thanh vượt ngưỡng).
2. **Cân chỉnh ngưỡng tĩnh (Noise Floor Calibration):**
   - Giữ phòng ở trạng thái yên tĩnh bình thường.
   - Dùng tua-vít dẹp nhỏ xoay chiết áp theo chiều kim đồng hồ hoặc ngược chiều kim đồng hồ cho đến khi **LED Xanh lá vừa tắt hẳn**. Đây là điểm ngưỡng cân bằng tĩnh của căn phòng.
3. **Thử nghiệm tiếng vỗ tay:**
   - Đứng cách cảm biến khoảng $1.5\text{m} - 2\text{m}$ và vỗ tay dứt khoát 1 tiếng: LED Xanh lá trên module phải **nháy sáng tức thời rồi tắt ngay**.
   - Nếu LED Xanh lá không nháy: Xoay nhẹ chiết áp ngược chiều kim đồng hồ để tăng độ nhạy.
   - Nếu LED Xanh lá tự nháy liên tục do tiếng nói chuyện thông thường hoặc quạt gió: Xoay nhẹ chiết áp theo chiều kim đồng hồ để giảm độ nhạy.

---

## 7. Quy Trình Kiểm Chứng & Mô Phỏng (Verification Guide)

### 7.1 Kiểm tra Cú pháp & Biên dịch (CLI)

```bash
# 1. Kiểm tra cú pháp tệp diagram.json
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc-voi-02-acoustic-clap-switch/diagram.json", "utf8"))' && echo "JSON OK"

# 2. Biên dịch firmware cho môi trường board thật
pio run -d pocs/poc-voi-02-acoustic-clap-switch -e esp32dev

# 3. Biên dịch firmware cho môi trường mô phỏng Wokwi
pio run -d pocs/poc-voi-02-acoustic-clap-switch -e wokwi

# 4. Xác nhận artifact nhị phân được tạo thành công
test -f pocs/poc-voi-02-acoustic-clap-switch/.pio/build/esp32dev/firmware.bin && echo "Firmware BIN OK"
test -f pocs/poc-voi-02-acoustic-clap-switch/.pio/build/esp32dev/firmware.elf && echo "Firmware ELF OK"
```

### 7.2 Nạp Lên Phần Cứng Thật

```bash
# 1. Liệt kê cổng serial trên macOS
pio device list

# 2. Nạp code vào ESP32 DevKit V1
pio run -d pocs/poc-voi-02-acoustic-clap-switch -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# 3. Mở Serial Monitor để quan sát log (115200 baud)
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```
