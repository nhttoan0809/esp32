# Kiến trúc Mẫu: IoT Device Provisioning & Điều khiển qua WebSocket (POC 5)

Tài liệu này mô tả chi tiết kiến trúc của **POC 5** — một mô hình hoàn chỉnh cho thiết bị IoT ESP32 bao gồm: Wi-Fi Provisioning qua Captive Portal với **`tzapu/WiFiManager`**, kết nối Cloud an toàn qua Outbound WebSocket (WSS/TLS) với **`gilmaimon/ArduinoWebsockets`**, và điều khiển hai chiều với máy chủ FastAPI theo mô hình `desired_state`.

---

## 1. Sơ đồ Kiến trúc Tổng thể

```text
┌────────────────────────────────┐
│   Điện thoại / Trình duyệt     │
└──────────────┬─────────────────┘
               │  1. Join SoftAP & Tự động bung Captive Portal (WiFiManager)
               ▼
┌────────────────────────────────┐      Outbound WSS (Port 443)      ┌──────────────────────────┐
│   ESP32 DevKit V1 (Device)     │ ────────────────────────────────► │  Cloud Server (FastAPI)  │
│                                │   (gilmaimon/ArduinoWebsockets)   │  + Cloudflare / ngrok    │
│  - WiFiManager (Captive Portal)│ ◄──────────────────────────────── │                          │
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

## 2. Các Thư viện Cốt lõi Tiêu chuẩn

Mã nguồn tại `pocs/poc5-cloud-device/src/` được xây dựng dựa trên 2 thư viện cốt lõi đã được chứng thực ổn định:

1. **`tzapu/WiFiManager` (`^2.0.17`)**:
   - Quản lý toàn bộ vòng đời Wi-Fi: tự động kết nối Wi-Fi đã lưu trong NVS, tự động mở SoftAP và Captive Portal khi chưa có mạng hoặc kết nối thất bại.
   - Quản lý các tham số tùy chỉnh: `Server Host` (Cloudflare/ngrok domain), `Server Port`, `WebSocket Path`.
   - Xem chi tiết: [`docs/reference/WIFI-PROVISIONING-AND-WIFIMANAGER.md`](file:///Users/toannguyen/Documents/esp32-learning/docs/reference/WIFI-PROVISIONING-AND-WIFIMANAGER.md).

2. **`gilmaimon/ArduinoWebsockets` (`^0.5.4`)**:
   - Quản lý kết nối WSS Outbound qua cổng 443 với mã hóa TLS linh hoạt (`setInsecure()` hoặc CA Cert).
   - Chuẩn hóa Header `Host: domain` (không bị dính `:443` gây lỗi ở các reverse proxy).
   - Hỗ trợ gửi `ping/pong` định kỳ giữ kết nối và xử lý bản tin JSON qua `ArduinoJson`.
   - Xem chi tiết: [`docs/reference/WEBSOCKET-CLIENT-AND-ARDUINOWEBSOCKETS.md`](file:///Users/toannguyen/Documents/esp32-learning/docs/reference/WEBSOCKET-CLIENT-AND-ARDUINOWEBSOCKETS.md).

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

### 5.2 Mở Public Tunnel (Cloudflare Tunnel hoặc ngrok)
```bash
# Lựa chọn 1: Cloudflare Tunnel (Khuyên dùng - Ổn định & Ping thấp)
cloudflared tunnel --url http://localhost:8000

# Lựa chọn 2: ngrok
ngrok http 8000
```

### 5.3 Nạp Firmware & Theo dõi Serial
```bash
# 1. Xoá NVS nếu cần đổi cấu hình mới
pio run -d pocs/poc5-cloud-device -e esp32dev -t erase --upload-port /dev/cu.usbserial-XXXX

# 2. Build & Flash firmware
pio run -d pocs/poc5-cloud-device -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# 3. Theo dõi Serial Monitor
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```
