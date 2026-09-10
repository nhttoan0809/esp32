# Trạng thái POC 6 — Voice Wake-Up & Cloud WebSocket Dashboard

Cập nhật: **2026-09-04**.

---

## 1. Tổng kết Trạng thái & Nghiệm thu Tính năng

- **Môi trường & Nền tảng:**
  - Vi điều khiển **ESP32 DevKit V1 (30 chân)** với kiến trúc WSS Cloud Controller kế thừa từ POC 5.
  - Backend **FastAPI + Uvicorn** phục vụ REST API & WebSocket hub tại `/api/devices/{id}/state` và `/ws/device`.
- **Tính năng Voice Wake-Up trên Web Dashboard:**
  - **Công nghệ lõi:** Sử dụng **Native Web Speech API** (`webkitSpeechRecognition` / `SpeechRecognition`) chuẩn W3C, thuần JavaScript, zero-dependency.
  - **Âm thanh phản hồi trực tiếp (Synthesized Audio Chimes):** Tích hợp **Web Audio API** (`AudioContext`) tạo âm thanh beep/chime tần số cao không cần file mp3 hay asset ngoài.
  - **Quản lý trạng thái thuần Client (100% Frontend State Machine):**
    - `inactive`: Tắt microphone, hiển thị nút bật trong header và floating badge.
    - `sleeping`: Chế độ Always-Listening nền, chờ từ khoá `"Wake Up"`. Hiệu ứng pulse xanh dương nhẹ.
    - `awake`: Thức dậy sau khi nhận `"Wake Up"`, mở cửa sổ nhận lệnh trong 8 giây (có đếm ngược badge `8s...1s` và sóng âm động).
    - `executing`: Nhận diện chỉ thị `"Change status"` (hoặc các biến thể `"change the status"`, `"toggle"`, `"turn on/off"`), phát chime xác nhận và tự động gọi API `handleToggle()` để đảo trạng thái đèn LED.
    - `error`: Xử lý mượt mà khi trình duyệt không hỗ trợ hoặc người dùng từ chối cấp quyền mic.
  - **Khôi phục kết nối ngầm (Continuous Restart):** Xử lý sự kiện `onend` tự động gọi lại `recognition.start()` đảm bảo chế độ Always-Listening không bị ngắt quãng bởi các khoảng im lặng của trình duyệt.

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
