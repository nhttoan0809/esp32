# Danh mục & Thông số Linh kiện Phần cứng (Kit Starter)

Tài liệu này tổng hợp toàn bộ các module, cảm biến và linh kiện có sẵn trong bộ kit thí nghiệm ESP32 Starter để phục vụ tra cứu nhanh khi thiết kế và nối mạch.

---

## 1. Màn hình & Hiển thị

### 1.1 OLED 0.96 inch I2C (SSD1306)
- **Độ phân giải:** 128 × 64 pixels, đơn sắc (trắng hoặc vàng-xanh).
- **Điện áp hoạt động:** 3.3V – 5V (tốt nhất là 3.3V với ESP32).
- **Giao tiếp:** I2C (mặc định địa chỉ `0x3C` hoặc `0x3D`).
- **Chân nối ESP32:**
  - `VCC` → 3V3
  - `GND` → GND
  - `SCL` → GPIO 22
  - `SDA` → GPIO 21
- **Thư viện khuyên dùng:** `Adafruit SSD1306` + `Adafruit GFX Library` hoặc `U8g2`.

---

## 2. Cảm biến (Sensors)

### 2.1 Cảm biến Nhiệt độ & Độ ẩm DHT11
- **Dải đo nhiệt độ:** 0°C đến 50°C (sai số ±2°C).
- **Dải đo độ ẩm:** 20% đến 90% RH (sai số ±5%).
- **Tần số lấy mẫu:** Tối đa 1 Hz (chu kỳ đọc khuyến nghị >= 2.0s bằng `millis()`).
- **Giao thức:** Single-Bus 1-Wire (Gửi xung Start >= 18ms, nhận 40-bit dữ liệu).
- **Đặc điểm Module 3 Chân trong Kit (Khác với chip trần 4 chân):**
  - Cảm biến trong kit là **Breakout Module 3 chân** đã tích hợp sẵn điện trở kéo pull-up 10kΩ và tụ lọc nguồn (không cần gắn thêm trở ngoài).
  - Chuẩn sơ đồ chân **Kiểu A**:
    - **`S`** (Signal): Nối vào GPIO tự do (mặc định trong POC là **GPIO 19**).
    - **`+`** (VCC): Nối vào **3V3** (hoặc **VIN / 5V** nếu dùng module clone bị thiếu áp).
    - **`-`** (GND): Nối vào **GND** chung.
  - *Lưu ý mô phỏng Wokwi:* Wokwi chỉ có chip ảo `wokwi-dht22` (dạng linh kiện trần 4 chân: `VCC`, `SDA`, `NC`, `GND`). Khi đấu nối Wokwi chỉ dùng 3 chân (`VCC`, `SDA`, `GND`) và bỏ trống chân `NC`.
  - ⚠️ **Cảnh báo an toàn:** Không cắm nhầm chân `S` vào `3V3` và `+` vào GPIO vì sẽ làm chân tín hiệu bị kéo cứng vào nguồn 3.3V, khiến firmware không nhận diện được cảm biến.

### 2.2 Cảm biến Chuyển động hồng ngoại thụ động PIR HC-SR501
- **Điện áp cấp:** 5V (nối vào chân VIN của ESP32 để đủ áp cho IC BISS0001).
- **Mức logic ngõ ra (OUT):** 3.3V High / 0V Low (an toàn cho GPIO ESP32).
- **Dải phát hiện:** Góc 120°, cự ly từ 3m – 7m (điều chỉnh qua chiết áp Sx).
- **Thời gian trễ (Delay time):** 0.3s – 5 phút (điều chỉnh qua chiết áp Tx).

### 2.3 Module Cảm biến Tránh vật cản hồng ngoại (LM393)
- **Cấu tạo:** 1 LED phát hồng ngoại + 1 Photodiode thu hồng ngoại + IC so sánh LM393.
- **Ngõ ra:** Digital OUT (DO): Mức `LOW` khi phát hiện vật cản phản xạ tia hồng ngoại; mức `HIGH` khi không có vật cản.
- **Điện áp cấp:** 3.3V – 5V.

### 2.4 Module Quang trở (LDR Light Sensor with LM393)
- **Đặc điểm phần cứng thực tế trong bộ Kit:**
  - **Biến thể Module 3 Chân (Phổ biến nhất trong Kit):**
    - Ký hiệu in trên PCB từ trái sang phải: **`DO` - `GND` - `VCC`**.
    - **`DO`** (Digital Out): Tín hiệu số TTL (0V / 3.3V) từ ngõ ra IC so sánh LM393.
    - **`GND`**: Nối đất chung.
    - **`VCC`**: Cấp nguồn 3.3V (hoặc 5V).
    - **Không có chân `AO`**: Ngưỡng sáng/tối được thiết lập trực tiếp thông qua biến trở vi chỉnh (Trimpot màu xanh) trên module.
    - Mức logic `DO`: Khi đủ sáng $\rightarrow$ `LOW` (LED báo tín hiệu trên bo mạch sáng); Khi trời tối $\rightarrow$ `HIGH` (LED báo tín hiệu tắt).
  - **Biến thể Module 4 Chân:**
    - Có thêm chân **`AO`** (Analog Out): Điện áp tương tự thay đổi tỷ lệ nghịch với cường độ ánh sáng (nối vào kênh ADC1 của ESP32: GPIO 32, 33, 34, 35, 36, 39).
- **Lưu ý mô phỏng Wokwi:** Part `wokwi-photoresistor-sensor` có sẵn cả chân `AO` và `DO`. Khi mô phỏng cho module 3 chân thực tế, chỉ cần kết nối chân `DO` vào GPIO ngõ vào của ESP32.

---

## 3. Bộ truyền động & Ngõ ra (Actuators & Outputs)

### 3.1 Relay Module 2 kênh 5V (Cách ly quang Optocoupler)
- **Điện áp cuộn hút:** 5V (cấp từ VIN).
- **Tín hiệu kích:** Thường là kích mức thấp (**Active LOW** - kéo LOW sẽ đóng relay) hoặc có jumper chọn mức.
- **Tiếp điểm:** NO (Thường mở), NC (Thường đóng), COM (Chân chung).
- ⚠️ **Cảnh báo an toàn:** Chỉ dùng tiếp điểm relay để đóng cắt tải điện áp thấp an toàn (< 24V DC / pin). Tuyệt đối **không** đóng cắt điện lưới AC 220V trong phạm vi thực hành học tập.

### 3.2 Buzzer (Còi báo)
- **Active Buzzer (Còi chủ động):** Có sẵn mạch dao động bên trong; chỉ cần cấp điện áp 3.3V / kích `HIGH` là phát ra tiếng bíp cố định.
- **Passive Buzzer (Còi thụ động):** Không có dao động trong; cần cấp sóng vuông xung PWM (sử dụng hàm `ledcWriteTone()` hoặc thư viện `tone`) để phát ra các nốt nhạc / giai điệu khác nhau.

### 3.3 Đèn LED & Đèn RGB
- **LED đơn (Đỏ, Vàng, Xanh lá):** Luôn mắc nối tiếp điện trở **220Ω** vào cực dương (Anode - chân dài) trước khi vào GPIO.
- **LED RGB (Common Cathode / Cực âm chung):**
  - Chân dài nhất nối GND.
  - 3 chân còn lại (R, G, B) mỗi chân mắc qua 1 điện trở 220Ω nối vào 3 GPIO hỗ trợ PWM.

---

## 4. Linh kiện thụ động & Phụ kiện

| Linh kiện | Số lượng | Ứng dụng tiêu biểu |
|---|:---:|---|
| **Điện trở 220Ω** | 10 | Hạn dòng nối tiếp bảo vệ LED khi nối với GPIO 3.3V (dòng ~6mA an toàn) |
| **Điện trở 1kΩ / 10kΩ** | 20 | Mạch phân áp ADC, điện trở kéo lên/kéo xuống (Pull-up / Pull-down) cho nút bấm ngoài |
| **Biến trở xoay 10kΩ** | 1 | Nối chân giữa vào GPIO ADC1 để thử nghiệm đọc giá trị chiết áp 0–4095 |
| **Nút bấm 12×12mm** | 6 | Ngõ vào số; sử dụng chế độ `pinMode(pin, INPUT_PULLUP)` trong code (nút nối giữa GPIO và GND) |
| **Breadboard MB102** | 1 | Bo cắm mạch thử nghiệm không cần hàn; hai hàng rãnh giữa vừa vặn cắm ESP32 30-pin |
| **Cáp Jumper (Đực-Đực, Đực-Cái, Cái-Cái)** | 30 | Cắm kết nối giữa ESP32, breadboard và các module cảm biến |

---

## 5. Nguyên tắc an toàn chung khi ráp mạch
1. **Không bao giờ cắm tải trực tiếp vào chân ESP32:** Động cơ nhỏ, cuộn hút relay, loa công suất lớn đều phải qua transistor / MOSFET / relay trung gian và diode dập xung ngược (Flyback Diode).
2. **Kiểm tra ngắn mạch trước khi cắm USB:** Sau khi ráp linh kiện trên breadboard, hãy kiểm tra đảm bảo chân 3V3/VIN không bị chập thẳng vào GND.
3. **Cáp nạp USB:** Luôn sử dụng cáp dữ liệu (Data Cable) có đủ 4 sợi dây; cáp chỉ sạc (Charge-only) sẽ không tạo ra cổng giao tiếp serial `/dev/cu.*` trên máy tính.
