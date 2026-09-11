# POC: PIR Motion Sensor Controller (Đèn Thông Minh Tự Tắt & Báo Động An Ninh)

Bản Proof of Concept (POC) thực nghiệm ứng dụng **Module Cảm biến Chuyển động hồng ngoại thụ động PIR HC-SR501** kết hợp giải thuật **Non-blocking Keep-Alive Timer (Hold Time)** và **FSM 2 chế độ (Tiết kiệm điện vs Báo động an ninh)** trên vi điều khiển **ESP32 DevKit V1 (30 chân)**.

---

## 1. Thông Số Tính Toán & Giải Thuật

- **Cảm biến PIR HC-SR501:**
  - Nguồn cấp bắt buộc: **VIN (5V)** từ cổng USB để vi mạch xử lý tín hiệu BISS0001 và chip ổn áp 7133 hoạt động ổn định.
  - Ngõ ra `OUT`: Tín hiệu logic **3.3V TTL High** khi phát hiện bức xạ hồng ngoại cơ thể sống di chuyển, mức **0V Low** khi tĩnh. Nối an toàn vào **GPIO 33**.
  - Xử lý thời gian làm nóng quang học (Warm-up Period: $10\text{s}$ ban đầu để tránh báo động giả lúc khởi động).
- **Máy trạng thái 2 chế độ (Dual-Mode FSM):**
  - **Chế độ 1: Đèn tự động tiết kiệm điện (AUTO-LIGHT - Mặc định):**
    - Khi có chuyển động: Bật đèn LED Vàng (GPIO 21) ngay lập tức và liên tục gia hạn thời gian sáng (`lastMotionTime = millis()`).
    - Khi người rời đi: Giữ sáng thêm $8\text{s}$ (Hold Time) rồi tự động tắt.
  - **Chế độ 2: Báo động chống trộm (ARMED SECURITY):**
    - Nhấn nút bấm (GPIO 4) để kích hoạt. Còi bíp 2 tiếng xác nhận ARM.
    - Khi phát hiện có người: Kích hoạt chớp đèn và còi Buzzer (GPIO 22) liên tục ($120\text{ms}$ nhịp cảnh báo).
    - Nhấn nút lần nữa để giải phóng (DISARM - Còi kêu 1 tiếng dài).

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
