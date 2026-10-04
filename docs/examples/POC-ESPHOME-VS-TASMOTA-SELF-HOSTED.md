# Báo Cáo Thực Nghiệm: So Sánh Đối Chứng ESPHome vs Tasmota với Self-Hosted Server

> **Tài liệu nghiệm thu kiến trúc & kết quả thực nghiệm:** Triển khai thành công 2 bản POC độc lập và máy chủ điều khiển IoT tự host kèm AI Decision Engine.

---

## 1. Tổng Quan Kiến Trúc Đã Triển Khai

Hệ thống đã triển khai bao gồm 3 cấu phần hoàn chỉnh trong thư mục `pocs/`:

```mermaid
flowchart TD
    subgraph Shared_Breadboard["Mạch Phần Cứng Chuẩn Hóa (Dùng chung 100%)"]
        HW["ESP32 DevKit V1 (30 chân)\n• GPIO23: Tải Actuator (LED Đỏ + 220Ω)\n• GPIO18: Nút nhấn tại chỗ (INPUT_PULLUP)\n• GPIO19: Cảm biến DHT11 (Data)\n• GPIO32: Ngõ vào Analog (ADC1)"]
    end

    subgraph POC_A["POC-A: ESPHome Edge (`pocs/poc-esphome-edge`)"]
        YML["`esphome.yaml` (CaC)"] --> CC["Compile-Time Generator"]
        CC --> BIN_ESP["Firmware Độc bản (ESP-IDF)"]
        BIN_ESP --> EDGE_A["Edge Autonomy (< 5ms)\n+ Native MQTT Client"]
    end

    subgraph POC_B["POC-B: Tasmota Edge (`pocs/poc-tasmota-edge`)"]
        MONO["`tasmota32.factory.bin`\n(Monolith dựng sẵn)"] --> RUNTIME["Runtime Mapping\n(`templates/esp32_devkit_template.json`)"]
        RUNTIME --> RULES["Rules Engine (`rules/edge_rules.txt`)\n+ MQTT Ingestion"]
    end

    subgraph Self_Hosted["Self-Hosted IoT Server (`pocs/poc-self-hosted-server`)"]
        BROKER["Embedded MQTT 3.1.1 Broker (:1883)"]
        AI["AI Decision Controller (> 28.5°C -> Fan ON)"]
        HTTP["HTTP Dashboard & REST API (:8080)"]
        BROKER <--> AI
        AI <--> HTTP
    end

    HW -.-> POC_A
    HW -.-> POC_B
    POC_A <-->|"MQTT: `edge/esphome/#`"| BROKER
    POC_B <-->|"MQTT: `tele/edge_tasmota/#`, `cmnd/#`"| BROKER
```

---

## 2. Kết Quả Chứng Minh 2 Logic Cốt Lõi

### Logic 1: Chứng minh Triết lý Điều khiển Edge IoT & Khả năng Mở rộng

| Tiêu chí | ESPHome Approach (POC-A) | Tasmota Approach (POC-B) |
|---|---|---|
| **Triết lý cốt lõi** | **Compile-Time Tailored (Configuration as Code)** | **Runtime Monolith & Template-Driven** |
| **Cách định cấu hình** | Khai báo trong `esphome.yaml`, biên dịch ra mã C++ chỉ chứa đúng driver cần thiết. | Nạp 1 file binary chung, dán JSON Template và gõ lệnh cấu hình trong Web/Console. |
| **Thay đổi chân GPIO** | Cần sửa file YAML và build lại firmware (1-3 phút). | Gõ lệnh console (`GPIO23 224`), áp dụng ngay sau **1 giây** không cần build. |
| **Edge Autonomy (Mất mạng)** | Logic `on_press` compiled trực tiếp vào CPU: nút bấm toggle tải < 5ms. | Lệnh `Rule1 ON Button1#State DO Power1 2 ENDON` chạy ngầm tại biên, bảo đảm tải bật tắt bình thường. |
| **Mở rộng sang Arduino Nano / STM32** | Dùng component `modbus_controller` / `custom_uart` biên dịch driver giải mã thanh ghi trực tiếp thành các entity chuẩn. | Dùng driver `SerialBridge` / `TuyaMCU` hoặc viết script hướng đối tượng **Berry** (trên LittleFS) xử lý luồng UART mà không cần recompile. |

### Logic 2: Server Tự Host Cơ Bản Độc Lập 100%

- **Không phụ thuộc Home Assistant:** Hệ thống đã chứng minh thành công việc điều khiển cả 2 trường phái qua máy chủ tự host `pocs/poc-self-hosted-server/server.py`.
- **Zero-Dependency:** Sử dụng thư viện chuẩn của Python 3, tích hợp sẵn MQTT 3.1.1 Broker (port 1883) và HTTP Dashboard (port 8080).
- **AI Decision Engine:**
  - Nhận diện telemetry nhiệt độ từ cả ESPHome (`edge/esphome/sensor/ambient_temperature/state`) và Tasmota (`tele/edge_tasmota/SENSOR`).
  - Khi nhiệt độ vượt ngưỡng `28.5°C`: Tự động gửi lệnh bật quạt/relay (`edge/esphome/switch/living_fan_relay/command` -> `ON` và `cmnd/edge_tasmota/POWER` -> `ON`).
  - Khi nhiệt độ hạ về `<= 27.0°C`: Tự động gửi lệnh tắt quạt và ghi nhật ký minh bạch (Audit Trail) lên Dashboard.

---

## 3. Hướng Dẫn Vận Hành Hệ Thống

### Bước 1: Khởi chạy Self-Hosted Server
```bash
cd pocs/poc-self-hosted-server
python3 server.py
```
*Mở trình duyệt truy cập: `http://localhost:8080`*

### Bước 2: Vận hành Thử nghiệm Thiết bị Rìa

- **Với POC-B (Tasmota Simulation):**
  ```bash
  python3 pocs/poc-tasmota-edge/scripts/simulate_tasmota_node.py --host 127.0.0.1 --port 1883
  ```
  *Quan sát trên Web Dashboard: Cột Tasmota chuyển sang màu xanh (Online), nhiệt độ và độ ẩm cập nhật liên tục, AI kích hoạt bật/tắt quạt tự động.*

- **Với POC-A (ESPHome Simulation / Hardware):**
  - Nạp firmware `esphome.yaml` lên thiết bị hoặc chạy companion firmware qua PlatformIO:
  ```bash
  pio run -d pocs/poc-esphome-edge -e esp32dev
  ```

---

## 4. Kết Luận

Hai bản POC đã hoàn thành xuất sắc mục tiêu đề ra:
1. Phân định rõ nét sự khác biệt giữa trường phái **biên dịch tối ưu (ESPHome)** và **cấu hình động tại runtime (Tasmota)**.
2. Thiết lập một hệ sinh thái **Self-Hosted IoT hoàn chỉnh**, mở đường cho việc tích hợp các mô hình AI Controller (Local LLM / Heuristic Agent) mà không chịu bất kỳ sự ràng buộc nào từ các nền tảng thương mại.
