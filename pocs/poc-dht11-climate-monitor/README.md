# POC: DHT11 Climate Monitor & Mold Alert (Giám Sát Nhiệt Ẩm & Cảnh Báo Nồm Ẩm)

Bản Proof of Concept (POC) thực nghiệm ứng dụng **Module Cảm biến Nhiệt độ & Độ ẩm DHT11** để theo dõi tiểu khí hậu phòng và phát hiện sớm hiện tượng **nồm ẩm (H >= 75%)** hoặc **quá nhiệt (T >= 35°C)** trên **ESP32 DevKit V1 (30 chân)**.

---

## 1. Thông Số Tính Toán Kỹ Thuật

- **Cảm biến DHT11:**
  - Dải nhiệt độ: 0°C - 50°C (sai số ±2°C).
  - Dải độ ẩm: 20% - 90% RH (sai số ±5%).
  - Tần số lấy mẫu: <= 1 Hz (Firmware quy định lấy mẫu mỗi 2.0s bằng `millis()`).
  - Giao thức: Single-Bus 1-Wire trên **GPIO 19**.
  - Nguồn cấp: **3.3V** (3V3) đồng bộ mức logic.
- **Phân loại trạng thái & Cảnh báo:**
  - **Mức Lý Tưởng (Comfort):** 20°C <= T <= 30°C & 40% <= H <= 70% -> Bật **LED Xanh lá (GPIO 23)**.
  - **Cảnh báo Nồm Ẩm (Mold Alert):** H >= 75% -> Bật **LED Vàng (GPIO 25)**.
  - **Cảnh báo Quá Nhiệt / Khắc Nghiệt (Heat Alert):** T >= 35°C hoặc T <= 16°C -> Bật **LED Đỏ (GPIO 26)**.

---

## 2. Bản Đồ Nối Dây (Pinout Map)

### 2.1 Bảng đối chiếu chân Board thật vs Mô phỏng Wokwi

| Chân ESP32 | Linh kiện thực tế | Chân Module 3-Pin (Kiểu A) | Chân Wokwi (`wokwi-dht22`) | Chức năng |
|---|---|:---:|:---:|---|
| **3V3** (hoặc **VIN**) | Module DHT11 | **`+`** (VCC) | `VCC` | Nguồn cấp 3.3V (hoặc 5V từ VIN) |
| **GND** | Module DHT11 | **`-`** (GND) | `GND` | Nối đất chung |
| **D19 (GPIO 19)** | Module DHT11 | **`S`** (Signal) | `SDA` | Tín hiệu 1-Wire (Đã có sẵn trở kéo 10kΩ) |
| *Không nối* | Module DHT11 | *(Không có)* | `NC` | Chân rỗng trên chip rời (Bỏ trống) |
| **D23 (GPIO 23)** | Trở 220Ω -> LED Xanh lá | Anode (+) | `led_g:A` | Báo mức Comfort |
| **D25 (GPIO 25)** | Trở 220Ω -> LED Vàng | Anode (+) | `led_y:A` | Báo nồm ẩm (H >= 75%) |
| **D26 (GPIO 26)** | Trở 220Ω -> LED Đỏ | Anode (+) | `led_r:A` | Báo nhiệt độ khắc nghiệt |
| **GND** | 3 LED | Cathode (-) | `led:C` | Nối đất chung |

> ⚠️ **Lưu ý quan trọng khi cắm Module DHT11 thật:**
> - Module trong bộ Kit sử dụng chuẩn chân **Kiểu A: `S` (Signal ngoài cùng bên trái) — `+` (VCC ở giữa) — `-` (GND ở ngoài cùng bên phải)**.
> - **Cảnh báo lỗi:** Tuyệt đối không cắm nhầm chân `S` vào `3V3` và `+` vào GPIO. Khi đó chân tín hiệu bị kéo cứng vào nguồn điện, cảm biến không thể phát xung và firmware sẽ báo timeout `Không đọc được dữ liệu từ DHT11`.
> - Module 3 chân đã được hàn sẵn điện trở kéo pull-up 10kΩ trên bo mạch, cắm trực tiếp vào ESP32 không cần gắn thêm trở ngoài.

---

## 3. Lệnh Thao Tác Dòng Lệnh (CLI-First)

### 3.1 Kiểm tra sơ đồ Wokwi
```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc-dht11-climate-monitor/diagram.json", "utf8"))'
wokwi-cli lint pocs/poc-dht11-climate-monitor
```

### 3.2 Biên dịch Firmware
```bash
# 1. Biên dịch cho Board thật (Cảm biến DHT11 vật lý - Mặc định)
pio run -d pocs/poc-dht11-climate-monitor -e esp32dev
test -f pocs/poc-dht11-climate-monitor/.pio/build/esp32dev/firmware.bin && echo "BIN OK"

# 2. Biên dịch cho Mô phỏng Wokwi (Cảm biến ảo DHT22)
pio run -d pocs/poc-dht11-climate-monitor -e wokwi
```

### 3.3 Nạp lên Board thật
```bash
# Nạp firmware DHT11 lên board thật qua cổng USB
pio run -d pocs/poc-dht11-climate-monitor -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# Theo dõi Serial Monitor (115200 baud)
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```
