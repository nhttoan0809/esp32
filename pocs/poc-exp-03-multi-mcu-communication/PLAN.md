# POC EXP-03: Kế Hoạch Triển Khai Hệ Thống Phân Tán Đa Vi Điều Khiển (Distributed Multi-MCU Communication Lab)

> **Mã Đề Tài:** EXP-03  
> **Git Worktree & Branch:** `wt/poc/exp-03-multi-mcu-communication`  
> **Kiến trúc Mạng:** Phân cấp Master-Slave / Coprocessor (ESP32 Master + STM32F4 Coprocessor + Arduino Nano Sub-Controller)  
> **Chuẩn Quy Định:** Tuân thủ 100% quy chuẩn [AGENTS.md](../../AGENTS.md) và tài liệu phần cứng [PROJECT-CATALOG-AND-SPECS.md](../../docs/projects/PROJECT-CATALOG-AND-SPECS.md)

---

## 1. Tổng Quan Yêu Cầu & Bối Cảnh Kỹ Thuật (Requirement & Context)

### 1.1 Mục Tiêu Đề Tài
Xây dựng mô hình trạm điều khiển nhúng phân tán đa vi điều khiển tương tự mạng ECU trên xe hơi ô tô hoặc dây chuyền tự động hóa công nghiệp. Hệ thống phân chia nhiệm vụ chuyên biệt cho 3 nền tảng vi điều khiển khác nhau:
1. **ESP32 DevKit V1 (30 chân) — Master Gateway & Coordinator:**
   - Quản trị kết nối mạng Wi-Fi, host Web Dashboard theo thời gian thực (HTTP Web Server + WebSocket streaming).
   - Đồng bộ thời gian thực chuẩn quốc tế qua NTP Server.
   - Làm I2C Bus Master điều phối STM32F4 Coprocessor và làm UART2 Master giao tiếp với Arduino Nano.
   - Tổng hợp dữ liệu đo đạc (telemetry aggregation) và phân phối lệnh điều khiển an toàn.
2. **STM32F4 Black Pill (ARM Cortex-M4 @ 84MHz/100MHz + FPU) — Coprocessor (Math & Motion):**
   - Đóng vai trò thiết bị tớ (I2C Slave/Device) trên địa chỉ `0x42`.
   - Đảm nhiệm tác vụ thời gian thực cứng (hard real-time): Điều khiển động cơ bước 28BYJ-48 (qua driver ULN2003) hoặc Servo SG90 quay mịn, gia tốc mượt mà không bị nghẽn bởi tác vụ mạng của ESP32.
   - Đảm nhiệm xử lý toán học hiệu năng cao: Chạy giải thuật số thực (FPU DSP benchmark, bộ lọc FIR/IIR) và phản hồi thời gian thực thi (execution time tính bằng microsecond $\mu\text{s}$) về Master.
3. **Arduino Nano (ATmega328P, 5V) — I/O Expander & Sub-Controller:**
   - Hoạt động ở mức điện áp chuẩn $5\text{V}$, xử lý các ngoại vi và cảm biến 5V bản địa.
   - Đọc cảm biến dòng điện ACS712 hoặc biến trở $10\text{k}\Omega$ trên chân ADC 10-bit ($0 - 5\text{V}$) mà không làm suy hao dải đo.
   - Điều khiển đóng/ngắt an toàn Module Relay 5V (SRD-05VDC-SL-C) và phát âm thanh còi Buzzer.
   - Giao tiếp với ESP32 Serial2 qua giao thức UART có đóng gói khung tin (Framing + CRC/Checksum).
4. **Mạch Chuyển Đổi USB-UART HW-896 V1.2 — Kênh Giám Sát & Debug Độc Lập:**
   - Dùng làm kênh Serial Monitor thứ hai trên máy tính nối vào STM32 (USART1) hoặc Nano (SoftwareSerial/HardwareSerial) để theo dõi luồng dữ liệu song song mà không can thiệp vào các đường bus liên kết chính.

---

## 2. Tính Toán & An Toàn Điện Phối Ghép (Electrical Safety & Level Matching)

### 2.1 Cầu Phân Áp Điện Trở Cho Đường Truyền Nano TX (5V) $\rightarrow$ ESP32 RX2 (3.3V)
Chân GPIO của ESP32 tuyệt đối không chịu được điện áp $5\text{V}$. Chân TX của Arduino Nano xuất mức logic HIGH là $5.0\text{V}$.  
Bắt buộc bố trí mạch phân áp dùng điện trở chuẩn có sẵn trong kit ($R_1 = 1\text{k}\Omega, R_2 = 2\text{k}\Omega$):

$$V_{\text{ESP32\_RX2}} = V_{\text{Nano\_TX}} \times \frac{R_2}{R_1 + R_2} = 5.0\text{V} \times \frac{2\text{k}\Omega}{1\text{k}\Omega + 2\text{k}\Omega} = \mathbf{3.33\text{V}}$$

- **Dòng tiêu thụ trên cầu phân áp:**  
  $$I_{\text{divider}} = \frac{5\text{V}}{3\text{k}\Omega} \approx 1.67\text{mA} \ll 20\text{mA} \quad \text{(An toàn tuyệt đối cho chân TX ATmega328P)}$$
- **Hằng số thời gian RC & Băng thông tín hiệu:**  
  Điện dung ký sinh dây dẫn $C_s \approx 50\text{pF}$, trở kháng Thevenin $R_{th} = R_1 \parallel R_2 \approx 667\Omega$.  
  Thời gian trễ: $\tau = R_{th} \times C_s \approx 33.3\text{ns}$.  
  Ở tốc độ $115200\text{ bps}$, độ rộng 1 bit là $8.68\mu\text{s} > 260 \times \tau$, đảm bảo xung sườn vuông vắn, không suy hao méo dạng tín hiệu.

### 2.2 Chiều ESP32 TX2 (3.3V) $\rightarrow$ Arduino Nano RX (5V)
- Điện áp ngõ ra mức HIGH của ESP32: $V_{OH} \approx 3.3\text{V}$.
- Ngưỡng điện áp nhận biết mức HIGH ngõ vào của ATmega328P ($V_{CC} = 5\text{V}$):
  $$V_{IH\_\min} = 0.6 \times V_{CC} = 0.6 \times 5.0\text{V} = \mathbf{3.0\text{V}}$$
- Do $V_{OH} (3.3\text{V}) > V_{IH\_\min} (3.0\text{V})$, Arduino Nano nhận dạng chính xác mức logic HIGH từ ESP32 một cách an toàn mà không cần thêm linh kiện đệm nâng áp.

### 2.3 Phối Ghép Bus I2C Giữa ESP32 & STM32F4 Black Pill
- Cả ESP32 và STM32F4 Black Pill đều chạy ở điện áp logic chuẩn $3.3\text{V}$.
- Các chân PB6 (SCL) và PB7 (SDA) của STM32F401/411 đều là chân chuẩn **5V-Tolerant (FT)**.
- Bus I2C là cấu trúc ngõ ra cực thu hở (Open-Drain), kích hoạt điện trở kéo lên nguồn 3.3V ($4.7\text{k}\Omega$ hoặc trở kéo nội bộ). Tốc độ xung nhịp chuẩn: **$100\text{kHz}$** (Standard Mode) hoặc **$400\text{kHz}$** (Fast Mode).

### 2.4 Nguyên Tắc Nối Đất Chung (Common Ground) & Cách Ly Nguồn Động Lực
- Toàn bộ 3 bo vi điều khiển (ESP32, STM32, Nano), cảm biến và module relay **bắt buộc nối chung một đường mass (Common GND)**.
- Cuộn hút Relay 5V và Động cơ bước 28BYJ-48 ăn dòng đột biến khi khởi động: Cấp nguồn 5V ngoài hoặc lấy từ cổng USB 5V (chân VIN), tuyệt đối không cấp từ chân 3V3 của ESP32 hay STM32.

---

## 3. Kiến Trúc Mạng & Sơ Đồ Khối Hệ Thống (Architecture Topology)

```mermaid
flowchart TD
    subgraph Browser ["Web Client / PC"]
        UI["Web Dashboard (HTML5 / WebSocket)"]
        Terminal["Serial Monitor (HW-896 USB-UART)"]
    end

    subgraph ESP32_Node ["ESP32 DevKit V1 (Master Gateway - 3.3V)"]
        WS["WebSocket Server & HTTP Server"]
        Coord["System State Coordinator"]
        I2C_M["I2C Master Engine (Wire)"]
        UART_M["UART2 Engine (Serial2)"]
    end

    subgraph STM32_Node ["STM32F4 Black Pill (Coprocessor - 3.3V)"]
        I2C_S["I2C Slave Engine (Addr: 0x42)"]
        Stepper_Ctrl["28BYJ-48 Stepper Driver (ULN2003)"]
        DSP_Bench["ARM Cortex-M4 FPU Math Benchmark"]
    end

    subgraph Nano_Node ["Arduino Nano (I/O Sub-Controller - 5V)"]
        UART_S["UART Parser & Frame Validator"]
        Sensors["5V Sensors (ACS712 Current / 10k Pot)"]
        Actuators["5V Actuators (Relay Module & Buzzer)"]
    end

    UI <== "Wi-Fi (HTTP / WebSocket 80)" ==> WS
    WS <--> Coord
    Coord <--> I2C_M
    Coord <--> UART_M

    I2C_M <== "I2C Bus (3.3V: SDA=GPIO21, SCL=GPIO22)" ==> I2C_S
    I2C_S --> Stepper_Ctrl
    I2C_S --> DSP_Bench

    UART_M <== "ESP32 TX2 (GPIO17) -> Direct 3.3V" ==> UART_S
    UART_S <== "Nano TX (5V) -> Cầu Phân Áp 1k/2k -> ESP32 RX2 (GPIO16)" ==> UART_M

    UART_S --> Sensors
    UART_S --> Actuators

    Terminal -.- "Debug Channel (HW-896 TX/RX)" -.- STM32_Node
    Terminal -.- "Debug Channel (HW-896 TX/RX)" -.- Nano_Node
```

---

## 4. Đặc Tả Giao Thức Truyền Thông (Communication Protocol Specification)

### 4.1 Giao Thức I2C (ESP32 Master $\leftrightarrow$ STM32F4 Slave 0x42)
Giao tiếp theo mô hình thanh ghi (Register-Based Protocol):

| Mã Thanh Ghi | Tên Thanh Ghi | Chế Độ | Kích Thước | Mô Tả Chức Năng |
|:---:|:---|:---:|:---:|:---|
| `0x00` | `REG_WHO_AM_I` | Read | 1 Byte | Trả về mã nhận diện `0x42` để kiểm tra kết nối. |
| `0x01` | `REG_SYS_STATUS` | Read | 1 Byte | Bit 0: Ready, Bit 1: Stepper Moving, Bit 2: Math Busy, Bit 3: Error. |
| `0x10` | `REG_STEPPER_CMD` | Write | 4 Bytes | Byte 0: Hướng quay (0: Thuận, 1: Nghịch); Byte 1: Tốc độ (RPM); Byte 2-3: Số bước (`uint16_t`). |
| `0x14` | `REG_STEPPER_STOP`| Write | 1 Byte | `0x01`: Dừng khẩn cấp động cơ bước ngay lập tức. |
| `0x15` | `REG_STEPPER_POS` | Read | 4 Bytes | Trả về vị trí bước hiện tại (`int32_t`). |
| `0x20` | `REG_DSP_START`   | Write | 2 Bytes | Byte 0: Mã thuật toán (1: Float Matrix, 2: FIR Filter); Byte 1: Số vòng lặp. |
| `0x24` | `REG_DSP_RESULT`  | Read | 6 Bytes | 4 Bytes: Thời gian thực thi ($\mu\text{s}$); 2 Bytes: Checksum kết quả kiểm tra tính đúng đắn. |

### 4.2 Giao Thức UART (ESP32 Master $\leftrightarrow$ Arduino Nano Sub-Controller)
Giao tiếp theo gói tin nhị phân có khung đóng gói định dạng chuẩn công nghiệp (Packet Framing):

```text
┌────────────┬─────────────┬───────────┬──────────────┬─────────────┬───────────┬──────────┐
│ START_BYTE │ PACKET_TYPE │  SEQ_NUM  │ PAYLOAD_LEN  │   PAYLOAD   │ CHECKSUM  │ END_BYTE │
│   (0xAA)   │   (1 Byte)  │ (1 Byte)  │ (1 Byte: N)  │  (N Bytes)  │ (1 Byte)  │  (0x55)  │
└────────────┴─────────────┴───────────┴──────────────┴─────────────┴───────────┴──────────┘
```

- **START_BYTE:** `0xAA` (Đồng bộ khung tin).
- **PACKET_TYPE:**
  - `0x01`: `CMD_SET_RELAY` (Master $\rightarrow$ Nano: Payload = `[relay_idx, state 0/1]`).
  - `0x02`: `CMD_TRIGGER_BUZZER` (Master $\rightarrow$ Nano: Payload = `[freq_H, freq_L, duration_ms]`).
  - `0x05`: `CMD_PING` (Master $\rightarrow$ Nano: Kiểm tra nhịp tim Heartbeat).
  - `0x80`: `RESP_ACK` (Nano $\rightarrow$ Master: Xác nhận thực thi lệnh kèm SEQ_NUM).
  - `0x81`: `RESP_NACK` (Nano $\rightarrow$ Master: Báo lỗi lệnh hoặc sai Checksum).
  - `0x10`: `TELEMETRY_DATA` (Nano $\rightarrow$ Master: Gửi định kỳ 200ms–500ms):
    - Payload 8 Bytes: `[ACS712_H, ACS712_L, POT_H, POT_L, RELAY_BITS, BUTTON_BITS, VCC_5V_H, VCC_5V_L]`.
- **CHECKSUM:** Thuật toán XOR cộng dồn toàn bộ các byte từ `PACKET_TYPE` đến hết `PAYLOAD`:
  $$\text{Checksum} = \text{PACKET\_TYPE} \oplus \text{SEQ\_NUM} \oplus \text{PAYLOAD\_LEN} \oplus \bigoplus_{i=0}^{N-1} \text{PAYLOAD}[i]$$
- **END_BYTE:** `0x55` (Chốt đuôi khung tin).
- **Cơ chế Timeout & Failsafe:** Nếu ESP32 không nhận được bản tin telemetry hoặc phản hồi PING trong quá $1500\text{ms}$, Master chuyển trạng thái Nano sang `NODE_OFFLINE` và hiển thị cảnh báo đỏ trên Dashboard.

---

## 5. Bảng Phân Bổ Chân Toàn Hệ Thống (Hardware Pinout Map)

Bảng đối chiếu 4 cột bắt buộc theo quy chuẩn **Rule 7** của `AGENTS.md`:

### 5.1 Bo Mạch ESP32 DevKit V1 (30 chân) — Master
| Chân ESP32 DevKit V1 | Ký hiệu in Bo mạch / Module | Chân mô phỏng Wokwi (`diagram.json`) | Chức năng kỹ thuật & Ghi chú an toàn |
|---|:---:|:---:|---|
| **GPIO 21** | `SDA` | `esp:21` ── `stm32:PB7` | Tuyến truyền dữ liệu I2C Master Data (kéo trở $4.7\text{k}\Omega$ lên 3.3V). |
| **GPIO 22** | `SCL` | `esp:22` ── `stm32:PB6` | Tuyến xung nhịp I2C Master Clock (kéo trở $4.7\text{k}\Omega$ lên 3.3V). |
| **GPIO 17 (TX2)** | `TX2` | `esp:17` ── `nano:RX` | Tuyến truyền dữ liệu UART2 từ ESP32 sang Nano RX (mức 3.3V an toàn). |
| **GPIO 16 (RX2)** | `RX2` | `esp:16` ── `r_div:out` | Tuyến nhận dữ liệu UART2 từ Nano TX qua cầu phân áp $1\text{k}\Omega/2\text{k}\Omega$ về 3.3V. |
| **GPIO 25** | Anode (+) | `led_i2c:A` | LED báo trạng thái hoạt động bus I2C (xanh lục, qua trở $220\Omega$). |
| **GPIO 26** | Anode (+) | `led_uart:A` | LED báo trạng thái hoạt động bus UART (vàng, qua trở $220\Omega$). |
| **GPIO 27** | Anode (+) | `led_sys:A` | LED báo trạng thái hệ thống / Wi-Fi (xanh dương, qua trở $220\Omega$). |
| **3V3** | `3V3` | `esp:3V3` | Nguồn 3.3V cấp cho mạch kéo I2C và logic. |
| **GND** | `GND` | `esp:GND` | Nối mass chung toàn bộ 3 vi điều khiển và ngoại vi. |

### 5.2 Bo Mạch Arduino Nano — I/O Sub-Controller (5V)
| Chân Arduino Nano | Ký hiệu in Bo mạch / Module | Chân tương ứng ngoài | Chức năng kỹ thuật & Ghi chú an toàn |
|---|:---:|:---:|---|
| **D1 (TX)** | `TXD` | Đầu vào cầu phân áp $1\text{k}\Omega$ | Xuất dữ liệu UART 5V TTL $\rightarrow$ qua phân áp về GPIO 16 ESP32. |
| **D0 (RX)** | `RXD` | Nối thẳng chân GPIO 17 ESP32 | Nhận lệnh UART từ ESP32 (mức 3.3V nhận dạng tốt mức HIGH). |
| **D7** | `IN1` | Module Relay 1 Kênh 5V | Kích cuộn hút Relay (Active LOW: 0 = Đóng rơ-le, 1 = Ngắt). |
| **D9** | `+` (PWM) | Passive Buzzer | Phát âm thanh bíp xác nhận lệnh hoặc cảnh báo sự cố. |
| **D2** | Nút 1 | Pushbutton (to GND) | Nút bấm ngõ vào số 1 (cấu hình `INPUT_PULLUP`). |
| **D3** | Nút 2 | Pushbutton (to GND) | Nút bấm ngõ vào số 2 (cấu hình `INPUT_PULLUP`). |
| **A0** | `OUT` | Cảm biến dòng ACS712 / Biến trở | Đọc điện áp tương tự 0–5V (ADC 10-bit độ phân giải 4.88mV/LSB). |
| **D13** | Built-in LED | LED trên bo Nano | Đèn chớp báo nhịp tim Heartbeat của Sub-Controller. |
| **5V & GND** | `5V` & `GND` | Nguồn 5V & Mass chung | Cấp nguồn nuôi cho Nano và module relay. |

### 5.3 Bo Mạch STM32F4 Black Pill — Coprocessor (3.3V)
| Chân STM32F4 Black Pill | Ký hiệu in Bo mạch | Chân tương ứng ngoài | Chức năng kỹ thuật & Ghi chú an toàn |
|---|:---:|:---:|---|
| **PB7** | `PB7` (I2C1_SDA) | Nối chân GPIO 21 ESP32 | Kênh dữ liệu I2C Slave (địa chỉ `0x42`, mức 3.3V). |
| **PB6** | `PB6` (I2C1_SCL) | Nối chân GPIO 22 ESP32 | Kênh xung nhịp I2C Slave Clock (mức 3.3V). |
| **PA0** | `IN1` | Driver ULN2003 (Pha A) | Tín hiệu kích pha A động cơ bước 28BYJ-48. |
| **PA1** | `IN2` | Driver ULN2003 (Pha B) | Tín hiệu kích pha B động cơ bước 28BYJ-48. |
| **PA2** | `IN3` | Driver ULN2003 (Pha C) | Tín hiệu kích pha C động cơ bước 28BYJ-48. |
| **PA3** | `IN4` | Driver ULN2003 (Pha D) | Tín hiệu kích pha D động cơ bước 28BYJ-48. |
| **PC13** | Built-in LED | LED trên bo STM32 | Báo trạng thái Coprocessor sẵn sàng (Active LOW). |
| **PA9 / PA10** | `TX1 / RX1` | Module USB-UART HW-896 | Kênh Serial USART1 xuất log gỡ lỗi độc lập ra PC (115200 baud). |
| **3V3 & GND** | `3V3` & `GND` | Nguồn 3.3V & Mass chung | Nguồn cấp và mass chung toàn hệ thống. |

---

## 6. Kiến Trúc Cấu Trúc Mã Nguồn & PlatformIO (Project Structure)

Dự án được cấu trúc hợp nhất và quản lý trong thư mục `pocs/poc-exp-03-multi-mcu-communication` với file cấu hình đa môi trường `platformio.ini` cho phép biên dịch độc lập từng bo vi điều khiển bằng CLI:

```text
pocs/poc-exp-03-multi-mcu-communication/
├── platformio.ini              # Cấu hình đa môi trường: esp32_master, stm32_coprocessor, nano_subcontroller, wokwi
├── wokwi.toml                  # Cấu hình mô phỏng Wokwi CLI cho ESP32 Master
├── diagram.json                # Sơ đồ mạch trực quan có nhãn (Rule 6 & Rule 7)
├── README.md                   # Tài liệu hướng dẫn sử dụng, đấu dây, CLI và API
├── include/
│   ├── protocol_defs.h         # Định nghĩa cấu trúc khung tin UART, mã lệnh, thanh ghi I2C dùng chung
│   └── web_dashboard.h         # Mã nguồn HTML/CSS/JS nhúng của giao diện Web điều khiển thời gian thực
└── src/
    ├── master/                 # Mã nguồn ESP32 DevKit V1 (Master Gateway)
    │   ├── main_master.cpp     # Vòng lặp chính, quản lý Wi-Fi, WebServer, WebSocket
    │   ├── i2c_master_mgr.cpp  # Quản lý giao tiếp I2C với STM32 Coprocessor
    │   ├── uart_nano_mgr.cpp   # Quản lý truyền nhận, giải mã gói tin UART với Nano
    │   └── ntp_time_mgr.cpp    # Đồng bộ thời gian thực NTP
    ├── coprocessor/            # Mã nguồn STM32F4 Black Pill (Coprocessor)
    │   ├── main_stm32.cpp      # Xử lý ngắt I2C Slave (Wire.onReceive / onRequest)
    │   ├── stepper_driver.cpp  # Điều khiển động cơ bước 28BYJ-48 tăng tốc/giảm tốc mượt
    │   └── dsp_benchmark.cpp   # Thuật toán số thực FPU tính toán thời gian thực thi (us)
    └── subcontroller/          # Mã nguồn Arduino Nano (I/O Sub-Controller)
        ├── main_nano.cpp       # Vòng lặp 16MHz, đọc cảm biến 5V, xuất xung relay
        ├── packet_handler.cpp  # Tạo và phân tích gói tin UART nhị phân kèm XOR Checksum
        └── relay_buzzer.cpp    # Quản lý đóng cắt relay an toàn và chuông bíp
```

---

## 7. Kế Hoạch Triển Khai Chi Tiết Từng Bước (Vertical Slice Tasks)

Kế hoạch được chia thành 6 giai đoạn với các tác vụ độc lập, có thể kiểm chứng riêng biệt:

### Giai Đoạn 1: Định Nghĩa Giao Thức Chung & Cấu Hình Build PlatformIO
- [x] **Task 1.1: Tạo khung dự án & `include/protocol_defs.h`**
  - Định nghĩa opcode, struct khung tin UART (`UartPacketHeader`, `TelemetryPayload`, `RelayCommandPayload`).
  - Định nghĩa danh mục thanh ghi I2C (`I2C_REG_*`) và bitmask trạng thái.
  - Viết hàm tính và kiểm tra XOR Checksum dùng chung.
  - *Kiểm chứng:* File header biên dịch tương thích trên cả kiến trúc Xtensa (ESP32), ARM Cortex-M4 (STM32) và AVR (Nano).
- [x] **Task 1.2: Thiết lập file `platformio.ini` đa môi trường**
  - Khai báo 4 môi trường: `[env:esp32_master]`, `[env:stm32_coprocessor]`, `[env:nano_subcontroller]`, và `[env:wokwi]`.
  - Cấu hình `build_src_filter` tương ứng cho từng môi trường.
  - *Kiểm chứng:* Chạy lệnh `pio run -d pocs/poc-exp-03-multi-mcu-communication -e esp32_master` biên dịch sạch 0 cảnh báo.

### Giai Đoạn 2: Xây Dựng Firmware Arduino Nano (I/O Sub-Controller 5V)
- [x] **Task 2.1: Triển khai Engine phân tích gói tin UART (`src/subcontroller/`)**
  - Máy trạng thái nhận byte bất đồng bộ (FSM: `WAIT_START` $\rightarrow$ `READ_HEADER` $\rightarrow$ `READ_PAYLOAD` $\rightarrow$ `VERIFY_CHECKSUM` $\rightarrow$ `DISPATCH`).
  - Xử lý các lệnh `CMD_SET_RELAY`, `CMD_TRIGGER_BUZZER`, `CMD_PING`.
- [x] **Task 2.2: Đọc cảm biến 5V & Đóng gói Telemetry định kỳ**
  - Đọc kênh ADC A0 (cảm biến dòng ACS712 / biến trở), đọc nút nhấn D2/D3 có chống rung.
  - Gửi gói tin `TELEMETRY_DATA` mỗi $250\text{ms}$ qua UART TX.
  - Nhấp nháy LED D13 nhịp $1\text{Hz}$ báo hiệu sống.
  - *Kiểm chứng:* Biên dịch `pio run -d pocs/poc-exp-03-multi-mcu-communication -e nano_subcontroller` thành công (Flash 14.9%, RAM 12.3%).

### Giai Đoạn 3: Xây Dựng Firmware STM32F4 Black Pill (Coprocessor)
- [x] **Task 3.1: Triển khai I2C Slave Engine (`src/coprocessor/`)**
  - Cấu hình `Wire.begin(0x42)` trên chân PB6/PB7.
  - Xử lý `onReceive` nhận lệnh ghi thanh ghi và `onRequest` xuất dữ liệu phản hồi theo thanh ghi đã trỏ.
- [x] **Task 3.2: Module Điều Khiển Động Cơ Bước 28BYJ-48 & Module FPU Benchmark**
  - Điều khiển 4 chân PA0–PA3 bước nửa bước (Half-stepping 8 chu kỳ) cho động cơ 28BYJ-48 quay êm.
  - Triển khai hàm đo điểm chuẩn toán học FPU (nhân ma trận hoặc tính tích phân số thực) và tính toán thời gian thực thi bằng bộ đếm vi giây `micros()`.
  - *Kiểm chứng:* Biên dịch `pio run -d pocs/poc-exp-03-multi-mcu-communication -e stm32_coprocessor` thành công (Flash 8.9%, RAM 2.4%).

### Giai Đoạn 4: Xây Dựng Firmware ESP32 Master Gateway
- [x] **Task 4.1: Master I2C Polling & UART2 Comm Manager (`src/master/`)**
  - Khởi tạo Hardware Serial2 trên chân TX2 (GPIO 17) và RX2 (GPIO 16) baud rate $115200$.
  - Thu nhận và giải mã telemetry từ Arduino Nano, giám sát Heartbeat ngắt kết nối.
  - Khởi tạo I2C Master trên GPIO 21/22, định kỳ $100\text{ms}$ quét trạng thái STM32F4 Coprocessor.
- [x] **Task 4.2: Web Dashboard & WebSocket Telemetry Server**
  - Nhúng Web Dashboard giao diện phẳng (HTML/CSS/JS) hỗ trợ Responsive trên Mobile & Desktop.
  - Kênh WebSocket đẩy trực tiếp thông số 3 bo mạch với tần số $10\text{Hz}$:
    - Thẻ Node 1 (ESP32): IP Wi-Fi, RAM trống, Giờ NTP, Trạng thái bus.
    - Thẻ Node 2 (STM32): Trạng thái động cơ bước, vị trí hiện tại, thời gian tính toán FPU ($\mu\text{s}$).
    - Thẻ Node 3 (Nano): Giá trị cảm biến dòng điện ACS712, điện áp biến trở, nút gạt điều khiển Relay, còi Buzzer.
  - *Kiểm chứng:* Biên dịch `pio run -d pocs/poc-exp-03-multi-mcu-communication -e esp32_master` thành công (Flash 61.0%, RAM 14.3%).

### Giai Đoạn 5: Thiết Kế Sơ Đồ Wokwi & Mô Phỏng Tự Động
- [x] **Task 5.1: Xây dựng `diagram.json` chuẩn Rule 6 & Rule 7**
  - Định nghĩa board ESP32, linh kiện LED chỉ thị trạng thái, các khối cầu nối I2C và UART.
  - Gán đầy đủ thuộc tính `"label"` cho 100% linh kiện trên sơ đồ.
- [x] **Task 5.2: Kiểm tra cú pháp & Lint Wokwi**
  - Kiểm tra cú pháp JSON bằng Node.js CLI.
  - Thiết lập chế độ mô phỏng `[env:wokwi]` hỗ trợ loopback/mock để kiểm thử tự động trên Wokwi CLI.
  - *Kiểm chứng:* Chạy `wokwi-cli lint` đạt 0 lỗi, 0 cảnh báo.

### Giai Đoạn 6: Tài Liệu Hóa & Kiểm Thử Tổng Thể
- [x] **Task 6.1: Viết tài liệu `README.md` hoàn chỉnh**
  - Đầy đủ thông số kỹ thuật, bảng ánh xạ chân 4 cột, sơ đồ nguyên lý ASCII trực quan.
  - Bảng danh mục lệnh CLI cho cả 3 bo vi điều khiển (biên dịch, nạp code, mở serial monitor).
  - Hướng dẫn kết nối mạch nạp độc lập USB-UART HW-896.
- [x] **Task 6.2: Kiểm tra đối chiếu toàn diện (Verification Gate Rule 3)**
  - Xác nhận toàn bộ các binary `.bin`, `.elf`, `.hex` sinh ra đầy đủ cho cả 4 môi trường: `esp32_master`, `stm32_coprocessor`, `nano_subcontroller`, `wokwi`.

---

## 8. Danh Mục Lệnh CLI Tiêu Chuẩn Thực Thi

```bash
# 1. Biên dịch Node ESP32 Master Gateway
pio run -d pocs/poc-exp-03-multi-mcu-communication -e esp32_master

# 2. Biên dịch Node STM32F4 Coprocessor
pio run -d pocs/poc-exp-03-multi-mcu-communication -e stm32_coprocessor

# 3. Biên dịch Node Arduino Nano I/O Sub-Controller
pio run -d pocs/poc-exp-03-multi-mcu-communication -e nano_subcontroller

# 4. Kiểm tra file nhị phân sau biên dịch
test -f pocs/poc-exp-03-multi-mcu-communication/.pio/build/esp32_master/firmware.bin && echo "ESP32 Firmware BIN OK"
test -f pocs/poc-exp-03-multi-mcu-communication/.pio/build/stm32_coprocessor/firmware.bin && echo "STM32 Firmware BIN OK"
test -f pocs/poc-exp-03-multi-mcu-communication/.pio/build/nano_subcontroller/firmware.hex && echo "Nano Firmware HEX OK"

# 5. Kiểm tra cú pháp file sơ đồ diagram.json
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc-exp-03-multi-mcu-communication/diagram.json", "utf8"))'
```

---

## 9. Ma Trận Đánh Giá Rủi Ro & Giải Pháp Kỹ Thuật (Risk Matrix)

| Rủi Ro Kỹ Thuật | Tác Động Tiêu Cực | Biện Pháp Phòng Ngừa & Xử Lý Triệt Để |
|---|---|---|
| **Cháy chân GPIO ESP32 do nối nhầm 5V từ Nano** | Hỏng vĩnh viễn vi điều khiển ESP32 | Luôn đo kiểm tra điện áp tại ngõ ra cầu phân áp ($3.33\text{V}$) bằng đồng hồ vạn năng trước khi cắm vào chân GPIO 16 (RX2). |
| **Treo bus I2C do STM32 bận tính toán ngắt lâu** | ESP32 bị kẹt lệnh `Wire.endTransmission()` | Cấu hình cờ I2C Clock Stretching timeout trên ESP32 Master; STM32 chỉ thực hiện gán cờ trong ngắt I2C, đưa việc tính toán FPU nặng ra vòng lặp ngoài `loop()`. |
| **Nhiễu điện cảm từ cuộn hút Relay hoặc Động cơ bước** | Vi điều khiển bị reset bất ngờ (Brown-out) | Module Relay trong kit đã có Optocoupler cách ly quang và diode dập xung ngược. Cấp nguồn 5V riêng cho động cơ bước, không kéo chung trên chân 3.3V. |
| **Nghẽn bộ đệm UART Serial2 khi Nano gửi quá nhanh** | Tràn bộ đệm Ring Buffer, sai Checksum | Đặt tốc độ UART2 $115200\text{ bps}$, chu kỳ gửi telemetry từ Nano giới hạn ở mức tối ưu $\ge 200\text{ms}$, kích thước bộ đệm phần cứng đủ lớn ($256\text{ bytes}$). |
