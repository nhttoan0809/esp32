# POC: LDR Smart Light Controller (Đèn Thông Minh Với Hysteresis & Auto-Dimmer)

Bản Proof of Concept (POC) thực nghiệm ứng dụng **Module Quang trở LDR (LM393)** để tự động điều khiển đèn chiếu sáng thông minh (Dusk-to-Dawn) với thuật toán **Schmitt-Trigger Hysteresis chống chập chờn** và **Ambient Auto-Dimming** trên **ESP32 DevKit V1 (30 chân)**.

---

## 1. Thông Số Tính Toán & Giải Thuật

- **Cảm biến ánh sáng LDR LM393:**
  - `AO` (Analog Out): Điện áp liên tục tỷ lệ nghịch với cường độ sáng (Lux). Trời càng tối $\rightarrow$ điện trở LDR càng lớn $\rightarrow$ điện áp AO càng tăng tiến về $3.3\text{V}$.
  - Nối vào **GPIO 34 (ADC1_CH6)**: Chân Input-Only, thuộc ADC1 (không bị mất khi bật Wi-Fi).
  - `DO` (Digital Out): Mức logic so sánh qua IC LM393 trên board, nối vào **GPIO 35**.
- **Đèn LED chiếu sáng:**
  - Nối từ **GPIO 18** qua điện trở hạn dòng **$220\Omega$** về GND. Dòng danh định qua LED $\approx 5.91\text{ mA}$ (an toàn $< 12\text{ mA}$).
  - Điều chế xung **LEDC PWM** tần số $5\text{ kHz}$, độ phân giải 12-bit ($0 - 4095$).
- **Thuật toán Schmitt-Trigger Hysteresis (Ngưỡng trễ):**
  - Ngưỡng BẬT đèn: $ADC \ge 2800$ (Trời tối sâu).
  - Ngưỡng TẮT đèn: $ADC \le 2200$ (Trời sáng rõ ràng).
  - Khoảng đệm trễ $\Delta = 600$ đơn vị ADC ($0.48\text{V}$) loại bỏ hoàn toàn hiện tượng nhấp nháy đèn lúc hoàng hôn / bình minh hoặc khi có bóng mây che qua.
- **Ambient Auto-Dimming:** Khi đèn BẬT, độ sáng LED tự động tăng giảm mượt mà theo mức độ tối của môi trường.

---

## 2. Bản Đồ Nối Dây (Pinout Map)

| Chân ESP32 | Linh kiện | Chân linh kiện | Chức năng |
|---|---|---|---|
| **3V3** | LDR Module | `VCC` | Nguồn cấp 3.3V |
| **GND** | LDR Module | `GND` | Nối đất chung |
| **D34 (GPIO 34)** | LDR Module | `AO` | Ngõ vào analog ADC1_CH6 (0–3.3V) |
| **D35 (GPIO 35)** | LDR Module | `DO` | Ngõ vào số Digital Out |
| **D18 (GPIO 18)** | Điện trở $220\Omega$ | Đầu 1 | Ngõ ra xung PWM LEDC (5 kHz) |
| *(Nối tiếp)* | Điện trở $220\Omega$ (Đầu 2) | LED Vàng | Cực Anode (+) |
| **GND** | LED Vàng | Cực Cathode (-) | Nối đất |

---

## 3. Lệnh Thao Tác Dòng Lệnh (CLI-First)

### 3.1 Kiểm tra cú pháp sơ đồ Wokwi
```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc-ldr-smart-light/diagram.json", "utf8"))'
wokwi-cli lint pocs/poc-ldr-smart-light
```

### 3.2 Biên dịch Firmware
```bash
pio run -d pocs/poc-ldr-smart-light -e esp32dev
test -f pocs/poc-ldr-smart-light/.pio/build/esp32dev/firmware.bin && echo "BIN OK"
```

### 3.3 Chạy kiểm thử tự động với Wokwi CLI
```bash
wokwi-cli --expect-text "[SYSTEM] POC LDR Smart Light Initialized" --timeout 15000 pocs/poc-ldr-smart-light
```

### 3.4 Nạp lên Board thật
```bash
# Nạp firmware
pio run -d pocs/poc-ldr-smart-light -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# Theo dõi Serial Monitor
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```
