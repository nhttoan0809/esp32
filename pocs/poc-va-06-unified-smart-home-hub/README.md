# POC-VA-06: Unified AI Smart Home Hub (ESP32 DevKit V1 30-Pin)

Hệ thống điều khiển và giám sát nhà thông minh toàn diện tích hợp trên **1 vi điều khiển ESP32 DevKit V1 (30 chân)** duy nhất, giao tiếp thời gian thực 2 chiều qua **WebSocket WSS (TLS)** với Web Backend AI Agent độc lập (hỗ trợ điều khiển bằng giọng nói tiếng Việt / tiếng Anh, Edge TTS, LLM Function Calling, và Dashboard trực quan).

---

## 1. Tính năng Nổi bật

1. **Khí hậu & Môi trường:** Giám sát nhiệt độ và độ ẩm phòng qua cảm biến DHT11 (GPIO 19).
2. **Chiếu sáng Thông minh Đa cấp:**
   - **Đèn Chính (Main Lamp):** Điều khiển BẬT/TẮT qua Relay cách ly quang (GPIO 5, Active LOW).
   - **Đèn Tùy biến Độ sáng (Dimmer LED):** Điều chỉnh mịn 0% – 100% bằng phần cứng LEDC PWM (GPIO 18, 5kHz 8-bit).
   - **Núm xoay Chiết áp (Potentiometer):** Điều chỉnh độ sáng thủ công tại chỗ qua kênh ADC1 (GPIO 34) không bị xung đột khi bật Wi-Fi.
   - **Cảm biến Quang trở (LDR):** Tự động nhận diện trạng thái ngày/đêm (GPIO 35, Bright/Dark).
3. **An ninh & Cảnh báo Xâm nhập:**
   - **Cảm biến Chuyển động (PIR):** Phát hiện người di chuyển (GPIO 33).
   - **Chế độ Bảo vệ (Guard Mode):** Kích hoạt còi báo động Buzzer (GPIO 27) khi phát hiện người lạ.
   - **Chế độ Tiết kiệm (Eco Mode):** Tắt còi, chỉ gửi telemetry báo cáo chuyển động về Web.
4. **Màn hình Trực quan Đa giao diện (OLED SSD1306 0.96" I2C):**
   - 4 trang hiển thị luân phiên: **Climate** (Nhiệt ẩm) ➔ **Lighting** (Trạng thái đèn & Dimmer) ➔ **Security** (An ninh PIR/Còi) ➔ **System** (IP, Wi-Fi RSSI, Cloud Status).
   - Chuyển trang tức thì bằng Nút nhấn Mode (Button 1, GPIO 4).
5. **Điều khiển Kép Linh hoạt (Dual-Control):**
   - **Thủ công tại chỗ:** Nút nhấn Button 2 (GPIO 14) bật/tắt Relay; Chiết áp (GPIO 34) chỉnh Dimmer.
   - **Từ xa qua Web & Giọng nói:** Dashboard Next.js, Web Speech API Always-Listening ("Wake Up"), Edge TTS phát âm phản hồi tiếng Việt tự nhiên.

---

## 2. Bảng Phân Bổ Chân Phần Cứng (14-Pin Map) Tuân Thủ Rule 7

Toàn bộ 14 chân GPIO đã được kiểm chứng không xung đột phần cứng với bộ nhớ SPI Flash nội bộ (GPIO 6–11) và tương thích hoàn toàn khi Wi-Fi kích hoạt (sử dụng ADC1, không dùng ADC2):

| STT | Chân ESP32 (30-Pin) | Ký hiệu in trên Bo Mạch Module (Physical PCB Label) | Chân Wokwi (`diagram.json`) | Chức năng Kỹ thuật & Lưu ý An toàn |
|:---:|:-------------------:|:--------------------------------------------------:|:---------------------------:|:-----------------------------------|
| 1 | **GPIO 4** | Nút nhấn / Chân 1.L | `btn_mode:1.l` | **Nút nhấn Mode:** Nhấn ngắn chuyển 4 trang OLED; Nhấn giữ 5s để **Factory Reset** xóa Wi-Fi/NVS. Cấu hình `INPUT_PULLUP`. |
| 2 | **GPIO 5** | `IN` / `IN1` (Relay Module) | `relay1:IN` | **Relay Ngắt Nguồn Cực Âm (Ground Cutoff - Phương án A):** Tiếp điểm `COM` nối vào Cathode (-) của Đèn, tiếp điểm `NO` nối `GND`. Khi Relay TẮT, mạch hở hoàn toàn (0W standby, an toàn điện tuyệt đối). |
| 3 | **GPIO 13** | Anode (`+`) LED Đỏ | `r_alert:2` ➔ `led_alert:A` | **LED Báo Động (Đỏ):** Sáng khi có cảnh báo/chuyển động. Nối tiếp điện trở 220Ω. |
| 4 | **GPIO 14** | Nút nhấn / Chân 1.R | `btn_lamp:1.r` | **Nút nhấn Đèn:** Đảo trạng thái Bật/Tắt Đèn Thông Minh tại chỗ (khôi phục mức sáng đã nhớ). Cấu hình `INPUT_PULLUP`. |
| 5 | **GPIO 15** | Anode (`+`) LED Xanh Lá | `r_comfort:2` ➔ `led_comfort:A` | **LED Trạng thái Khí hậu (Xanh lá):** Báo phòng mát mẻ, dễ chịu. Điện trở 220Ω. |
| 6 | **GPIO 18** | Anode (`+`) Smart Lamp Vàng | `r_dimmer:2` ➔ `led_dimmer:A` | **Đèn Thông Minh (PWM 0-100%):** Xuất xung Active-HIGH qua trở 220Ω vào Anode (+), cực Cathode (-) đi qua tiếp điểm Relay về GND. |
| 7 | **GPIO 19** | `DAT` / `OUT` / `S` (Module DHT11) | `dht1:SDA` (Wokwi DHT22) | **Cảm biến Nhiệt Ẩm DHT11:** Giao tiếp 1-Wire. Mạch đã có sẵn trở kéo 10kΩ trên PCB. |
| 8 | **GPIO 21** | `SDA` (OLED SSD1306) | `oled1:SDA` | **Dữ liệu I2C SDA:** Hiển thị màn hình OLED 128x64. |
| 9 | **GPIO 22** | `SCL` (OLED SSD1306) | `oled1:SCL` | **Xung nhịp I2C SCL:** Tần số chuẩn 400kHz. |
| 10 | **GPIO 23** | Anode (`+`) LED Xanh Dương | `r_cloud:2` ➔ `led_cloud:A` | **LED Cloud Online (Xanh dương):** Sáng khi kết nối WebSocket thành công. |
| 11 | **GPIO 27** | `+` (Buzzer Module) | `bz1:1` | **Còi Báo Động (Buzzer):** Kêu ngắt quãng khi phát hiện xâm nhập trong Guard Mode. |
| 12 | **GPIO 33** | `OUT` (PIR Motion Module) | `pir1:OUT` | **Cảm biến Chuyển động:** Mức HIGH (3.3V) khi có người di chuyển. |
| 13 | **GPIO 34** | `SIG` / Chân Giữa Chiết Áp 10kΩ | `pot1:SIG` | **Chiết áp Dimmer (ADC1_CH6):** Input-only, đọc áp Analog 0-3.3V an toàn khi bật Wi-Fi. |
| 14 | **GPIO 35** | `DO` (Digital Output Module LDR) | `ldr1:DO` | **Cảm biến Ánh sáng LDR:** Input-only, mức HIGH khi trời tối (Dark), LOW khi sáng (Bright). |
| 15 | **3V3** | `VCC` / `+` | `3V3` rail | **Nguồn cấp 3.3V:** Cấp cho DHT11, OLED, PIR, LDR, Potentiometer. |
| 16 | **VIN (5V)** | `VCC` (Relay, Buzzer) | `5V` rail | **Nguồn cấp 5V:** Cấp cho cuộn hút Relay và Còi buzzer. |
| 17 | **GND** | `GND` / `-` | `GND` rail | **Điểm nối đất chung (Common Ground).** |

### 2.1 Kiến trúc Đèn Thông Minh Hợp Nhất (Phương Án A: Relay Ground Cutoff + PWM)

Phương án A giải quyết bài toán: **Vừa đóng/ngắt nguồn an toàn tuyệt đối bằng Relay, vừa tinh chỉnh độ sáng 0% – 100% bằng PWM và Chiết áp**:

```text
  [ESP32: GPIO 18 (PWM)] ──► [Trở 220Ω] ──► [Anode (+)] LED [Cathode (-)] ──► [Relay: COM]
     (Xung PWM Active-HIGH                                                          │
      LEDC 5kHz 0-100%)                                  [Relay: NO] ───────────────┴──► [ESP32: GND]
```

- **Khi Đèn TẮT (OFF):** Tiếp điểm `COM` hở mạch với `NO` $\to$ Cực Cathode bị ngắt hoàn toàn khỏi GND $\to$ Dòng điện qua đèn bằng 0 tuyệt đối (**0W standby, an toàn điện chuẩn công nghiệp**).
- **Khi Đèn BẬT (ON):** Relay đóng tiếp điểm `COM` thông sang `NO` $\to$ Cực Cathode được nối đất (GND), xung PWM từ GPIO 18 điều chế độ sáng mượt mà.
- **Bộ nhớ độ sáng (Memory Brightness):** Khi bật lại bằng nút bấm, giọng nói AI hoặc Web, đèn tự động khôi phục mức sáng gần nhất (mặc định 70%).
- **Xoay Chiết áp (Potentiometer):** Xoay từ 0% lên >0% tự động đóng Relay và xuất độ sáng tương ứng; vặn về 0% tự động ngắt tiếp điểm Relay.

---

## 3. Kiến trúc Phần mềm

```
esp32-learning/pocs/poc-va-06-unified-smart-home-hub/
├── diagram.json                 # Sơ đồ mạch Wokwi có đầy đủ Visual Labels (Rule 6)
├── platformio.ini               # Cấu hình Dual-target ([env:esp32dev] & [env:wokwi])
├── wokwi.toml                   # Cấu hình nạp firmware mô phỏng
├── include/
│   ├── app_config.h             # Định nghĩa 14 chân GPIO, cấu hình PWM, ngưỡng cảm biến
│   ├── device_config.h          # Cấu hình tên thiết bị, timeout, chu kỳ telemetry
│   ├── secrets.h                # Thông tin Wi-Fi, URL WebSocket, Token xác thực
│   ├── secrets.example.h        # File mẫu cấu hình bí mật
│   └── tls_ca.h                 # Chứng chỉ gốc Root CA TLS Let's Encrypt / Cloudflare
├── src/
│   ├── main.cpp                 # Luồng chính FreeRTOS quản lý 14 chân & điều phối ngoại vi
│   ├── cloud_client.h / .cpp    # WebSocket client WSS nhận diện lệnh và gửi telemetry
│   └── display_manager.h / .cpp # Giao diện 4 trang OLED đa chức năng
└── web/                         # Web Backend độc lập (Port 8006, Next.js, AI Agent, TTS)
    ├── server.ts                # Custom HTTP + WebSocket Server
    ├── src/lib/types.ts         # Zod Schema: telemetry, brightness, security mode
    ├── src/lib/registry.ts      # Quản lý phiên kết nối WebSocket ESP32 và đồng bộ trạng thái
    ├── src/lib/ai/
    │   ├── device-tools.ts      # Tool Function Calling: setLedBrightness, setSecurityMode...
    │   └── smart-home-agent.ts  # System Prompt suy luận ngữ cảnh đa cảm biến
    └── src/components/
        └── DeviceCard.tsx       # Giao diện thẻ Hub: hiển thị khí hậu, ánh sáng, chuyển động, thanh trượt dimmer
```

---

## 4. Kịch bản Giao tiếp Bằng Giọng nói (Voice Interaction Examples)

Hệ thống hỗ trợ cả tiếng Việt và tiếng Anh:

1. **Hỏi Khí Hậu & Môi Trường:**
   - **User:** *"Nhiệt độ phòng hiện tại thế nào?"*
   - **AI Agent:** Gọi tool `getSensorData` ➔ Trả lời: *"Nhiệt độ phòng hiện tại là 26.5°C, độ ẩm 62%, không khí rất mát mẻ và dễ chịu."*
2. **Điều Khiển Đèn Chính (Relay):**
   - **User:** *"Bật đèn phòng khách lên"*
   - **AI Agent:** Gọi tool `setDeviceState({ on: true })` ➔ ESP32 kéo GPIO 5 xuống mức LOW ➔ Relay đóng đèn sáng.
3. **Điều Khiển Chiết Áp / Độ Sáng Đèn Dimmer:**
   - **User:** *"Đặt độ sáng đèn thành 75 phần trăm"*
   - **AI Agent:** Gọi tool `setLedBrightness({ brightness: 75 })` ➔ ESP32 điều chế LEDC PWM GPIO 18 đạt chu kỳ công tác 75%.
4. **Kích Hoạt An Ninh Chống Trộm:**
   - **User:** *"Bật chế độ bảo vệ an ninh"*
   - **AI Agent:** Gọi tool `setSecurityMode({ mode: "guard" })` ➔ Khi PIR (GPIO 33) phát hiện chuyển động ➔ Buzzer (GPIO 27) kêu còi báo động, LED Đỏ (GPIO 13) nhấp nháy liên tục.
5. **Chuyển sang Chế độ Tiết kiệm:**
   - **User:** *"Chuyển sang eco mode"*
   - **AI Agent:** Gọi tool `setSecurityMode({ mode: "eco" })` ➔ Tắt còi, hệ thống chuyển về chế độ theo dõi nhẹ nhàng.

---

## 5. Hướng Dẫn Biên Dịch & Chạy Thử Nghiệm

### 5.1 Kiểm tra và Biên dịch Firmware
```bash
# 1. Kiểm tra cú pháp sơ đồ Wokwi
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc-va-06-unified-smart-home-hub/diagram.json", "utf8")); console.log("JSON OK");'

# 2. Biên dịch cho Board thật (ESP32 DevKit V1 30-pin, dùng DHT11 thật)
pio run -d pocs/poc-va-06-unified-smart-home-hub -e esp32dev

# 3. Biên dịch cho Mô phỏng Wokwi (Dùng driver Wokwi DHT22)
pio run -d pocs/poc-va-06-unified-smart-home-hub -e wokwi

# 4. Lint cấu hình diagram Wokwi
wokwi-cli lint pocs/poc-va-06-unified-smart-home-hub
```

### 5.2 Khởi động Web Backend Độc lập
```bash
cd pocs/poc-va-06-unified-smart-home-hub/web

# Chạy test suite (17 test files, 87 unit tests)
pnpm test

# Kiểm tra kiểu TypeScript
pnpm type-check

# Khởi động server (Port 8006)
pnpm dev
```
Truy cập trình duyệt tại: `http://localhost:8006/dashboard`.
Mã API Key mặc định: `hub-secret-token`.

### 5.3 Nạp Firmware Lên Board Thật
```bash
# Tìm cổng Serial trên macOS
pio device list

# Nạp code
pio run -d pocs/poc-va-06-unified-smart-home-hub -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# Mở Serial Monitor theo dõi log
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```
