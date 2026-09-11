# ESP32 IoT Learning & Prototyping (CLI-First)

Repository phát triển, thử nghiệm và xây dựng các ứng dụng IoT trên vi điều khiển **ESP32 DevKit V1 (30 chân)** theo triết lý **CLI-First** kết hợp mô phỏng kiến trúc trên **Wokwi**.

---

## 1. Điểm nổi bật & Triết lý Thiết kế

- **CLI-First Workflow (Khuyến nghị):** Không phụ thuộc vào giao diện mở rộng IDE. Mọi thao tác biên dịch, nạp code, giám sát Serial và mô phỏng đều thực thi trực tiếp qua dòng lệnh bằng **PlatformIO Core CLI (`pio`)** và **Wokwi CLI (`wokwi-cli`)**, đảm bảo tính tái lập (reproducible) và thân thiện với AI coding agent / CI-CD pipeline.
- **VS Code Extensions (Tùy chọn / Optional):** Dự án khai báo sẵn các extension khuyến nghị tại [`.vscode/extensions.json`](.vscode/extensions.json) (`platformio.platformio-ide`, `wokwi.wokwi-vscode`) dành cho các nhà phát triển muốn thao tác, debug hoặc mô phỏng trực quan thông qua giao diện đồ họa VS Code.
- **Hardware-Ready:** Chuẩn hoá thiết kế mạch và cấu hình cho board **ESP32 DevKit V1 30-pin** cùng bộ linh kiện thí nghiệm tiêu chuẩn.
- **Thư viện Cốt lõi Chuẩn:**
  - **Wi-Fi Provisioning:** Sử dụng **`tzapu/WiFiManager`** với Captive Portal tự động, menu chọn Wi-Fi Dropdown và Clean Connect.
  - **Cloud WebSocket (WSS):** Sử dụng **`gilmaimon/ArduinoWebsockets`** kết nối bảo mật qua cổng 443 với Cloudflare / ngrok.
- **Reference Architecture (POC 5):** Dự án mẫu hoàn chỉnh về IoT Provisioning qua SoftAP/Web Portal + Outbound Secure WebSocket (WSS) + Máy chủ FastAPI (Desired State pattern) + Giao diện Web Light Theme.

---

## 2. Cấu trúc Dự án

```text
esp32-learning/
├── .vscode/
│   └── extensions.json        # Khuyến nghị Extension VS Code (PlatformIO, Wokwi) - Tùy chọn
├── AGENTS.md                  # Quy tắc & Quy chuẩn bắt buộc cho AI Coding Agent
├── README.md                  # Cẩm nang tổng quan dự án
├── platformio.ini             # Cấu hình PlatformIO gốc (esp32dev, Arduino, 115200)
├── wokwi.toml                 # Cấu hình mô phỏng Wokwi
├── diagram.json               # Sơ đồ mạch Wokwi gốc (ESP32 DevKit V1 30-pin)
├── src/                       # Mã nguồn ứng dụng cơ sở / Playground
│   └── main.cpp
├── docs/                      # Hệ thống tài liệu tham khảo chung
│   ├── hardware/              # Đặc tả phần cứng board 30 chân & Danh mục linh kiện
│   │   ├── BOARD-ESP32-DEVKIT-V1-30PIN.md
│   │   ├── KIT-COMPONENTS-REFERENCE.md
│   │   ├── POTENTIOMETER-LED-DIMMER-SPECIFICATION.md
│   │   └── MINIMAL-SENSORS-POC-SPECIFICATION.md
│   ├── guides/                # Hướng dẫn thao tác dòng lệnh & Nạp board thật
│   │   ├── CLI-WORKFLOW-GUIDE.md
│   │   └── HARDWARE-FLASHING-GUIDE.md
│   ├── reference/             # Giới hạn kỹ thuật, Kiến trúc Wi-Fi, WebSocket & Sổ tay lỗi
│   │   ├── WIFI-PROVISIONING-AND-WIFIMANAGER.md
│   │   ├── WEBSOCKET-CLIENT-AND-ARDUINOWEBSOCKETS.md
│   │   ├── WOKWI-SIMULATION-AND-LIMITS.md
│   │   └── TROUBLESHOOTING-AND-LESSONS.md
│   └── examples/              # Kiến trúc dự án mẫu
│       └── POC-05-CLOUD-WEBSOCKET.md
└── pocs/
    ├── poc-potentiometer-dimmer/  # POC Điều chỉnh độ sáng LED bằng biến trở (ADC + PWM)
    ├── poc-relay-ac-fan/          # POC Điều khiển quạt AC 220V qua Relay cách ly quang
    ├── poc-ldr-smart-light/       # POC Đèn thông minh LDR với Hysteresis chống rung & Auto-Dimmer
    ├── poc-dht11-climate-monitor/ # POC Giám sát tiểu khí hậu & Cảnh báo nồm ẩm DHT11
    ├── poc-ir-obstacle-barrier/   # POC Cảm biến tiệm cận không chạm & Đếm sản phẩm IR LM393
    ├── poc-pir-motion-alarm/      # POC Đèn an ninh tự tắt & Báo động chuyển động PIR HC-SR501
    └── poc5-cloud-device/         # Dự án mẫu hoàn chỉnh (Firmware ESP32 + FastAPI Backend)
```

---

## 3. Lệnh Nhanh (Quick Cheat Sheet)

### 3.1 Biên dịch (Build)
```bash
# Build dự án gốc
pio run -e esp32dev

# Build dự án mẫu POC 5
pio run -d pocs/poc5-cloud-device -e esp32dev
```

### 3.2 Nạp code & Xem Serial (Flash & Monitor)
```bash
# Xem danh sách cổng Serial kết nối với Mac
pio device list

# Nạp code lên board thật (Giữ BOOT, nhấn EN/RST nếu cần vào bootloader)
pio run -d pocs/poc5-cloud-device -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# Mở Serial Monitor (Baud 115200, thoát bằng Ctrl + C hoặc Ctrl + ])
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200

# Xoá trắng Flash / NVS (Khi cần đổi mạng hoặc đổi Server host)
pio run -d pocs/poc5-cloud-device -e esp32dev -t erase --upload-port /dev/cu.usbserial-XXXX
```

### 3.3 Mô phỏng & Lint (Wokwi CLI)
```bash
# Lint kiểm tra tính hợp lệ của sơ đồ mạch
wokwi-cli lint
wokwi-cli lint pocs/poc5-cloud-device

# Chạy mô phỏng tự động và bắt chuỗi Serial (Cần WOKWI_CLI_TOKEN)
wokwi-cli --expect-text "Hello ESP32!" --timeout 15000 .
```

### 3.4 Sử dụng VS Code Extensions (Tùy chọn / Optional)
Nếu bạn muốn sử dụng giao diện đồ họa trên VS Code:
1. Mở thẻ **Extensions** (`Ctrl+Shift+X` hoặc `Cmd+Shift+X`), chấp nhận cài đặt các Extension khuyến nghị trong [`.vscode/extensions.json`](.vscode/extensions.json):
   - **PlatformIO IDE** (`platformio.platformio-ide`): Quản lý board, thư viện, nạp code và Serial Monitor qua thanh trạng thái dưới cùng.
   - **Wokwi Simulator** (`wokwi.wokwi-vscode`): Mở trực tiếp file `diagram.json` để kích hoạt mô phỏng đồ họa mạch ESP32.
2. **Lưu ý:** Giao diện đồ họa là tùy chọn để thuận tiện quan sát; toàn bộ tiêu chuẩn kiểm chứng cốt lõi của dự án vẫn luôn được vận hành và xác thực qua các công cụ CLI tiêu chuẩn (`pio`, `wokwi-cli`, `pytest`).

---

## 4. Hệ thống Tài liệu Tham khảo Chi tiết

- 📘 [**Sơ đồ chân ESP32 DevKit V1 30-Pin**](docs/hardware/BOARD-ESP32-DEVKIT-V1-30PIN.md): Chi tiết các chân Input-only, Strapping pins, ADC1 vs ADC2, I2C, SPI.
- 📦 [**Danh mục Linh kiện Thí nghiệm**](docs/hardware/KIT-COMPONENTS-REFERENCE.md): Thông số và cách nối OLED SSD1306, DHT11, PIR HC-SR501, Relay, Còi buzzer, v.v.
- ⚙️ [**Hướng dẫn Workflow CLI**](docs/guides/CLI-WORKFLOW-GUIDE.md): Chi tiết cách làm việc với PlatformIO Core và Wokwi CLI.
- 🔌 [**Hướng dẫn Cắm nạp Board thật (macOS)**](docs/guides/HARDWARE-FLASHING-GUIDE.md): Driver CP210x/CH34x, Bootloader sequence, cấp nguồn an toàn.
- 📶 [**Kiến trúc Wi-Fi Provisioning & WiFiManager**](docs/reference/WIFI-PROVISIONING-AND-WIFIMANAGER.md): Giới hạn Single RF PHY, quy trình Captive Portal, cấu hình tham số Server và xử lý NVS.
- 🌐 [**Kiến trúc WebSocket & ArduinoWebsockets**](docs/reference/WEBSOCKET-CLIENT-AND-ARDUINOWEBSOCKETS.md): Kết nối Outbound WSS qua Cloudflare/ngrok, cấu hình TLS, Custom Headers và xử lý Pydantic Schema.
- 🔬 [**Mô phỏng Wokwi & Giới hạn Kỹ thuật**](docs/reference/WOKWI-SIMULATION-AND-LIMITS.md): So sánh Public vs Private Gateway, TLS outbound issue #721.
- 💡 [**Sổ tay Kinh nghiệm & Xử lý sự cố**](docs/reference/TROUBLESHOOTING-AND-LESSONS.md): Bài học thực tế về RF Lock, WebSockets Header, NVS safe commit, Desired state pattern.
- 🚀 [**Kiến trúc Mẫu POC 5: Cloud WebSocket**](docs/examples/POC-05-CLOUD-WEBSOCKET.md): Tài liệu thiết kế hệ thống IoT đầy đủ.
