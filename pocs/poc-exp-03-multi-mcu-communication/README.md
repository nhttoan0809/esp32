# POC EXP-03: Distributed Multi-MCU Communication Lab (Hệ Thống Phân Tán Đa Vi Điều Khiển)

Hệ thống điều khiển và thu thập dữ liệu phân tán chuyên nghiệp kết hợp 3 dòng vi điều khiển khác biệt (**ESP32 DevKit V1 30 chân**, **STM32F4 Black Pill ARM Cortex-M4**, và **Arduino Nano 5V ATmega328P**) thông qua các tuyến bus công nghiệp độc lập (**I2C Bus** và **UART2 Bus**), tích hợp **Web Dashboard thời gian thực qua WebSocket** và kênh giám sát gỡ lỗi song song qua **mạch nạp USB-UART HW-896**.

---

## 1. Tính Năng Kỹ Thuật Nổi Bật

- **Mô hình Mạng Nhúng Phân Tán Đẳng Cấp Ô Tô (Automotive ECU Architecture):**
  - **ESP32 (Master Gateway & Coordinator):** Quản trị mạng Wi-Fi, host Web Dashboard giao diện phẳng hiện đại, đồng bộ thời gian NTP, làm Master điều phối và tổng hợp dữ liệu đo đạc (Telemetry Aggregator).
  - **STM32F401/411 (Hard Real-Time Coprocessor):** Sử dụng nhân ARM Cortex-M4 100MHz có FPU phần cứng để điều khiển góc quay động cơ bước 28BYJ-48 (qua ULN2003) mượt mà không bị giật lag khi Master bận xử lý gói tin mạng, đồng thời xử lý các thuật toán số thực (DSP benchmark) tính toán thời gian thực thi tính bằng microsecond ($\mu\text{s}$).
  - **Arduino Nano (I/O Expander & Sub-Controller 5V):** Hoạt động ở mức điện áp chuẩn $5\text{V}$, tận dụng tối đa dải đo ADC 10-bit cho cảm biến dòng ACS712 và biến trở xoay $10\text{k}\Omega$, đóng/ngắt an toàn Module Relay 5V và phát âm thanh còi Passive Buzzer.
- **An Toàn Điện & Chống Quá Áp Tuyệt Đối (Rule 1 & Rule 9):**
  - Mạch phân áp chính xác ($1\text{k}\Omega / 2\text{k}\Omega$) hạ điện áp từ Nano TX ($5.0\text{V}$) về $3.33\text{V}$ bảo vệ an toàn chân RX2 của ESP32.
  - Điểm nối đất chung (**Common GND**) toàn hệ thống và cách ly nguồn động lực cho rơ-le, động cơ bước.
- **Giao Thức Truyền Thông Công Nghiệp Độc Lập:**
  - Tuyến I2C (Master ESP32 $\leftrightarrow$ Slave STM32 @ `0x42`): Mô hình thanh ghi chuẩn, hỗ trợ đọc/ghi vị trí động cơ bước và kích hoạt tính toán FPU.
  - Tuyến UART2 (Master ESP32 $\leftrightarrow$ Nano Sub-controller): Đóng gói khung tin chuẩn (Header `0xAA`, Payload, Checksum XOR, Footer `0x55`), tự động nhận diện ngắt kết nối và phục hồi nhịp tim Failsafe ($1500\text{ms}$).
- **Web Dashboard Thời Gian Thực (10 Hz WebSocket Streaming):**
  - Giám sát đồng thời cả 3 vi điều khiển trên 1 màn hình duy nhất mà không cần tải lại trang.
  - Đóng ngắt Relay, bấm còi Buzzer, xoay động cơ bước thuận/nghịch góc 45° và kích hoạt FPU benchmark tức thì.

---

## 2. Tính Toán & An Toàn Điện Phối Ghép (Electrical Engineering)

### 2.1 Cầu Phân Áp Tuyến Nano TX (5V) $\rightarrow$ ESP32 RX2 (3.3V)
Chân GPIO của ESP32 **không có khả năng chịu quá áp 5V**. Chân TX của Arduino Nano xuất mức logic $5.0\text{V}$.  
Mạch phân áp sử dụng 2 điện trở có sẵn trong kit ($R_1 = 1.0\text{k}\Omega, R_2 = 2.0\text{k}\Omega$):

$$V_{\text{ESP32\_RX2}} = V_{\text{Nano\_TX}} \times \frac{R_2}{R_1 + R_2} = 5.0\text{V} \times \frac{2000\Omega}{1000\Omega + 2000\Omega} = \mathbf{3.333\text{V}}$$

- **Dòng tiêu thụ trên phân áp:** $I = \frac{5.0\text{V}}{3000\Omega} \approx 1.67\text{mA} \ll 20\text{mA}$ (An toàn tuyệt đối cho chân ngõ ra ATmega328P).
- **Hằng số thời gian RC:** Trở kháng Thevenin $R_{th} \approx 667\Omega$, điện dung ký sinh $C_s \approx 50\text{pF}$.  
  $$\tau = R_{th} \times C_s \approx 667 \times 50 \times 10^{-12} \approx 33.3\text{ns} \ll T_{\text{bit}} (8.68\mu\text{s} \text{ ở } 115200\text{ baud})$$  
  Đảm bảo sườn xung vuông sắc nét, không bị trễ méo dạng tín hiệu nhị phân.

### 2.2 Tuyến ESP32 TX2 (3.3V) $\rightarrow$ Arduino Nano RX (5V)
- Ngưỡng nhận diện mức HIGH ngõ vào tối thiểu của ATmega328P: $V_{IH\_\min} = 0.6 \times 5.0\text{V} = 3.0\text{V}$.
- Mức điện áp ngõ ra của ESP32: $V_{OH} \approx 3.3\text{V} > 3.0\text{V}$ (Chân RX của Nano nhận diện tốt mức HIGH trực tiếp mà không cần mạch đệm kích áp).

---

## 3. Sơ Đồ Đấu Nối Toàn Hệ Thống (Wiring Diagrams)

### 3.1 Sơ Đồ Liên Kết Giữa 3 Bo Vi Điều Khiển (Inter-MCU Bus Wiring)

```text
       ┌───────────────────────────────┐
       │   ESP32 DevKit V1 (Master)    │
       │   Wi-Fi + Web Server + I2C/UART│
       └───┬───────────────────────┬───┘
           │ (I2C Bus: 3.3V Logic) │ (UART2 Bus: 3.3V Logic)
           │ SDA: GPIO 21          │ TX2: GPIO 17
           │ SCL: GPIO 22          │ RX2: GPIO 16
           │                       │
           ▼                       │       ┌──────────────────────┐
  ┌──────────────────┐             │       │ Cầu Phân Áp 1k/2k    │
  │ STM32F4 Coproc   │             │       │ Nano TX ──[1k]──┬──► GPIO 16 (RX2)
  │ PB7: I2C1_SDA    │             │       │                 │
  │ PB6: I2C1_SCL    │             │       │               [2k]
  │ PC13: LED Ready  │             │       │                 │
  └──────────────────┘             │       │                GND
                                   ▼       └──────────────────────┘
                           ┌──────────────────┐
                           │ Arduino Nano 5V  │
                           │ RX: Chân D0 ◄────┼── Nối trực tiếp GPIO 17 (TX2)
                           │ TX: Chân D1 ─────┘
                           └──────────────────┘

  *LƯU Ý BẮT BUỘC: Nối dây GND chung (Common Ground) giữa cả 3 vi điều khiển!
```

### 3.2 Sơ Đồ Ngoại Vi Từng Nút Mạng

```text
[STM32F4 Black Pill]
  ├── PA0 ──────► Driver ULN2003 IN1 ──► Động cơ bước 28BYJ-48 (Pha A)
  ├── PA1 ──────► Driver ULN2003 IN2 ──► Động cơ bước 28BYJ-48 (Pha B)
  ├── PA2 ──────► Driver ULN2003 IN3 ──► Động cơ bước 28BYJ-48 (Pha C)
  ├── PA3 ──────► Driver ULN2003 IN4 ──► Động cơ bước 28BYJ-48 (Pha D)
  └── PA9/PA10 ─► RXD/TXD Module USB-UART HW-896 (Theo dõi log PC 115200 baud)

[Arduino Nano 5V]
  ├── D7  ──────► Chân IN Module Relay 1 Kênh 5V (Active LOW)
  ├── D9  ──────► Cực dương (+) Còi Passive Buzzer
  ├── D2  ──────► Nút nhấn 1 (Chân còn lại nối GND)
  ├── D3  ──────► Nút nhấn 2 (Chân còn lại nối GND)
  ├── A0  ──────► Ngõ ra cảm biến dòng ACS712 (hoặc chân giữa Biến trở 10k)
  └── A1  ──────► Chân giữa Biến trở xoay 10k (hai chân bìa nối 5V và GND)

[ESP32 DevKit V1]
  ├── GPIO 25 ──► Trở 220Ω ──► LED Xanh Lục (I2C Activity) ──► GND
  ├── GPIO 26 ──► Trở 220Ω ──► LED Vàng (UART Activity)     ──► GND
  └── GPIO 27 ──► Trở 220Ω ──► LED Xanh Dương (System/Wi-Fi)──► GND
```

---

## 4. Bảng Ánh Xạ Chân (Hardware Pinout Map)

Bảng đối chiếu 4 cột bắt buộc theo quy chuẩn **Rule 7** của `AGENTS.md`:

### 4.1 Bo Mạch ESP32 DevKit V1 (30 chân) — Master Gateway
| Chân ESP32 DevKit V1 | Ký hiệu in Bo mạch / Module | Chân mô phỏng Wokwi (`diagram.json`) | Chức năng kỹ thuật & Ghi chú an toàn |
|---|:---:|:---:|---|
| **GPIO 21** | `SDA` | `esp:D21` ── `stm32:PB7` | Tuyến truyền dữ liệu I2C Master Data (kéo trở $4.7\text{k}\Omega$ lên 3.3V). |
| **GPIO 22** | `SCL` | `esp:D22` ── `stm32:PB6` | Tuyến xung nhịp I2C Master Clock (kéo trở $4.7\text{k}\Omega$ lên 3.3V). |
| **GPIO 17 (TX2)** | `TX2` | `esp:TX2` ── `nano:RX` | Tuyến xuất dữ liệu UART2 sang chân RX của Nano (mức 3.3V an toàn). |
| **GPIO 16 (RX2)** | `RX2` | `esp:RX2` ── `r_div:out` | Tuyến nhận dữ liệu UART2 từ Nano TX qua cầu phân áp $1\text{k}\Omega/2\text{k}\Omega$. |
| **GPIO 25** | Anode (+) | `esp:D25` ── `led_i2c:A` | LED báo trạng thái I2C Bus (xanh lục, nối tiếp điện trở $220\Omega$). |
| **GPIO 26** | Anode (+) | `esp:D26` ── `led_uart:A`| LED báo trạng thái UART Bus (vàng, nối tiếp điện trở $220\Omega$). |
| **GPIO 27** | Anode (+) | `esp:D27` ── `led_sys:A` | LED báo trạng thái Hệ thống / Wi-Fi (xanh dương, qua trở $220\Omega$). |
| **3V3** | `3V3` | `esp:3V3` | Nguồn 3.3V cấp cho mạch kéo I2C và các LED chỉ thị. |
| **GND** | `GND` | `esp:GND.1` | Nối mass chung toàn bộ 3 vi điều khiển và ngoại vi. |

### 4.2 Bo Mạch Arduino Nano — I/O Sub-Controller (5V)
| Chân Arduino Nano | Ký hiệu in Bo mạch / Module | Chân tương ứng ngoài | Chức năng kỹ thuật & Ghi chú an toàn |
|---|:---:|:---:|---|
| **D1 (TX)** | `TXD` | Đầu vào cầu phân áp $1\text{k}\Omega$ | Xuất dữ liệu UART 5V TTL $\rightarrow$ qua cầu phân áp hạ về GPIO 16 ESP32. |
| **D0 (RX)** | `RXD` | Nối thẳng chân GPIO 17 ESP32 | Nhận lệnh UART từ ESP32 (mức logic 3.3V nhận dạng tốt mức HIGH). |
| **D7** | `IN1` | Module Relay 1 Kênh 5V | Kích cuộn hút Relay (Active LOW: 0 = Bật relay, 1 = Tắt relay). |
| **D9** | `+` (PWM) | Passive Buzzer | Phát âm thanh phản hồi lệnh hoặc cảnh báo an toàn qua xung PWM. |
| **D2** | Nút 1 | Pushbutton (to GND) | Nút bấm ngõ vào số 1 (cấu hình `INPUT_PULLUP`). |
| **D3** | Nút 2 | Pushbutton (to GND) | Nút bấm ngõ vào số 2 (cấu hình `INPUT_PULLUP`). |
| **A0** | `OUT` | Cảm biến dòng ACS712 / Biến trở | Đọc điện áp tương tự 0–5V (ADC 10-bit độ phân giải 4.88mV/LSB). |
| **A1** | Wiper | Biến trở xoay $10\text{k}\Omega$ | Kênh analog phụ trợ đo điện áp thử nghiệm 0–5V. |
| **D13** | Built-in LED | LED trên bo Nano | Đèn chớp báo nhịp tim Heartbeat của Sub-Controller. |
| **5V & GND** | `5V` & `GND` | Nguồn 5V & Mass chung | Cấp nguồn nuôi cho Nano và module relay. |

### 4.3 Bo Mạch STM32F4 Black Pill — Coprocessor (3.3V)
| Chân STM32F4 Black Pill | Ký hiệu in Bo mạch | Chân tương ứng ngoài | Chức năng kỹ thuật & Ghi chú an toàn |
|---|:---:|:---:|---|
| **PB7** | `PB7` (I2C1_SDA) | Nối chân GPIO 21 ESP32 | Kênh dữ liệu I2C Slave (địa chỉ `0x42`, mức logic 3.3V). |
| **PB6** | `PB6` (I2C1_SCL) | Nối chân GPIO 22 ESP32 | Kênh xung nhịp I2C Slave Clock (mức logic 3.3V). |
| **PA0** | `IN1` | Driver ULN2003 (Pha A) | Tín hiệu kích pha A động cơ bước 28BYJ-48. |
| **PA1** | `IN2` | Driver ULN2003 (Pha B) | Tín hiệu kích pha B động cơ bước 28BYJ-48. |
| **PA2** | `IN3` | Driver ULN2003 (Pha C) | Tín hiệu kích pha C động cơ bước 28BYJ-48. |
| **PA3** | `IN4` | Driver ULN2003 (Pha D) | Tín hiệu kích pha D động cơ bước 28BYJ-48. |
| **PC13** | Built-in LED | LED trên bo STM32 | Báo trạng thái Coprocessor sẵn sàng (Active LOW). |
| **PA9** | `TX1` (USART1) | RXD Module USB-UART HW-896 | Kênh Serial xuất log gỡ lỗi FPU ra PC độc lập (115200 baud). |
| **PA10**| `RX1` (USART1) | TXD Module USB-UART HW-896 | Kênh nhận lệnh debug phụ trợ từ PC. |
| **3V3 & GND** | `3V3` & `GND` | Nguồn 3.3V & Mass chung | Nguồn cấp và mass chung toàn hệ thống. |

---

## 5. Danh Mục Lệnh Thao Tác Dòng Lệnh (CLI-First)

### 5.1 Biên Dịch Toàn Bộ Firmware (PlatformIO Build)
```bash
# 1. Biên dịch Node ESP32 Master Gateway
pio run -d pocs/poc-exp-03-multi-mcu-communication -e esp32_master

# 2. Biên dịch Node STM32F4 Coprocessor
pio run -d pocs/poc-exp-03-multi-mcu-communication -e stm32_coprocessor

# 3. Biên dịch Node Arduino Nano Sub-Controller
pio run -d pocs/poc-exp-03-multi-mcu-communication -e nano_subcontroller

# 4. Kiểm tra sự tồn tại của file nhị phân sau khi build thành công
test -f pocs/poc-exp-03-multi-mcu-communication/.pio/build/esp32_master/firmware.bin && echo "ESP32 BIN OK"
test -f pocs/poc-exp-03-multi-mcu-communication/.pio/build/stm32_coprocessor/firmware.bin && echo "STM32 BIN OK"
test -f pocs/poc-exp-03-multi-mcu-communication/.pio/build/nano_subcontroller/firmware.hex && echo "Nano HEX OK"
```

### 5.2 Nạp Code & Theo Dõi Serial Trên Board Thật (Flash & Monitor)
```bash
# Xem danh sách cổng nạp kết nối vào macOS
pio device list

# A. Nạp code cho ESP32 DevKit V1:
pio run -d pocs/poc-exp-03-multi-mcu-communication -e esp32_master -t upload --upload-port /dev/cu.usbserial-XXXX
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200

# B. Nạp code cho Arduino Nano:
pio run -d pocs/poc-exp-03-multi-mcu-communication -e nano_subcontroller -t upload --upload-port /dev/cu.wchusbserialXXXX
pio device monitor -p /dev/cu.wchusbserialXXXX -b 115200

# C. Nạp code cho STM32F4 Black Pill (qua ST-Link hoặc DFU USB):
pio run -d pocs/poc-exp-03-multi-mcu-communication -e stm32_coprocessor -t upload

# D. Theo dõi log debug của STM32 qua mạch nạp USB-UART HW-896:
pio device monitor -p /dev/cu.usbserial-HW896 -b 115200
```

### 5.3 Mô Phỏng & Kiểm Tra Tự Động (Wokwi CLI)
```bash
# Kiểm tra cú pháp sơ đồ mạch diagram.json
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc-exp-03-multi-mcu-communication/diagram.json", "utf8"))'
wokwi-cli lint pocs/poc-exp-03-multi-mcu-communication

# Chạy mô phỏng kiểm thử tự động với Wokwi CLI
wokwi-cli --expect-text "ESP32 Master Gateway Running!" --timeout 15000 pocs/poc-exp-03-multi-mcu-communication
```

---

## 6. Hướng Dẫn Truy Cập Web Dashboard Điều Khiển

1. Cấp nguồn cho toàn bộ 3 bo vi điều khiển.
2. Dùng điện thoại hoặc máy tính kết nối vào mạng Wi-Fi do ESP32 phát:
   - **Tên Wi-Fi (SSID):** `ESP32-EXP03-MASTER`
   - **Mật khẩu (Password):** `12345678`
3. Mở trình duyệt web truy cập địa chỉ IP: **`http://192.168.4.1`**
4. Giao diện Web Dashboard hiển thị các tính năng:
   - **Node 1 (ESP32):** Trạng thái Wi-Fi, Uptime, Free Heap, số lượng lỗi gói tin UART2.
   - **Node 2 (STM32F4):** Vị trí bước hiện tại, các nút bấm điều khiển quay động cơ bước 28BYJ-48 và nút kích hoạt kiểm chuẩn FPU Benchmark.
   - **Node 3 (Arduino Nano):** Đồ thị đo dòng điện ACS712, thước đo điện áp biến trở 10k, công tắc BẬT/TẮT Relay và nút nhấn còi Buzzer.
