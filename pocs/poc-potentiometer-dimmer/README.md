# POC: Potentiometer LED Dimmer (Điều Chỉnh Độ Sáng LED Bằng Biến Trở)

Bản Proof of Concept (POC) thực nghiệm ứng dụng biến trở xoay $10\text{k}\Omega$ để điều khiển độ sáng bóng đèn LED đỏ mượt mà từ tắt hẳn ($0\%$) đến sáng nhất ($100\%$) trên vi điều khiển **ESP32 DevKit V1 (30 chân)**.

---

## 1. Thông Số Tính Toán Kỹ Thuật

- **Điện áp rơi thuận của LED ($V_F$):** $\approx 2.0\text{V}$.
- **Điện trở thuần hạn dòng ($R_{limit}$):** **$220\Omega$** (chuẩn trong kit).
- **Cường độ dòng điện cực đại qua LED:**
  $$I_{LED\_max} = \frac{3.3\text{V} - 2.0\text{V}}{220\Omega} \approx \mathbf{5.91\text{ mA}}$$
  *(Nằm trong ngưỡng an toàn tuyệt đối $< 12\text{ mA}$ của GPIO ESP32).*
- **Nguồn cấp cho biến trở ($V_{in\_pot}$):** **$3.3\text{V}$ (3V3)**. Dòng tiêu thụ tĩnh: $0.33\text{ mA}$ ($330\mu\text{A}$), công suất nhiệt: $1.09\text{ mW}$.
- **Dải hoạt động của biến trở:**
  - **Dải điện trở Wiper-GND:** $0\Omega \rightarrow 10\text{k}\Omega$ ($0\% \rightarrow 100\%$ góc xoay).
  - **Dải điện áp đưa vào ADC (GPIO 34):** $0.0\text{V} \rightarrow 3.3\text{V}$.
  - **Dải điều chế xung LEDC PWM (5 kHz, 12-bit):** Duty cycle $0 \rightarrow 4095$ ($0\% \rightarrow 100\%$).
  - **Xử lý Dead-zone:** $ADC \le 35$ ép tắt ngắt hoàn toàn ($0.00\text{ mA}$); $ADC \ge 4050$ ép sáng tối đa ($5.91\text{ mA}$).

Xem tài liệu thiết kế chi tiết: [`docs/hardware/POTENTIOMETER-LED-DIMMER-SPECIFICATION.md`](../../docs/hardware/POTENTIOMETER-LED-DIMMER-SPECIFICATION.md).

---

## 2. Bản Đồ Nối Dây (Pinout)

| Chân ESP32 | Linh kiện | Chân linh kiện | Chức năng |
|---|---|---|---|
| **3V3** | Biến trở $10\text{k}\Omega$ | Chân 1 (Bìa) | Nguồn cấp 3.3V |
| **GND** | Biến trở $10\text{k}\Omega$ | Chân 3 (Bìa) | Mass chung |
| **D34 (GPIO 34)** | Biến trở $10\text{k}\Omega$ | Chân 2 (Wiper) | Ngõ vào analog ADC1_CH6 |
| **D18 (GPIO 18)** | Điện trở $220\Omega$ | Đầu 1 | Ngõ ra xung PWM LEDC (5 kHz) |
| *(Nối tiếp)* | Điện trở $220\Omega$ (Đầu 2) | LED Đỏ | Cực Anode (Chân dài) |
| **GND** | LED Đỏ | Cực Cathode (Chân ngắn) | Nối đất |

---

## 3. Lệnh Thao Tác Dòng Lệnh (CLI-First)

### 3.1 Biên dịch (Build Firmware)
```bash
pio run -d pocs/poc-potentiometer-dimmer -e esp32dev
```

### 3.2 Kiểm tra cú pháp sơ đồ Wokwi
```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc-potentiometer-dimmer/diagram.json", "utf8"))'
wokwi-cli lint pocs/poc-potentiometer-dimmer
```

### 3.3 Nạp lên Board thật (Flash & Monitor)
```bash
# Nạp code
pio run -d pocs/poc-potentiometer-dimmer -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# Theo dõi Serial Monitor (115200 baud)
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```

### 3.4 Định dạng Serial Monitor đầu ra
```text
[  1200 ms] [----------]   0.0% | ADC:  12 | Vin:0.01V | Duty:   0 | I_LED:0.00mA | [OFF 🌑]
[  2450 ms] [#####-----]  50.0% | ADC:2048 | Vin:1.65V | Duty:2048 | I_LED:2.95mA | [DIMMING]
[  4800 ms] [##########] 100.0% | ADC:4080 | Vin:3.28V | Duty:4095 | I_LED:5.91mA | [MAX 🌕]
```
