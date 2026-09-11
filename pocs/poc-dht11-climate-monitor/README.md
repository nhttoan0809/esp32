# POC: DHT11 Climate Monitor & Mold Alert (Giám Sát Nhiệt Ẩm & Cảnh Báo Nồm Ẩm)

Bản Proof of Concept (POC) thực nghiệm ứng dụng **Module Cảm biến Nhiệt độ & Độ ẩm DHT11** để theo dõi tiểu khí hậu phòng và phát hiện sớm hiện tượng **nồm ẩm ($H \ge 75\%$)** hoặc **quá nhiệt ($T \ge 35^\circ\text{C}$)** trên **ESP32 DevKit V1 (30 chân)**.

---

## 1. Thông Số Tính Toán Kỹ Thuật

- **Cảm biến DHT11:**
  - Dải nhiệt độ: $0^\circ\text{C} - 50^\circ\text{C}$ (sai số $\pm 2^\circ\text{C}$).
  - Dải độ ẩm: $20\% - 90\%\text{ RH}$ (sai số $\pm 5\%$).
  - Tần số lấy mẫu: $\le 1\text{ Hz}$ (Firmware quy định lấy mẫu mỗi $2.0\text{s}$ bằng `millis()`).
  - Giao thức: Single-Bus 1-Wire trên **GPIO 19**.
  - Nguồn cấp: **3.3V** (3V3) đồng bộ mức logic.
- **Phân loại trạng thái & Cảnh báo:**
  - **Mức Lý Tưởng (Comfort):** $20^\circ\text{C} \le T \le 30^\circ\text{C}$ & $40\% \le H \le 70\%$ $\rightarrow$ Bật **LED Xanh lá (GPIO 23)**.
  - **Cảnh báo Nồm Ẩm (Mold Alert):** $H \ge 75\%$ $\rightarrow$ Bật **LED Vàng (GPIO 25)**.
  - **Cảnh báo Quá Nhiệt / Khắc Nghiệt (Heat Alert):** $T \ge 35^\circ\text{C}$ hoặc $T \le 16^\circ\text{C}$ $\rightarrow$ Bật **LED Đỏ (GPIO 26)**.

---

## 2. Bản Đồ Nối Dây (Pinout Map)

| Chân ESP32 | Linh kiện | Chân linh kiện | Chức năng |
|---|---|---|---|
| **3V3** | Module DHT11 | `VCC` | Nguồn cấp 3.3V |
| **GND** | Module DHT11 | `GND` | Nối đất |
| **D19 (GPIO 19)** | Module DHT11 | `DATA` | Tín hiệu 1-Wire |
| **D23 (GPIO 23)** | Điện trở $220\Omega$ $\rightarrow$ LED Xanh lá (A) | Anode (+) | Báo mức Comfort |
| **D25 (GPIO 25)** | Điện trở $220\Omega$ $\rightarrow$ LED Vàng (A) | Anode (+) | Báo nồm ẩm ($H \ge 75\%$) |
| **D26 (GPIO 26)** | Điện trở $220\Omega$ $\rightarrow$ LED Đỏ (A) | Anode (+) | Báo nhiệt độ khắc nghiệt |
| **GND** | 3 LED | Cathode (-) | Nối đất chung |

---

## 3. Lệnh Thao Tác Dòng Lệnh (CLI-First)

### 3.1 Kiểm tra sơ đồ Wokwi
```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc-dht11-climate-monitor/diagram.json", "utf8"))'
wokwi-cli lint pocs/poc-dht11-climate-monitor
```

### 3.2 Biên dịch Firmware
```bash
pio run -d pocs/poc-dht11-climate-monitor -e esp32dev
test -f pocs/poc-dht11-climate-monitor/.pio/build/esp32dev/firmware.bin && echo "BIN OK"
```

### 3.3 Nạp lên Board thật
```bash
# Đối với board thật, định nghĩa cờ REAL_HARDWARE_DHT11 nếu dùng DHT11 thay cho DHT22
pio run -d pocs/poc-dht11-climate-monitor -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# Theo dõi Serial Monitor
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```
