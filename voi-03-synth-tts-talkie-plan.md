# Kế Hoạch Triển Khai Chi Tiết: Đề Tài VOI-03 — Máy Phát Nhạc Điện Tử & Bộ Đọc Giọng Nói Tổng Hợp (Synthesizer & TTS Talkie)

> **Mã Đề Tài:** VOI-03  
> **Git Worktree:** `/Users/toannguyen/.gemini/antigravity/worktrees/esp32-stuff/voi-03-synth-tts-talkie`  
> **Git Branch:** `wt/poc/voi-03-synth-tts-talkie`  
> **Nền Tảng:** ESP32 DevKit V1 (30 chân) + PlatformIO CLI + Arduino Framework + Wokwi Simulator  
> **Thư Mục POC Dự Kiến:** `pocs/poc-voi-03-synth-tts-talkie`  

---

## 1. Yêu Cầu Đề Tài & Mục Tiêu Kỹ Thuật (Requirements & Objectives)

### 1.1 Yêu Cầu Cốt Lõi (Theo `docs/projects/PROJECT-CATALOG-AND-SPECS.md`)
- **Tên Đề Tài:** VOI-03 — Máy Phát Nhạc Điện Tử & Bộ Đọc Giọng Nói Tổng Hợp (Synthesizer & TTS Talkie).
- **Tính Ứng Dụng Thực Tiễn:**
  - Hệ thống chuông báo đa âm điệu, tạo nhạc chiptune retro và âm thanh phản hồi UI cho thiết bị IoT.
  - Bộ phát thông báo bằng giọng nói tiếng Anh ngoại tuyến (Offline Text-to-Speech) **không cần kết nối internet hay thẻ nhớ SD**, sử dụng thuật toán nén âm vị Linear Predictive Coding (LPC).
  - Có thể ứng dụng đọc số đếm, cảnh báo an ninh, thông báo nhiệt độ/trạng thái cảm biến IoT.
- **Danh Sách Thiết Bị Phần Cứng (Sẵn sàng 100% trong bộ Kit):**
  1. 1× Bo mạch **ESP32 DevKit V1 (30 chân)** (MCU lõi kép 240MHz, tích hợp DAC 8-bit và bộ điều khiển phần cứng LEDC PWM).
  2. 1× Còi chíp thụ động **Passive Buzzer** (Loa gốm áp điện Piezo, yêu cầu tín hiệu dao động PWM/DAC).
  3. 1× **Bàn phím ma trận 4x4 (4x4 Membrane Matrix Keypad)** (Gồm 16 phím bấm: đóng vai trò phím đàn Organ, bàn phím chọn bài và chọn câu thoại).
  4. 1× **Biến trở xoay 10kΩ (Potentiometer)** (Chiết áp 3 chân: đóng vai trò núm vặn uốn cao độ Pitch Bend hoặc điều chỉnh tốc độ Tempo BPM).
  5. 1× Màn hình **LED 7 đoạn 4 số TM1637** (Giao tiếp 2 dây: hiển thị nốt nhạc C4-E5, nhịp độ BPM, trạng thái chế độ hoạt động).

### 1.2 Yêu Cầu Nghiêm Ngặt Từ Quy Chuẩn Dự Án (`AGENTS.md`)
- **CLI-First:** Toàn bộ quá trình build, test, lint thực hiện 100% bằng câu lệnh PlatformIO CLI (`pio`) và Wokwi CLI (`wokwi-cli`).
- **ESP32 DevKit V1 30-Pin Constraints:**
  - Không sử dụng các chân SPI Flash nội bộ (GPIO 6 – 11).
  - Tránh các chân Strapping (GPIO 0, 2, 12, 15) cho tải ngoài.
  - Sử dụng kênh **ADC1 (GPIO 34)** cho biến trở xoay, đảm bảo an toàn tuyệt đối khi bật Wi-Fi.
- **Hardware-First Dual-Target Rule:** Môi trường `[env:esp32dev]` cấu hình chuẩn cho phần cứng thật trong Kit.
- **Ghi Nhãn Trực Quan Sơ Đồ Wokwi (`diagram.json` Labels):** Mọi linh kiện trong `diagram.json` bắt buộc có thuộc tính `"label"` trong `"attrs"`.
- **Non-blocking Execution:** Tuyệt đối không dùng `delay()` làm nghẽn CPU trong vòng lặp chính; toàn bộ định thời đều điều phối bằng State Machine và `millis()`.

---

## 2. Nghiên Cứu Kỹ Thuật Chuyên Sâu (Technical Research)

### 2.1 Kiến Trúc Tín Hiệu Âm Thanh & Bàn Giao Phần Cứng (Audio Hardware Handover)
Hệ thống kết hợp 2 công nghệ âm thanh khác nhau trên cùng một chân GPIO 25 nối Passive Buzzer:
1. **Chế Độ Synthesizer / Jukebox / SFX (Hardware PWM LEDC):**
   - Vi điều khiển tạo sóng vuông ở tần số xác định ($100\text{ Hz} - 5000\text{ Hz}$) với Duty Cycle $50\%$.
   - Sử dụng ngoại vi phần cứng **LEDC** của ESP32 qua API `tone(pin, freq)` / `noTone(pin)` hoặc `ledcWriteTone()`.
2. **Chế Độ Speech TTS (Linear Predictive Coding DAC):**
   - Thư viện `Talkie` (phiên bản `arminjo/Talkie`) phát âm bằng cách tái tạo giọng nói dựa trên mô hình TMS5220 với dữ liệu âm vị nén LPC ở tần số mẫu $8\text{ kHz}$.
   - Trên ESP32, thư viện sử dụng trực tiếp bộ chuyển đổi tín hiệu số sang tương tự **DAC1 (GPIO 25)**. Màng loa piezo của Passive Buzzer đáp ứng dao động điện áp analog này và phát ra âm thanh giọng nói robot đặc trưng.
3. **Cơ Chế Bàn Giao Liền Mạch (Seamless Handover Mechanism):**
   - *Vấn đề kỹ thuật:* Nếu LEDC đang chiếm quyền điều khiển GPIO 25, DAC không thể xuất tín hiệu analog chính xác, và ngược lại.
   - *Giải pháp:* Thiết kế lớp quản lý `AudioDriver`:
     - Trước khi gọi `Talkie.say()`, ngắt LEDC bằng `noTone(PIN_BUZZER)` hoặc `ledcDetachPin(PIN_BUZZER)`.
     - Sau khi Talkie nói xong, hoặc khi chuyển sang Organ, khởi tạo lại kênh LEDC qua `tone()`.

### 2.2 Thuật Toán Xử Lý Tín Hiệu Biến Trở (Signal Conditioning)
- Biến trở xoay $10\text{k}\Omega$ nối vào kênh **ADC1_CH6 (GPIO 34)** (chân Input-only an toàn cao).
- Tín hiệu điện áp ADC 12-bit ($0 - 4095$) rất dễ bị nhiễu nhảy số (jitter) ở các bit cuối do nhiễu môi trường.
- **Giải Pháp Xử Lý:**
  - **Lọc Trung Bình Trượt (SMA - Simple Moving Average 8 mẫu):** Loại bỏ nhiễu trắng cao tần.
  - **Vùng Chết (Deadband Hysteresis $\Delta = 35$ đơn vị ADC):** Chỉ cập nhật giá trị Pitch/Tempo khi núm xoay thực sự dịch chuyển vượt ngưỡng trễ, tránh làm âm thanh bị rung rè khi buông tay.

### 2.3 Phân Bổ Chân Phần Cứng Chuẩn Hóa (Pinout Allocation Map)

| Chân ESP32 | Loại Chân | Ngoại Vi ESP32 | Linh Kiện Nối Vào | Chức Năng Cụ Thể |
|---|---|---|---|---|
| **GPIO 25** | Output / DAC | DAC1 / LEDC | **Passive Buzzer (+)** | Xuất sóng vuông PWM (Organ/Nhạc) hoặc tín hiệu DAC (Giọng nói Talkie) |
| **GND** | Mass | Nguồn 0V | **Passive Buzzer (-)** | Nối cực âm còi chíp |
| **GPIO 34** | Input-Only | ADC1_CH6 | **Biến trở xoay 10kΩ (Wiper)** | Đọc điện áp analog uốn cao độ Pitch Bend hoặc tốc độ Tempo |
| **3V3** | Nguồn | Nguồn 3.3V | **Biến trở chân 1, TM1637 VCC** | Cấp nguồn nuôi chuẩn |
| **GND** | Mass | Nguồn 0V | **Biến trở chân 3, TM1637 GND** | Nối mass chung |
| **GPIO 4** | Digital I/O | GPIO | **TM1637 CLK** | Xung Clock đồng bộ giao tiếp 2 dây màn hình LED 7 đoạn |
| **GPIO 23** | Digital I/O | VSPI MOSI / GPIO | **TM1637 DIO** | Dữ liệu Data I/O màn hình LED 7 đoạn |
| **GPIO 13** | Digital Output | GPIO | **Keypad Row 1 (R1)** | Quét hàng 1 ma trận phím |
| **GPIO 14** | Digital Output | GPIO | **Keypad Row 2 (R2)** | Quét hàng 2 ma trận phím |
| **GPIO 27** | Digital Output | GPIO | **Keypad Row 3 (R3)** | Quét hàng 3 ma trận phím |
| **GPIO 26** | Digital Output | GPIO | **Keypad Row 4 (R4)** | Quét hàng 4 ma trận phím |
| **GPIO 18** | Digital Input | Pull-Up / GPIO | **Keypad Col 1 (C1)** | Đọc cột 1 ma trận phím |
| **GPIO 19** | Digital Input | Pull-Up / GPIO | **Keypad Col 2 (C2)** | Đọc cột 2 ma trận phím |
| **GPIO 21** | Digital Input | Pull-Up / GPIO | **Keypad Col 3 (C3)** | Đọc cột 3 ma trận phím |
| **GPIO 22** | Digital Input | Pull-Up / GPIO | **Keypad Col 4 (C4)** | Đọc cột 4 ma trận phím |

> 🛡️ **Xác Nhận An Toàn 100%:**
> - Tuyệt đối không chạm vào GPIO 6–11 (SPI Flash).
> - Hoàn toàn tránh các chân Strapping nhạy cảm (GPIO 0, 2, 12, 15).
> - Giữ nguyên cổng nạp và gỡ lỗi UART0 (GPIO 1, 3) cho PlatformIO Serial Monitor.

---

## 3. Kiến Trúc Phần Mềm (Software Architecture)

### 3.1 Sơ Đồ Khối Chức Năng (Block Diagram)

```
┌────────────────────────────────────────────────────────────────────────┐
│                        ESP32 DevKit V1 (Main Loop)                     │
│                                                                        │
│  ┌──────────────────┐      ┌──────────────────┐      ┌───────────────┐ │
│  │  Keypad Scanner  │      │  ADC Potentiometer│      │ FSM Controller│ │
│  │   (Debounce)     │      │  (SMA Filter)    │      │  (4 Modes)    │ │
│  └────────┬─────────┘      └────────┬─────────┘      └───────┬───────┘ │
│           │                         │                        │         │
│           └─────────────────────────┼────────────────────────┘         │
│                                     ▼                                  │
│                 ┌───────────────────────────────────────┐              │
│                 │          Mode Dispatcher              │              │
│                 └───┬───────────┬───────────┬───────┬───┘              │
│                     │           │           │       │                  │
│          ┌──────────▼─┐  ┌──────▼─────┐ ┌───▼────┐ ┌▼─────────┐        │
│          │Mode 1: OrG │  │Mode 2: JUKE│ │Mode 3: │ │Mode 4:   │        │
│          │Synthesizer │  │Jukebox     │ │TALK TTS│ │8-Bit SFX │        │
│          └──────┬─────┘  └──────┬─────┘ └───┬────┘ └────┬─────┘        │
│                 │               │           │           │              │
│                 ▼               ▼           │           ▼              │
│           ┌───────────────────────────┐     │     ┌───────────┐        │
│           │   LEDC PWM Tone Engine    │     │     │  TM1637   │        │
│           └─────────────┬─────────────┘     │     │  Display  │        │
│                         │ (Tone)            │     └───────────┘        │
│                         ▼                   ▼ (DAC)                    │
│                 ┌───────────────────────────────────────┐              │
│                 │ Passive Buzzer / Loa Áp Điện (GPIO25) │              │
│                 └───────────────────────────────────────┘              │
└────────────────────────────────────────────────────────────────────────┘
```

### 3.2 4 Chế Độ Hoạt Động (Finite State Machine)

1. **Chế Độ 1: 🎹 ORGAN SYNTHESIZER (`OrG`):**
   - Phím `1` đến `9`, `0`: Tương ứng 10 nốt nhạc cơ bản (C4, D4, E4, F4, G4, A4, B4, C5, D5, E5).
   - Biến trở xoay: Núm uốn cao độ (Pitch Bend) tức thời $\pm 50\text{ Hz}$.
   - Phím `*`: Hạ 1 quãng tám (Octave Down).
   - Phím `#`: Tăng 1 quãng tám (Octave Up).
   - TM1637: Hiển thị tên nốt đang chơi (ví dụ: `C - 4`, `d - 4`, `E - 4`, `G - 5`).
2. **Chế Độ 2: 🎵 JUKEBOX / RETRO CHIPTUNE PLAYER (`JUKE`):**
   - Chứa danh sách các bản nhạc kinh điển:
     - `1`: Super Mario Bros Main Theme
     - `2`: Star Wars Imperial March
     - `3`: Tetris Theme (Korobeiniki)
     - `4`: Ode to Joy (Beethoven)
     - `5`: Pink Panther Theme
   - Biến trở xoay: Điều chỉnh nhịp độ Tempo real-time từ 60 BPM đến 240 BPM.
   - Phím `*`: Tạm dừng / Tiếp tục (Pause/Resume).
   - Phím `#`: Dừng phát (Stop).
   - TM1637: Hiển thị số hiệu bài và tốc độ (`Sn 1`, `120b`).
3. **Chế Độ 3: 🗣️ OFFLINE TTS TALKIE (`tALk`):**
   - Tổng hợp giọng nói LPC qua thư viện `Talkie`:
     - `1`: Đọc số đếm ("Zero", "One", "Two", "Three", "Four", "Five").
     - `2`: Thông báo cảnh báo an ninh ("Warning! Danger! Alert!").
     - `3`: Báo cáo thời tiết IoT ("System Ready. Temperature thirty degrees.").
     - `4`: Chào mừng và ủy quyền ("Access granted. Welcome.").
     - `5`: Lời chào buổi sáng ("Good morning.").
   - TM1637: Hiển thị trạng thái phát âm (`SAY `, `dAnG`, `rEAd`).
4. **Chế Độ 4: 🔊 8-BIT SOUND EFFECTS GENERATOR (`SFX`):**
   - Hiệu ứng âm thanh game cổ điển:
     - `1`: Laser Blaster Zap (tần số quét dốc từ 2000Hz xuống 300Hz).
     - `2`: Coin Pickup Ding (hai nốt cao liên tiếp B5 -> E6).
     - `3`: Jump Sound Boing (quét tần số đi lên 150Hz -> 600Hz).
     - `4`: Power-Up Fanfare.
     - `5`: Police Siren Alarm (sóng tam giác quét tần số 600Hz - 1200Hz).
   - TM1637: Hiển thị tên hiệu ứng (`LASE`, `COIn`, `JUIP`, `SIrE`).

### 3.3 Điều Khiển Menu & Phím Nóng Toàn Cục
- Phím `D`: Chuyển đổi vòng tròn giữa 4 chế độ (`OrG` $\rightarrow$ `JUKE` $\rightarrow$ `tALk` $\rightarrow$ `SFX` $\rightarrow$ `OrG`).
- Phím `A`: Phím nóng nhảy ngay về chế độ Organ Synthesizer.
- Phím `B`: Phím nóng nhảy ngay về chế độ TTS Talkie.
- Phím `C`: Phím nóng nhảy ngay về chế độ Jukebox.

---

## 4. Cấu Trúc Thư Mục Dự Án

```text
pocs/poc-voi-03-synth-tts-talkie/
├── platformio.ini              # Cấu hình PlatformIO dual-target, lib_deps (Talkie, TM1637Display, Keypad)
├── wokwi.toml                  # Cấu hình nạp firmware ELF/BIN cho Wokwi CLI
├── diagram.json                # Sơ đồ kết nối phần cứng ảo Wokwi kèm nhãn label chuẩn
├── README.md                   # Hướng dẫn chi tiết, bảng map chân, ca kiểm thử thực tế
└── src/
    ├── main.cpp                # Vòng lặp chính, khởi tạo FSM và dispatch sự kiện
    ├── config.h                # Định nghĩa chân GPIO, hằng số cấu hình, bảng nốt tần số
    ├── keypad_driver.h         # Khai báo lớp quét bàn phím ma trận 4x4
    ├── keypad_driver.cpp       # Triển khai quét phím, bắt sự kiện KeyDown/KeyUp non-blocking
    ├── pot_driver.h            # Khai báo lớp đọc biến trở ADC1
    ├── pot_driver.cpp          # Bộ lọc SMA và thuật toán tính Pitch Bend / Tempo BPM
    ├── tm1637_driver.h         # Khai báo lớp điều khiển hiển thị LED 7 đoạn
    ├── tm1637_driver.cpp       # Font ký tự tùy biến, hiển thị nốt nhạc, BPM và trạng thái
    ├── audio_engine.h          # Bộ điều khiển trung tâm âm thanh, quản lý LEDC vs DAC
    ├── audio_engine.cpp        # Hàm phát tone, ngắt tone, chuyển chế độ âm thanh an toàn
    ├── synth_mode.h            # Khai báo logic Organ & SFX
    ├── synth_mode.cpp          # Triển khai nốt đàn Organ, Pitch Bend và tạo hiệu ứng 8-bit
    ├── jukebox_mode.h          # Khai báo logic máy phát nhạc Chiptune
    ├── jukebox_mode.cpp        # Trình phát giai điệu non-blocking theo nhịp Tempo BPM
    ├── talkie_mode.h           # Khai báo logic bộ đọc TTS
    ├── talkie_mode.cpp         # Tích hợp thư viện Talkie LPC vocabulary và phát câu thoại
    └── songs_data.h            # Mảng dữ liệu các nốt và trường độ của các bản nhạc chiptune
```

---

## 5. Kế Hoạch Triển Khai Chi Tiết Từng Task (Step-by-Step Implementation Tasks)

### Milestone 1: Cấu Hình Dự Án & Môi Trường Biên Dịch (PlatformIO Scaffolding)
- [x] **Task 1.1:** Khởi tạo cấu trúc thư mục `pocs/poc-voi-03-synth-tts-talkie` và các thư mục con `src/`.
- [x] **Task 1.2:** Tạo `platformio.ini` khai báo board `esp32dev`, framework `arduino`, `monitor_speed = 115200`, cùng danh sách thư viện phụ thuộc:
  - `arminjo/Talkie @ ^1.4.0`
  - `https://github.com/avishorp/TM1637.git`
  - `chris--a/Keypad @ ^3.1.1`
- [x] **Task 1.3:** Tạo `wokwi.toml` chỉ đường dẫn tới `.pio/build/esp32dev/firmware.elf` và `firmware.bin`.

### Milestone 2: Thiết Kế Sơ Đồ Wokwi & Kiểm Thử Cú Pháp (Wokwi Simulation Circuit)
- [x] **Task 2.1:** Soạn thảo `diagram.json` kết nối đầy đủ:
  - `board-esp32-devkit-v1`
  - `wokwi-buzzer` (GPIO 25, GND)
  - `wokwi-potentiometer` (GPIO 34, 3V3, GND)
  - `wokwi-tm1637-7segment` (CLK: GPIO 4, DIO: GPIO 23, 3V3, GND)
  - `wokwi-membrane-keypad` (R1..R4: 13, 14, 27, 26; C1..C4: 18, 19, 21, 22)
- [x] **Task 2.2:** Gán đầy đủ thuộc tính `"label"` cho tất cả linh kiện theo đúng quy định 6 của `AGENTS.md`.
- [x] **Task 2.3:** Kiểm tra cú pháp JSON bằng Node.js script.

### Milestone 3: Driver Ngoại Vi Cơ Sở (Keypad, Display & Potentiometer)
- [x] **Task 3.1:** Soạn thảo `config.h` chuẩn hóa mọi định nghĩa chân GPIO, hằng số tần số nốt chuẩn ($C_4 = 262\text{ Hz} \dots E_5 = 659\text{ Hz}$).
- [x] **Task 3.2:** Triển khai `keypad_driver` xử lý bắt phím non-blocking với sự kiện nhấn (PRESSED) và nhả (RELEASED).
- [x] **Task 3.3:** Triển khai `tm1637_driver` ánh xạ các ký tự chữ cái 7 thanh (`OrG`, `JUKE`, `tALk`, `SFX`, `C-4`, `d-4`...) và hiển thị số.
- [x] **Task 3.4:** Triển khai `pot_driver` đọc ADC1 GPIO 34 kèm bộ lọc SMA 8 mẫu và Deadband $\pm 35$ đơn vị.

### Milestone 4: Audio Engine & Bộ Organ Synthesizer (Tone Generation & Pitch Bend)
- [x] **Task 4.1:** Xây dựng `audio_engine` đóng gói các thao tác điều khiển LEDC tone và quản lý chuyển đổi tài nguyên với DAC.
- [x] **Task 4.2:** Triển khai `synth_mode` xử lý đánh đàn 10 phím nốt, dịch Octave với phím `*` / `#`, và áp dụng Pitch Bend biến thiên tức thì theo biến trở.
- [x] **Task 4.3:** Bổ sung các hàm phát hiệu ứng âm thanh 8-bit cổ điển (`SFX`): Laser, Coin, Jump, Fanfare, Siren.

### Milestone 5: Trình Phát Nhạc Jukebox Chiptune Non-Blocking
- [x] **Task 5.1:** Soạn thảo `songs_data.h` mã hóa 4 bản nhạc kinh điển (Mario Theme, Star Wars, Tetris, Ode to Joy) dưới dạng mảng nốt và độ dài nhịp.
- [x] **Task 5.2:** Xây dựng `jukebox_mode` phát nhạc theo State Machine `millis()`, hỗ trợ Pause/Resume, Stop và thay đổi Tempo BPM động qua biến trở mà không chặn CPU.

### Milestone 6: Tích Hợp Giọng Nói Tổng Hợp TTS Talkie
- [x] **Task 6.1:** Triển khai `talkie_mode` tích hợp từ vựng tiếng Anh LPC từ thư viện `Talkie` (số đếm, cảnh báo an toàn, câu chào, trạng thái thiết bị).
- [x] **Task 6.2:** Đảm bảo giải phóng kênh LEDC trước khi kích hoạt Talkie DAC và phục hồi lại sau khi đọc xong.

### Milestone 7: Tích Hợp FSM Trung Tâm & CLI Serial Diagnostics
- [x] **Task 7.1:** Hoàn thiện `main.cpp` kết nối toàn bộ các mode qua FSM điều phối bởi phím `D` và phím nóng `A, B, C`.
- [x] **Task 7.2:** Thiết lập hệ thống log Serial 115200 baud trực quan, có banner khởi động và marker phục vụ kiểm thử tự động.
- [x] **Task 7.3:** Biên dịch mã nguồn với `pio run -d pocs/poc-voi-03-synth-tts-talkie -e esp32dev` và xác thực binary artifacts.

### Milestone 8: Tài Liệu Hướng Dẫn & Nghiệm Thu (README & Hardware Verification Guide)
- [x] **Task 8.1:** Viết `README.md` hoàn chỉnh cho POC gồm: bảng phân bổ chân đối chiếu 1-1, sơ đồ nguyên lý mạch, hướng dẫn thao tác phím và kịch bản nghiệm thu trên board thật.
- [ ] **Task 8.2:** Chạy kiểm thử tự động bằng Wokwi CLI (`wokwi-cli --expect-text` hoặc log verification).

---

## 6. Tiêu Chí Nghiệm Thu (Acceptance Criteria & Verification Gate)

1. **Biên Dịch Sạch Sẽ (Build Gate):**
   - Lệnh `pio run -d pocs/poc-voi-03-synth-tts-talkie -e esp32dev` trả về mã lỗi 0 (SUCCESS).
   - Artifacts `.pio/build/esp32dev/firmware.bin` và `.pio/build/esp32dev/firmware.elf` được tạo thành công.
2. **Kiểm Tra Sơ Đồ Wokwi (Lint Gate):**
   - File `diagram.json` hợp lệ JSON 100%.
   - 100% linh kiện ngoại vi có thuộc tính `"label"` hợp lệ.
3. **Chức Năng Organ Synthesizer:**
   - Bấm các phím số `1`..`0` phát ra đúng tần số các nốt C4..E5 qua buzzer.
   - Vặn biến trở làm tần số nốt uốn lượn (Pitch Bend) mượt mà.
   - Màn hình TM1637 cập nhật đúng tên nốt đang phát.
4. **Chức Năng Jukebox Melody:**
   - Phát các bản nhạc Mario/Tetris theo chuỗi nốt chính xác.
   - Vặn biến trở thay đổi tốc độ bản nhạc (BPM) ngay lập tức khi đang phát.
5. **Chức Năng Offline TTS Talkie:**
   - Phát âm rõ ràng các cụm từ số và cảnh báo qua chân GPIO 25.
   - Không gây xung đột treo máy khi chuyển đổi qua lại giữa Tone PWM và Talkie DAC.
6. **Log Khởi Động Serial:**
   - Serial Monitor in ra banner chào mừng: `[VOI-03] SYNTHESIZER & TTS TALKIE SYSTEM READY`.
