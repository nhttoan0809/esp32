# POC VOI-03: Máy Phát Nhạc Điện Tử & Bộ Đọc Giọng Nói Tổng Hợp (Synthesizer & TTS Talkie)

> **Mã Đề Tài:** VOI-03  
> **Nhóm Đề Tài:** Giọng Nói & Xử Lý Tín Hiệu Âm Thanh (Voice & Audio Processing)  
> **Nền Tảng:** ESP32 DevKit V1 (30 chân) + Arduino Framework + PlatformIO CLI + Wokwi Simulator  
> **Trạng Thái Linh Kiện:** 🟢 **Sẵn sàng 100% trong bộ Kit thí nghiệm**  

---

## 1. Giới Thiệu & Mục Tiêu

Dự án **VOI-03** xây dựng một hệ thống âm thanh nhúng đa năng toàn diện trên nền tảng vi điều khiển ESP32, tích hợp 4 chế độ hoạt động trong một firmware duy nhất:
1. **🎹 Organ Synthesizer:** Bàn phím đàn 16 nốt với khả năng uốn cao độ Pitch Bend mượt mà bằng núm vặn biến trở và đổi quãng tám Octave linh hoạt.
2. **🎵 Retro Chiptune Jukebox:** Máy phát các bản nhạc 8-bit kinh điển (Super Mario Bros, Star Wars, Tetris, Ode to Joy) kèm tính năng điều chỉnh tốc độ nhịp (Tempo BPM) trực tiếp theo thời gian thực (real-time).
3. **🗣️ Offline TTS Talkie:** Bộ đọc giọng nói tiếng Anh ngoại tuyến không cần kết nối internet hay thẻ nhớ SD, sử dụng thuật toán nén âm vị Linear Predictive Coding (LPC) phát âm qua DAC1 GPIO 25.
4. **🔊 8-Bit Sound Effects Generator:** Máy tạo hiệu ứng âm thanh game cổ điển (Laser Blaster, Coin Pickup, Jump Boing, Power-Up, Siren Alarm).

---

## 2. Bảng Phân Bổ & Ánh Xạ Chân Phần Cứng (Hardware Pinout Map 1-1)

Bảng đối chiếu chuẩn hóa giữa bo mạch vi điều khiển ESP32 DevKit V1 (30 chân), linh kiện phần cứng thật trong Kit và phần tử mô phỏng Wokwi:

| Chân ESP32 | Loại Chân | Ký Hiệu Trên Bo Thật | Chân Trên Wokwi (`diagram.json`) | Chức Năng Kỹ Thuật & Lưu Ý An Toàn |
|---|---|---|---|---|
| **GPIO 25** | Output / DAC1 | Chân D25 (Hàng trái, chân 8) | `esp:D25` $\rightarrow$ `bz1:2` | Ngõ ra âm thanh kép: Xung vuông PWM LEDC cho Organ/Nhạc hoặc Tín hiệu DAC cho Giọng nói Talkie |
| **GND** | Mass | Chân GND (Hàng trái, chân 14) | `esp:GND.1` $\rightarrow$ `bz1:1` | Nối mass cho Passive Buzzer |
| **GPIO 34** | Input-Only | Chân D34 (Hàng trái, chân 4) | `esp:D34` $\rightarrow$ `pot1:SIG` | Kênh ADC1_CH6 đọc điện áp analog biến trở (0–3.3V) uốn cao độ Pitch Bend hoặc Tempo BPM |
| **3V3** | Nguồn 3.3V | Chân 3V3 (Hàng phải, chân 16) | `esp:3V3` $\rightarrow$ `pot1:VCC`, `tm1:VCC` | Cấp nguồn chuẩn 3.3V cho biến trở và màn hình TM1637 |
| **GND** | Mass | Chân GND (Hàng phải, chân 17) | `esp:GND.2` $\rightarrow$ `pot1:GND`, `tm1:GND` | Nối mass chung cho biến trở và TM1637 |
| **GPIO 4** | Digital I/O | Chân D4 (Hàng phải, chân 20) | `esp:D4` $\rightarrow$ `tm1:CLK` | Chân tạo xung nhịp Clock cho màn hình TM1637 |
| **GPIO 23** | Digital I/O | Chân D23 (Hàng phải, chân 30) | `esp:D23` $\rightarrow$ `tm1:DIO` | Chân truyền nhận dữ liệu Data I/O cho màn hình TM1637 |
| **GPIO 13** | Digital Output | Chân D13 (Hàng trái, chân 13) | `esp:D13` $\rightarrow$ `kp1:R1` | Quét Hàng 1 (Row 1) của bàn phím ma trận 4x4 |
| **GPIO 14** | Digital Output | Chân D14 (Hàng trái, chân 11) | `esp:D14` $\rightarrow$ `kp1:R2` | Quét Hàng 2 (Row 2) của bàn phím ma trận 4x4 |
| **GPIO 27** | Digital Output | Chân D27 (Hàng trái, chân 10) | `esp:D27` $\rightarrow$ `kp1:R3` | Quét Hàng 3 (Row 3) của bàn phím ma trận 4x4 |
| **GPIO 26** | Digital Output | Chân D26 (Hàng trái, chân 9) | `esp:D26` $\rightarrow$ `kp1:R4` | Quét Hàng 4 (Row 4) của bàn phím ma trận 4x4 |
| **GPIO 18** | Digital Input | Chân D18 (Hàng phải, chân 24) | `esp:D18` $\rightarrow$ `kp1:C1` | Đọc Cột 1 (Col 1) ma trận phím (Kéo lên Pull-Up) |
| **GPIO 19** | Digital Input | Chân D19 (Hàng phải, chân 25) | `esp:D19` $\rightarrow$ `kp1:C2` | Đọc Cột 2 (Col 2) ma trận phím (Kéo lên Pull-Up) |
| **GPIO 21** | Digital Input | Chân D21 (Hàng phải, chân 26) | `esp:D21` $\rightarrow$ `kp1:C3` | Đọc Cột 3 (Col 3) ma trận phím (Kéo lên Pull-Up) |
| **GPIO 22** | Digital Input | Chân D22 (Hàng phải, chân 29) | `esp:D22` $\rightarrow$ `kp1:C4` | Đọc Cột 4 (Col 4) ma trận phím (Kéo lên Pull-Up) |

> 🛡️ **An Toàn Phần Cứng:**
> - Tuyệt đối không can thiệp vào GPIO 6–11 (kết nối Flash SPI nội bộ).
> - Hoàn toàn không sử dụng các chân Strapping nhạy cảm (GPIO 0, 2, 12, 15).
> - Kênh Analog dùng GPIO 34 thuộc ADC1, không bị xung đột khi kích hoạt Wi-Fi/Bluetooth.

---

## 3. Hướng Dẫn Thao Tác & Điều Khiển

### 3.1 Phím Nóng Toàn Cục (Global Shortcuts)
- **`[D]`:** Chuyển vòng tròn qua 4 chế độ: `OrG` $\rightarrow$ `JUKE` $\rightarrow$ `tALk` $\rightarrow$ `SFX` $\rightarrow$ `OrG`.
- **`[A]`:** Phím tắt chuyển ngay về chế độ **Organ Synthesizer**.
- **`[B]`:** Phím tắt chuyển ngay về chế độ **Offline TTS Talkie**.
- **`[C]`:** Phím tắt chuyển ngay về chế độ **Jukebox Retro Chiptune**.

### 3.2 Hướng Dẫn Chi Tiết Từng Chế Độ

#### 🎹 1. Chế Độ Organ Synthesizer (`OrG`)
- **Phím nốt:** Bấm và giữ các phím số `1`..`9`, `0` để ngân nốt nhạc tương ứng:
  - `1`: C4 (Do - 262Hz)
  - `2`: D4 (Re - 294Hz)
  - `3`: E4 (Mi - 330Hz)
  - `4`: F4 (Fa - 349Hz)
  - `5`: G4 (Sol - 392Hz)
  - `6`: A4 (La - 440Hz)
  - `7`: B4 (Si - 494Hz)
  - `8`: C5 (Do cao - 523Hz)
  - `9`: D5 (Re cao - 587Hz)
  - `0`: E5 (Mi cao - 659Hz)
- **Núm xoay Pitch Bend:** Vặn biến trở 10kΩ để uốn cong cao độ tức thời trong khoảng $\pm 50\text{ Hz}$.
- **Phím `*` / `#`:** Hạ / Tăng quãng tám (Octave 3, 4, 5).
- **Màn hình TM1637:** Hiển thị nốt đang chơi (`C - 4`, `d - 4`...).

#### 🎵 2. Chế Độ Jukebox Retro Chiptune (`JUKE`)
- **Chọn bài nhạc:**
  - `1`: Super Mario Bros Main Theme
  - `2`: Star Wars Imperial March (Darth Vader Theme)
  - `3`: Tetris Korobeiniki Theme
  - `4`: Beethoven Ode to Joy
- **Điều chỉnh tốc độ (Tempo):** Vặn biến trở để thay đổi trực tiếp nhịp độ bài nhạc từ **60 BPM đến 240 BPM**. Màn hình TM1637 hiển thị tốc độ hiện tại (`120b`).
- **Phím `*`:** Tạm dừng (Pause) hoặc Tiếp tục (Resume).
- **Phím `#`:** Dừng phát (Stop).

#### 🗣️ 3. Chế Độ Offline TTS Talkie (`tALk`)
- Nhấn các phím số để vi điều khiển phát âm giọng nói robot LPC qua DAC1:
  - `1`: Đếm số: *"Zero, One, Two, Three, Four, Five"*
  - `2`: Cảnh báo an ninh: *"Warning! Danger! Alert!"*
  - `3`: Báo cáo thời tiết IoT: *"System Ready. Temperature thirty degrees."*
  - `4`: Xác thực truy cập: *"Access granted. Welcome."*
  - `5`: Lời chào buổi sáng: *"Good morning."*
- Màn hình TM1637 hiển thị trạng thái phát âm (`COUn`, `dAnG`, `rEAd`, `PASS`, `COOd`).

#### 🔊 4. Chế Độ 8-Bit SFX Generator (`SFX`)
- `1`: Laser Blaster Zap
- `2`: Coin Pickup Ding
- `3`: Jump Sound Boing
- `4`: Power-Up Fanfare
- `5`: Police Siren Alarm

---

## 4. Hướng Dẫn Biên Dịch & Chạy Kiểm Thử (CLI Commands)

### 4.1 Kiểm Tra Cú Pháp Sơ Đồ Wokwi
```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc-voi-03-synth-tts-talkie/diagram.json", "utf8"))'
```

### 4.2 Biên Dịch Firmware
```bash
pio run -d pocs/poc-voi-03-synth-tts-talkie -e esp32dev
```

### 4.3 Nạp Lên Bo Mạch ESP32 Thật & Mở Serial Monitor
```bash
# 1. Liệt kê cổng serial trên máy
pio device list

# 2. Nạp firmware qua cổng USB
pio run -d pocs/poc-voi-03-synth-tts-talkie -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# 3. Theo dõi Serial Monitor (Baudrate 115200)
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```
