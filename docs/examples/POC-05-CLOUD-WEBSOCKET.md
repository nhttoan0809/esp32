# Kiến trúc Mẫu: IoT Device Provisioning & Điều khiển qua WebSocket (POC 5)

Tài liệu này mô tả chi tiết kiến trúc của **POC 5** — một mô hình hoàn chỉnh cho thiết bị IoT ESP32 bao gồm: Wi-Fi Provisioning qua SoftAP/Captive Portal, kết nối Cloud an toàn qua Outbound WebSocket (WSS/TLS), và điều khiển hai chiều với máy chủ FastAPI theo mô hình `desired_state`.

---

## 1. Sơ đồ Kiến trúc Tổng thể

```text
┌────────────────────────────────┐
│   Điện thoại / Trình duyệt     │
└──────────────┬─────────────────┘
               │  1. Join SoftAP & Nhập Wi-Fi + Server Config (192.168.4.1)
               ▼
┌────────────────────────────────┐      Outbound WSS (Port 443)      ┌──────────────────────────┐
│   ESP32 DevKit V1 (Device)     │ ────────────────────────────────► │  Cloud Server (FastAPI)  │
│                                │                                   │  + ngrok HTTPS Tunnel    │
│  - SoftAP: 192.168.4.1         │ ◄──────────────────────────────── │                          │
│  - Preferences (NVS Config)    │       Command: {"action":"set"}   └─────────────┬────────────┘
│  - GPIO23 (Real_Device Relay)  │ ────────────────────────────────►               │
│  - Status LEDs (18,19,21,22)   │       ACK: {"status":"synced"}                  │
└────────────────────────────────┘                                                 │
                                                                                   │ 2. Điều khiển qua Web UI
                                                                                   ▼
                                                                     ┌──────────────────────────┐
                                                                     │   Dashboard Điều khiển   │
                                                                     │   (/dashboard REST+WS)   │
                                                                     └──────────────────────────┘
```

---

## 2. Các Module Thành phần trong Firmware ESP32

Mã nguồn tại `pocs/poc5-cloud-device/src/` được chia thành các module độc lập, rõ ràng:

1. **`app_state.h / app_state.cpp`**: Quản lý máy trạng thái tổng thể (`Boot`, `Provisioning`, `ConnectingWiFi`, `ConnectedWiFi`, `CloudReady`, `Error`).
2. **`device_controller.h / device_controller.cpp`**: Quản lý chân GPIO của thiết bị tải thực tế (`Real_Device` - GPIO23), các LED trạng thái hệ thống, và nút bấm cấu hình (GPIO25).
3. **`config_manager.h / config_manager.cpp`**: Quản lý lưu trữ cấu hình mạng Wi-Fi và Cloud Server vào bộ nhớ flash NVS (qua thư viện `Preferences`) theo cơ chế safe commit.
4. **`provisioning_manager.h / provisioning_manager.cpp`**: Khởi chạy Wi-Fi SoftAP (`ESP32-SETUP-XXXX`) và HTTP Web Server để phục vụ giao diện portal cấu hình.
5. **`cloud_client.h / cloud_client.cpp`**: Quản lý kết nối WebSocket outbound (WSS với thư viện `arduinoWebSockets`), kiểm tra TLS probe, gửi bản tin `hello`, nhận lệnh `set_state` và gửi `state_report` ACK.

---

## 3. Bản đồ Chân GPIO trong POC5

| GPIO | Vai trò | Trạng thái hiển thị |
|---:|---|---|
| **18** | LED SoftAP Portal | Sáng khi ESP32 đang mở mạng Wi-Fi cấu hình |
| **19** | LED Client Connected | Sáng khi có điện thoại/máy tính kết nối vào SoftAP |
| **21** | LED Wi-Fi Connected | Sáng khi ESP32 đã kết nối thành công vào Wi-Fi gia đình |
| **22** | LED Cloud Ready | Sáng khi kết nối WebSocket với Server hoàn tất xác thực |
| **23** | LED / Relay `Real_Device` | Trạng thái bật/tắt của thiết bị điện đầu ra |
| **25** | Nút bấm Setup / Reset | Nhấn ngắn: Mở lại SoftAP; Nhấn giữ ≥ 5 giây: Factory reset xoá NVS |

---

## 4. Backend Server (FastAPI + Uvicorn)

Thư mục `pocs/poc5-cloud-device/server/`:
- **Giao thức:** REST API cho người dùng & WebSocket endpoint `/ws/devices/{device_id}` cho ESP32.
- **Xác thực:** Bearer token per-device và API Key cho Dashboard.
- **Mô hình State:** Lưu trữ `desired_state` và `reported_state` trong RAM. Khi người dùng bật công tắc trên dashboard, server gửi lệnh xuống ESP32 qua WebSocket và chỉ phản hồi HTTP 200 sau khi ESP32 đã kích hoạt GPIO và gửi lại bản tin xác nhận (`synced: true`).

---

## 5. Quy trình Chạy & Kiểm thử POC 5

### 5.1 Khởi động Backend Server
```bash
cd pocs/poc5-cloud-device/server
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
cp .env.example .env

# Chạy server Uvicorn
uvicorn app.main:app --host 127.0.0.1 --port 8000
```

### 5.2 Mở Public Tunnel (ngrok)
```bash
ngrok http 8000
```

### 5.3 Biên dịch Firmware & Nạp lên Board
```bash
# Build firmware
pio run -d pocs/poc5-cloud-device -e esp32dev

# Flash firmware lên board thật
pio run -d pocs/poc5-cloud-device -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# Theo dõi Serial
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```
