# AGENTS.md

## Phạm vi & Triết lý

Tài liệu này là quy chuẩn bắt buộc cho mọi tác vụ phát triển, lập trình và kiểm thử trong repository `esp32-learning`.
- **Triết lý:** **CLI-First & Agent-Friendly**. Không sử dụng hoặc phụ thuộc vào giao diện đồ hoạ (UI/Extensions) trên IDE. Mọi thao tác build, flash, lint, simulate, test phải thực hiện qua các lệnh CLI chính thức.
- **Nền tảng mục tiêu:** Vi điều khiển **ESP32 DevKit V1 (30 chân)** trên nền tảng **PlatformIO Core CLI (`pio`)** + **Arduino Framework** + **Wokwi Simulator CLI (`wokwi-cli`)**.
- **Thư viện Cốt lõi Chuẩn:**
  - **Wi-Fi Provisioning:** `tzapu/WiFiManager` (`^2.0.17`)
  - **Cloud WebSocket (WSS):** `gilmaimon/ArduinoWebsockets` (`^0.5.4`)

---

## 1. Nguyên tắc Bắt buộc

1. **Nguồn tài liệu sơ cấp:** Khi tra cứu API, chỉ sử dụng tài liệu chính thức:
   - Arduino-ESP32 Core: <https://docs.espressif.com/projects/arduino-esp32/en/latest/>
   - ESP32 Technical Reference Manual: <https://www.espressif.com/en/support/documents/technical-documents>
   - PlatformIO Core: <https://docs.platformio.org/en/latest/core/index.html>
   - WiFiManager: <https://github.com/tzapu/WiFiManager>
   - ArduinoWebsockets: <https://github.com/gilmaimon/ArduinoWebsockets>
   - Wokwi Docs & CLI: <https://docs.wokwi.com/>
2. **Không tự đoán identifier / part name:**
   - Mã board Wokwi: `board-esp32-devkit-v1` hoặc `board-esp32-devkit-c-v4`.
   - Mã linh kiện Wokwi: luôn có tiền tố `wokwi-` (ví dụ: `wokwi-led`, `wokwi-pushbutton`, `wokwi-resistor`).
3. **Cổng kiểm chứng (Verification Gate):** Không bao giờ báo thành công chỉ vì code biên dịch không lỗi (`pio run` pass). Luôn phải quan sát được log Serial thực tế qua `wokwi-cli --expect-text` hoặc `pio device monitor`.
4. **Tìm kiếm thông tin:** Khi cần search web hoặc tra cứu tài liệu ngoài, ưu tiên sử dụng Tavily CLI (`tvly`).

---

## 2. Phần cứng Chuẩn: ESP32 DevKit V1 (30 Chân)

Mọi thiết kế mạch, pin map và code phải tuân thủ nghiêm ngặt đặc tính vật lý của board 30 chân:
- **Input-Only Pins (GPIO 34, 35, 36/VP, 39/VN):** Chỉ dùng làm ngõ vào (Input / ADC1); **không** có điện trở kéo nội bộ (`INPUT_PULLUP` không hoạt động); **không thể** cấu hình làm Output.
- **Strapping Pins (GPIO 0, 2, 12, 15):** Quyết định chế độ bootloader/voltage của chip khi khởi động. Tránh dùng cho tải ngoài có thể kéo áp sai mức logic khi boot.
- **Kênh ADC khi bật Wi-Fi:** Khi Wi-Fi hoạt động, các kênh **ADC2 (GPIO 4, 0, 2, 15, 13, 12, 14, 27, 25, 26) bị vô hiệu hoá**. Bắt buộc dùng **ADC1 (GPIO 32, 33, 34, 35, 36, 39)** để đọc cảm biến Analog.
- **Cấm sử dụng GPIO 6 – 11:** Đây là các chân kết nối trực tiếp với bộ nhớ SPI Flash nội bộ; can thiệp vào sẽ gây crash ngay lập tức.
- **LED & Điện trở:** Luôn mắc nối tiếp điện trở hạn dòng **220Ω** với mỗi LED rời nối vào GPIO 3.3V.

---

## 3. Giới hạn Wokwi Simulator cần ghi nhớ

1. **Wokwi Free Tier:** Không hỗ trợ Private IoT Gateway. Không thể truy cập incoming connection (như web portal trong ESP32) từ trình duyệt máy host qua `localhost`.
2. **TLS Outbound Handshake (Issue #721):** Bắt tay HTTPS/WSS qua Wokwi Public Gateway có thể không hoàn tất. Kiểm tra luồng Wi-Fi/NTP trên Wokwi và nghiệm thu WSS/TLS cuối cùng trên **Board thật**.
3. **Wi-Fi ảo (`Wokwi-GUEST`):** Wokwi chỉ mô phỏng kết nối station ảo ra internet. Sóng vô tuyến SoftAP phát ra từ ESP32 ảo không thể nhận thấy bởi điện thoại thật.
4. **Bluetooth / BLE:** Wokwi hiện chưa hỗ trợ mô phỏng Bluetooth.

---

## 4. Danh mục Lệnh CLI Tiêu chuẩn

### 4.1 Biên dịch & Kiểm tra Tĩnh (PlatformIO)
```bash
# 1. Build firmware tại thư mục hiện tại
pio run -e esp32dev

# 2. Build firmware tại thư mục con (ví dụ POC5)
pio run -d pocs/poc5-cloud-device -e esp32dev

# 3. Xác nhận artifact tồn tại
test -f .pio/build/esp32dev/firmware.bin && echo "Firmware BIN OK"
test -f .pio/build/esp32dev/firmware.elf && echo "Firmware ELF OK"

# 4. Clean build
pio run -t clean
```

### 4.2 Nạp code & Theo dõi Serial trên Board thật
```bash
# 1. Liệt kê cổng serial trên macOS
pio device list

# 2. Nạp code (Nếu lỗi, đưa board vào Bootloader: giữ BOOT, nhấn RST/EN, thả BOOT)
pio run -d pocs/poc5-cloud-device -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# 3. Theo dõi Serial Monitor (115200 baud)
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
# Thoát: Ctrl + C hoặc Ctrl + ]

# 4. Xoá trắng Flash / NVS (Format toàn bộ cấu hình)
pio run -d pocs/poc5-cloud-device -e esp32dev -t erase --upload-port /dev/cu.usbserial-XXXX
```

### 4.3 Kiểm thử & Mô phỏng Tự động (Wokwi CLI)
```bash
# 1. Lint cú pháp diagram.json
wokwi-cli lint
wokwi-cli lint pocs/poc5-cloud-device

# 2. Kiểm thử tự động với chuỗi Serial mong đợi (Yêu cầu WOKWI_CLI_TOKEN)
wokwi-cli --expect-text "Hello ESP32!" --timeout 15000 .
```

---

## 5. Quy trình Kiểm chứng Bắt buộc (Verification Pipeline)

Mỗi thay đổi đối với codebase hoặc sơ đồ mạch phải vượt qua lần lượt các bước sau:

1. **Xác thực cú pháp `diagram.json`:**
   ```bash
   node -e 'JSON.parse(require("fs").readFileSync("diagram.json", "utf8"))'
   ```
2. **Biên dịch mã nguồn:**
   ```bash
   pio run -d pocs/poc5-cloud-device -e esp32dev
   ```
3. **Xác nhận Binary Artifact:**
   ```bash
   test -f pocs/poc5-cloud-device/.pio/build/esp32dev/firmware.elf && test -f pocs/poc5-cloud-device/.pio/build/esp32dev/firmware.bin
   ```
4. **Lint sơ đồ Wokwi:**
   ```bash
   wokwi-cli lint pocs/poc5-cloud-device
   ```
5. **Xác nhận hành vi Serial / Phần cứng:**
   - Trên Simulator: chạy `wokwi-cli --expect-text "<marker>"` để kiểm tra marker.
   - Trên Board thật: quan sát LED đổi trạng thái và thu log qua `pio device monitor`.

---

## 6. Cấu trúc Tài liệu Tham khảo

- [`docs/hardware/BOARD-ESP32-DEVKIT-V1-30PIN.md`](file:///Users/toannguyen/Documents/esp32-learning/docs/hardware/BOARD-ESP32-DEVKIT-V1-30PIN.md): Đặc tả phần cứng và sơ đồ pinout board 30 chân.
- [`docs/hardware/KIT-COMPONENTS-REFERENCE.md`](file:///Users/toannguyen/Documents/esp32-learning/docs/hardware/KIT-COMPONENTS-REFERENCE.md): Danh mục cảm biến, module và linh kiện kit thí nghiệm.
- [`docs/guides/CLI-WORKFLOW-GUIDE.md`](file:///Users/toannguyen/Documents/esp32-learning/docs/guides/CLI-WORKFLOW-GUIDE.md): Hướng dẫn chi tiết sử dụng PlatformIO và Wokwi CLI.
- [`docs/guides/HARDWARE-FLASHING-GUIDE.md`](file:///Users/toannguyen/Documents/esp32-learning/docs/guides/HARDWARE-FLASHING-GUIDE.md): Quy trình cắm nạp board thật trên macOS.
- [`docs/reference/WIFI-PROVISIONING-AND-WIFIMANAGER.md`](file:///Users/toannguyen/Documents/esp32-learning/docs/reference/WIFI-PROVISIONING-AND-WIFIMANAGER.md): Đặc tả kiến trúc Wi-Fi, Single RF PHY, Captive Portal, cấu hình và xử lý NVS.
- [`docs/reference/WEBSOCKET-CLIENT-AND-ARDUINOWEBSOCKETS.md`](file:///Users/toannguyen/Documents/esp32-learning/docs/reference/WEBSOCKET-CLIENT-AND-ARDUINOWEBSOCKETS.md): Đặc tả kiến trúc WebSocket WSS qua Cloudflare/ngrok, cấu hình TLS, Custom Headers và xử lý Pydantic Schema.
- [`docs/reference/WOKWI-SIMULATION-AND-LIMITS.md`](file:///Users/toannguyen/Documents/esp32-learning/docs/reference/WOKWI-SIMULATION-AND-LIMITS.md): Kiến trúc mô phỏng Wokwi và các giới hạn kỹ thuật.
- [`docs/reference/TROUBLESHOOTING-AND-LESSONS.md`](file:///Users/toannguyen/Documents/esp32-learning/docs/reference/TROUBLESHOOTING-AND-LESSONS.md): Sổ tay chẩn đoán sự cố và bài học kinh nghiệm toàn diện.
- [`docs/reference/WEB-ALWAYS-LISTENING-AND-VOICE-WAKEUP.md`](file:///Users/toannguyen/Documents/esp32-learning/docs/reference/WEB-ALWAYS-LISTENING-AND-VOICE-WAKEUP.md): Đặc tả kiến trúc Always-Listening, Voice Wake-Up và xử lý âm thanh Web Client bằng JavaScript.
- [`docs/examples/POC-05-CLOUD-WEBSOCKET.md`](file:///Users/toannguyen/Documents/esp32-learning/docs/examples/POC-05-CLOUD-WEBSOCKET.md): Kiến trúc mẫu dự án IoT Cloud WebSocket.
