# Self-Hosted IoT Server for ESPHome & Tasmota Edge Devices

Máy chủ điều khiển IoT cục bộ, độc lập 100% với Home Assistant hoặc bất kỳ dịch vụ Cloud nào. 

## 1. Điểm Nổi Bật

- **Zero-Dependency**: Sử dụng thư viện chuẩn của Python 3 (`asyncio`, `struct`, `json`), chạy ngay lập tức mà không cần cài đặt thêm bất kỳ package nào.
- **Embedded MQTT 3.1.1 Broker**: Tự động mở cổng `1883` để giao tiếp trực tiếp với cả ESPHome và Tasmota qua mạng LAN.
- **Đa phương thức (Dual-Dialect)**:
  - ESPHome: `edge/esphome/#` (publish telemetry & subscribe switch command)
  - Tasmota: `tele/edge_tasmota/#`, `stat/edge_tasmota/#`, `cmnd/edge_tasmota/#`
- **AI Decision & Rule Controller**: Tự động đánh giá các chỉ số cảm biến (nhiệt độ > 28.5°C) và ra quyết định điều khiển thiết bị chấp hành kèm nhật ký minh bạch (Audit Trail).
- **Web Dashboard Thời gian thực**: Giao diện HTML5/Tailwind nhẹ nhàng tại cổng `8080` hiển thị song song cả 2 thiết bị và cho phép can thiệp thủ công.

---

## 2. Hướng Dẫn Khởi Chạy

```bash
cd pocs/poc-self-hosted-server

# Khởi chạy server (mặc định MQTT: 1883, HTTP: 8080)
python3 server.py

# Tuỳ biến cổng nếu cần
MQTT_PORT=1883 HTTP_PORT=8080 python3 server.py
```

Truy cập Dashboard trên trình duyệt: **`http://localhost:8080`**

---

## 3. Kiến Trúc Topic MQTT

| Hướng dữ liệu | ESPHome Topic | Tasmota Topic | Ý nghĩa |
|---|---|---|---|
| **Device -> Server** | `edge/esphome/sensor/ambient_temperature/state` | `tele/edge_tasmota/SENSOR` (JSON) | Nhiệt độ môi trường |
| **Device -> Server** | `edge/esphome/sensor/ambient_humidity/state` | `tele/edge_tasmota/SENSOR` (JSON) | Độ ẩm môi trường |
| **Device -> Server** | `edge/esphome/binary_sensor/physical_button/state` | `stat/edge_tasmota/RESULT` | Trạng thái nút bấm |
| **Device -> Server** | `edge/esphome/switch/living_fan_relay/state` | `stat/edge_tasmota/POWER` | Trạng thái Relay tải |
| **Server -> Device** | `edge/esphome/switch/living_fan_relay/command` | `cmnd/edge_tasmota/POWER` | Lệnh bật/tắt (`ON`/`OFF`) |
| **Liveness** | `edge/esphome/status` (`online`/`offline`) | `tele/edge_tasmota/LWT` (`Online`/`Offline`) | Trạng thái kết nối |
