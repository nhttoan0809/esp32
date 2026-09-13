# POC: PIR Motion Sensor Controller (Đèn Thông Minh Tự Tắt & Báo Động An Ninh)

Bản Proof of Concept (POC) thực nghiệm ứng dụng **Module Cảm biến Chuyển động hồng ngoại thụ động PIR HC-SR501** kết hợp giải thuật **Non-blocking Keep-Alive Timer (Hold Time)** và **FSM 2 chế độ (Tiết kiệm điện vs Báo động an ninh)** trên vi điều khiển **ESP32 DevKit V1 (30 chân)**.

---

## 1. Thông Số Tính Toán & Giải Thuật

- **Cảm biến PIR HC-SR501:**
  - Nguồn cấp bắt buộc: **VIN (5V)** từ cổng USB để vi mạch xử lý tín hiệu BISS0001 và chip ổn áp 7133 hoạt động ổn định.
  - Ngõ ra `OUT`: Tín hiệu logic **3.3V TTL High** khi phát hiện bức xạ hồng ngoại cơ thể sống di chuyển, mức **0V Low** khi tĩnh. Nối an toàn vào **GPIO 33**.
  - Xử lý thời gian làm nóng quang học (Warm-up Period: $10\text{s}$ ban đầu để tránh báo động giả lúc khởi động).
  - **Thông số 2 núm chỉnh phần cứng (Trimmer Potentiometers):**
    - **Núm chỉnh độ nhạy (Sensitivity Adjust - Sx):**
      - Dải cự ly: $3\text{m} - 7\text{m}$, góc quét $\approx 120^\circ$.
      - Xoay theo chiều kim đồng hồ (CW): Tăng khoảng cách nhận diện (lên tối đa ~7m).
      - Xoay ngược chiều kim đồng hồ (CCW): Giảm khoảng cách nhận diện (xuống tối thiểu ~3m).
      - *Cấu hình phạm vi phần mềm (Software Range Profiles):* Do module PIR chỉ xuất tín hiệu logic số (0/1), ESP32 kết hợp bộ lọc thời lượng xung (*Duration Filter*) để cung cấp 3 mức phạm vi:
        - **Mức 1: Cự ly GẦN (~3m):** Yêu cầu tín hiệu duy trì $\ge 400\text{ms}$ (Lọc triệt để báo giả do vật nuôi, rèm cửa).
        - **Mức 2: TIÊU CHUẨN (~5m):** Yêu cầu tín hiệu duy trì $\ge 150\text{ms}$ (Cân bằng, mặc định).
        - **Mức 3: Cự ly XA (~7m):** Kích hoạt tức thời khi xung đạt $\ge 40\text{ms}$ (Độ nhạy tối đa).
        - *Cách chuyển mức:* Gõ phím `1`, `2`, `3` trên Serial Monitor HOẶC nhấn giữ nút bấm GPIO 4 trong $>1.2\text{s}$.
    - **Núm chỉnh thời gian trễ (Time Delay Adjust - Tx):**
      - Dải thời gian: $\approx 0.5\text{s} - 300\text{s}$ (5 phút), tuân theo công thức chip BISS0001: $Tx \approx 24576 \times R_{10} \times C_6$.
      - Xoay theo chiều kim đồng hồ (CW): Tăng thời gian xung `OUT` giữ mức `HIGH` (tối đa 5 phút).
      - Xoay ngược chiều kim đồng hồ (CCW): Giảm thời gian trễ về mức nhỏ nhất (~0.5s - 3s).
      - *Đo lường tự động:* ESP32 tự động đo thời gian giữ mức HIGH thực tế khi xung kết thúc và in kết quả ra Serial.
      - *Khuyến nghị căn chỉnh:* Vặn kịch kim ngược chiều kim đồng hồ (CCW) và gạt Jumper sang vị trí **H (Repeatable)** để ESP32 đo xung nhanh và làm chủ hoàn toàn giải thuật thời gian chờ (Hold Time) qua phần mềm.
- **Cơ Chế Hoạt Động Của Nút Bấm & Phím Tắt Serial:**
  - **Cơ chế phần cứng (INPUT_PULLUP):** Chân GPIO 4 được bật điện trở kéo lên nội bộ (~45kΩ lên 3.3V).
    - Trạng thái nhả (Release): Mức logic `HIGH` (3.3V).
    - Trạng thái nhấn (Press): Tiếp điểm cơ khí đóng xuống `GND`, tạo mức logic `LOW` (0V).
  - **Hai thao tác nút bấm:**
    - **Nhấn nhanh (< 1.2s):** Chuyển đổi chế độ hoạt động (AUTO-LIGHT $\leftrightarrow$ ARMED SECURITY).
    - **Nhấn giữ (> 1.2s):** Xoay vòng đổi mức phạm vi độ nhạy ($1 \rightarrow 2 \rightarrow 3 \rightarrow 1$), còi bíp số tiếng tương ứng xác nhận.
  - **Điều khiển qua Serial Monitor:**
    - Gõ `1`, `2`, `3`: Thay đổi phạm vi độ nhạy (Gần 3m / Vừa 5m / Xa 7m).
    - Gõ `t` hoặc `b`: Chạy chương trình chẩn đoán còi Buzzer (Phát 3 tần số 1500Hz, 2400Hz, 3200Hz).
    - Gõ `m`: Chuyển chế độ hoạt động.
    - Gõ `?`: In menu trợ giúp lệnh.
  - **Máy trạng thái 2 chế độ (Dual-Mode FSM):**
    - **Chế độ 1: Đèn tự động tiết kiệm điện (AUTO-LIGHT - Mặc định khi khởi động):**
      - Khi có chuyển động: Bật đèn LED Vàng (GPIO 21) và liên tục gia hạn thời gian sáng (`lastMotionTime = millis()`).
      - Khi người rời đi: Giữ sáng thêm $8\text{s}$ (Hold Time) rồi tự động tắt. Còi Buzzer ngắt hoàn toàn để giữ yên tĩnh.
    - **Chế độ 2: Báo động chống trộm (ARMED SECURITY):**
      - Kích hoạt bằng nút bấm hoặc phím `m` (Còi bíp 2 tiếng xác nhận ARM).
      - Khi phát hiện có chuyển động: Kích hoạt đồng thời chớp đèn LED và hú còi Buzzer (GPIO 22) ở tần số $2500\text{Hz}$ với nhịp $120\text{ms}$.
      - Nhấn nút lần nữa để giải phóng (DISARM về AUTO-LIGHT - Còi kêu 1 tiếng dài).
- **Cơ chế Điều Khiển Còi Buzzer (Active & Passive):**
  - Sử dụng hàm tạo xung âm thanh `tone(PIN_BUZZER, freq)` thay vì chỉ `digitalWrite(HIGH)` để tương thích 100% với cả **Passive Buzzer** (còi thụ động) lẫn **Active Buzzer** (còi chủ động).
  - Tự động phát 2 tiếng bíp lúc khởi động (**Power-on Self Test**) để người dùng kiểm chứng phần cứng ngay khi cấp nguồn.

---

## 2. Bản Đồ Nối Dây (Pinout Map)

| Chân ESP32 | Linh kiện | Chân linh kiện | Chức năng |
|---|---|---|---|
| **VIN (5V)** | Module PIR HC-SR501 | `VCC` | Nguồn cấp 5V |
| **GND** | Module PIR HC-SR501 | `GND` | Nối đất |
| **D33 (GPIO 33)** | Module PIR HC-SR501 | `OUT` | Tín hiệu 3.3V TTL |
| **D4 (GPIO 4)** | Nút bấm 12×12 | Chân 1.l $\rightarrow$ 2.l nối GND | Nút chuyển chế độ (INPUT_PULLUP) |
| **D21 (GPIO 21)** | Điện trở $220\Omega$ $\rightarrow$ LED Vàng (A) | Anode (+) | Đèn chiếu sáng thông minh |
| **GND** | LED Vàng | Cathode (-) | Nối đất |
| **D22 (GPIO 22)** | Active Buzzer | Cực dương (+) | Còi báo động an ninh |
| **GND** | Active Buzzer | Cực âm (-) | Nối đất |

---

## 3. Lệnh Thao Tác Dòng Lệnh (CLI-First)

### 3.1 Kiểm tra sơ đồ Wokwi
```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc-pir-motion-alarm/diagram.json", "utf8"))'
wokwi-cli lint pocs/poc-pir-motion-alarm
```

### 3.2 Biên dịch Firmware
```bash
pio run -d pocs/poc-pir-motion-alarm -e esp32dev
test -f pocs/poc-pir-motion-alarm/.pio/build/esp32dev/firmware.bin && echo "BIN OK"
```

### 3.3 Mô phỏng trên Wokwi Simulator
- Bấm vào cảm biến PIR, chọn **"Simulate Motion"** để kích hoạt chuyển động.
- Nhấn phím `m` (hoặc click nút Mode Toggle) để chuyển giữa chế độ Auto-Light và Armed Alarm.

### 3.4 Nạp lên Board thật
```bash
pio run -d pocs/poc-pir-motion-alarm -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```
