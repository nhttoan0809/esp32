# Trạng thái POC 5 — Cloud WebSocket Device & Dashboard

Cập nhật: **2026-09-02**.

---

## 1. Tổng kết Trạng thái & Nghiệm thu Thực tế

- **Môi trường & Phần cứng:**
  - Vi điều khiển **ESP32 DevKit V1 (30 chân)** đã được nạp firmware và nghiệm thu trực tiếp trên phần cứng thật qua cổng `/dev/cu.usbserial-0001`.
  - Toàn bộ 5 bóng LED chỉ báo trạng thái (GPIO 18, 19, 21, 22, 23) và nút bấm Setup (GPIO 25) hoạt động ổn định và chính xác.
- **Wi-Fi Provisioning & SoftAP Portal:**
  - Chuẩn hoá hoàn toàn sang thư viện **`tzapu/WiFiManager` (`^2.0.17`)**.
  - Giao diện Captive Portal hiện đại chuẩn **Light Theme**, tự động gom danh sách mạng Wi-Fi quét được thành dạng **Dropdown Select** trực quan, khắc phục lỗi vỡ hàng do icon và chuỗi RSSI %.
  - Xử lý sự kiện `ARDUINO_EVENT_WIFI_AP_STACONNECTED` / `STADISCONNECTED` giúp LED 19 (Client Joined) sáng/tắt chuẩn xác.
- **WebSocket Client & Cloud Server:**
  - Thư viện WebSocket: **`gilmaimon/ArduinoWebsockets` (`^0.5.4`)** kết nối Outbound WSS qua cổng 443 (Cloudflare Tunnel / ngrok) với chế độ `setInsecure()` và đồng bộ NTP.
  - Backend **FastAPI + Uvicorn** hỗ trợ mô hình `desired_state` cho cả thiết bị online và offline (tự động hồi phục trạng thái khi kết nối lại).
  - Khắc phục lỗi Schema Mismatch `set_state` giữa Server (flat `on: bool`) và Device (nested `desired.on: bool`).
- **Giao diện Quản trị Web (Light Theme):**
  - **Trang Đăng nhập (`/login`):** Xác thực Dashboard API Key, ghi nhớ `localStorage`, hỗ trợ xem/ẩn mật khẩu, thiết kế Light Theme chống zoom tự động trên iOS Safari.
  - **Trang Điều khiển (`/dashboard`):** Bảng điều khiển thiết bị thời gian thực (Auto Sync 3s), hiển thị trạng thái kết nối WSS Live (Online/Offline), nút gạt Relay GPIO 23, cảnh báo lệnh lưu trong hàng đợi và nút Đăng xuất.
- **Backend Test Suite:** Toàn bộ **13/13 tests PASSED**.
- **Dọn dẹp Mã nguồn:** Đã loại bỏ hoàn toàn các tệp dead code cũ (`provisioning_portal.*`, `wifi_manager.*`, `provisioning_page.h`), tối ưu dung lượng Flash và tăng tốc biên dịch PlatformIO.

---

## 2. Bản đồ Chân & Trạng thái LED Thực tế (ESP32 DevKit V1 30-Pin)

| GPIO | Linh kiện | Trạng thái hiển thị | Kết quả kiểm chứng |
|---:|---|---|---|
| **18** | LED Vàng (Setup) | Sáng khi ESP32 đang mở SoftAP cấu hình Wi-Fi | **PASS** (Sáng khi chưa có Wi-Fi hoặc bấm nút Setup) |
| **19** | LED Xanh dương (Client) | Sáng khi có điện thoại/máy tính kết nối vào SoftAP | **PASS** (Sáng khi điện thoại join, tắt khi rời SoftAP) |
| **21** | LED Xanh lá (Wi-Fi) | Sáng khi ESP32 đã kết nối thành công Wi-Fi gia đình | **PASS** (Sáng ổn định khi STA kết nối) |
| **22** | LED Trắng (Server WSS) | Sáng khi hoàn tất handshake `hello/ready` qua WSS | **PASS** (Sáng khi WSS authenticated với FastAPI) |
| **23** | LED Đỏ (`Real_Device`) | Sáng/tắt theo trạng thái của thiết bị tải/Relay | **PASS** (Đổi trạng thái tức thì khi gạt toggle trên Dashboard) |
| **25** | Nút bấm Setup / Reset | Nhấn ngắn: Mở lại SoftAP; Giữ ≥ 5s: Factory reset NVS | **PASS** (Đã kiểm chứng cả 2 chế độ) |

---

## 3. Danh mục Lệnh Kiểm chứng Đã Chạy

```bash
# 1. Biên dịch Firmware
pio run -d pocs/poc5-cloud-device -e esp32dev

# 2. Kiểm tra Artifact
test -f pocs/poc5-cloud-device/.pio/build/esp32dev/firmware.bin && echo "Firmware OK"

# 3. Lint Sơ đồ Wokwi
wokwi-cli lint pocs/poc5-cloud-device

# 4. Chạy Unit Test Backend Server
python3 -m pytest pocs/poc5-cloud-device/server/tests
```

**Kết quả:**
- PlatformIO Build: `RAM: 14.6%` (47,832 / 327,680 byte), `Flash: 79.7%` (1,044,901 / 1,310,720 byte).
- Backend Unit Tests: `13 passed in 0.34s`.
