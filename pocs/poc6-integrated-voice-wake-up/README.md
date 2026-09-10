# POC 6 — Tích hợp Voice Wake-Up & Điều khiển Đèn ESP32 qua Cloud WebSocket

POC này nâng cấp từ kiến trúc POC 5, bổ sung tính năng **Nhận diện giọng nói hai giai đoạn (Voice Wake-Up → Command Listening)** trực tiếp trên Dashboard Web bằng JavaScript thuần (Native Web Speech API & Web Audio API).

## Tính năng Voice Control trên Dashboard

1. **Chế độ Lắng nghe Liên tục (Always-Listening):**
   - Mặc định ở trạng thái **Sleeping** (chờ từ khoá kích hoạt).
   - Khi phát hiện từ khoá **`"Wake Up"`**, hệ thống thức dậy sang trạng thái **Awake**.
2. **Cửa sổ Nhận lệnh 8 giây (Command Window):**
   - Hiển thị hiệu ứng sóng âm (soundwaves) và bộ đếm ngược trực quan (8s countdown badge).
   - Phát âm thanh phản hồi chào đón (synthesized chime qua Web Audio API).
3. **Thực thi Lệnh bằng Giọng nói:**
   - Khi nhận lệnh **`"Change status"`**, hệ thống chuyển sang trạng thái **Executing**, tự động gọi API `PUT /api/devices/{id}/state` để đảo trạng thái đèn (BẬT/TẮT).
   - Sau khi thực thi xong, tự động trở về trạng thái **Sleeping**.
   - Nếu quá 8 giây không có lệnh hợp lệ, hệ thống phát tone báo hết giờ và tự quay lại **Sleeping**.
4. **Không lưu trạng thái trên Server:**
   - 100% State Machine (Inactive, Sleeping, Awake, Executing, Error) được quản lý trong bộ nhớ trình duyệt phía Client.
   - Zero-dependency: Không cần cài đặt thư viện ngoài, chạy trực tiếp trên Chrome/Edge.

> Tài liệu nghiên cứu chi tiết: [../../docs/reference/WEB-ALWAYS-LISTENING-AND-VOICE-WAKEUP.md](../../docs/reference/WEB-ALWAYS-LISTENING-AND-VOICE-WAKEUP.md)
> Tài liệu thiết kế WebSocket: [../../docs/examples/POC-05-CLOUD-WEBSOCKET.md](../../docs/examples/POC-05-CLOUD-WEBSOCKET.md)

---

## 1. Thành phần và Bản đồ chân (ESP32 DevKit V1 30-Pin)

| GPIO | Thành phần | Ý nghĩa khi LED sáng |
|---:|---|---|
| **18** | LED Vàng | SoftAP và HTTP portal cấu hình đang hoạt động |
| **19** | LED Xanh dương | Có client (điện thoại/máy tính) đang join SoftAP |
| **21** | LED Xanh lá | ESP32 STA đã kết nối thành công vào Wi-Fi |
| **22** | LED Trắng | WSS đã upgrade và hoàn tất xác thực `hello/ready` |
| **23** | LED Đỏ (`Real_Device`) | Ngõ ra thiết bị tải đang ở trạng thái ON |
| **25** | Nút bấm Setup | Nhấn ngắn: Mở lại portal; Giữ ≥ 5s: Factory reset xoá NVS |

---

## 2. Khởi chạy Backend Server (FastAPI)

Từ thư mục `pocs/poc5-cloud-device/server`:

```bash
# 1. Tạo venv và cài đặt dependencies
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

# 2. Tạo file cấu hình môi trường
cp .env.example .env

# 3. Chạy server Uvicorn (1 worker)
export POC5_DASHBOARD_API_KEY='dashboard-secret-key'
export POC5_DEVICE_TOKENS_JSON='{"esp32-poc5":"device-secret-token"}'
uvicorn app.main:app --host 127.0.0.1 --port 8000
```

Các URL local:
- Login: `http://127.0.0.1:8000/login`
- Dashboard: `http://127.0.0.1:8000/dashboard`
- API Docs: `http://127.0.0.1:8000/docs`
- Health check: `http://127.0.0.1:8000/health`

---

## 3. Public Server qua Cloudflare Tunnel hoặc ngrok

### Lựa chọn 1: Cloudflare Tunnel (Khuyên dùng)
```bash
cloudflared tunnel --url http://localhost:8000
```
Lấy domain `.trycloudflare.com` (chỉ lấy host, ví dụ: `xxxx.trycloudflare.com`) để cấu hình vào Portal.

### Lựa chọn 2: ngrok
```bash
ngrok http 8000
```
Lấy domain ngrok (ví dụ `abcxyz.ngrok-free.app`) để nhập vào portal ESP32 (chỉ nhập host, không kèm `https://`).

---

## 4. Cấu hình & Biên dịch Firmware

### 4.1 Đồng bộ Secret
```bash
cp include/secrets.example.h include/secrets.h
```
Chỉnh sửa `include/secrets.h`:
- `DEVICE_ID`: `esp32-poc5`
- `DEVICE_TOKEN`: Khớp với token đã khai báo trong `POC5_DEVICE_TOKENS_JSON`.
- `WOKWI_PRECONFIG_ENABLED`: Đặt `false` khi nạp board thật (để mở SoftAP portal), hoặc `true` nếu muốn test nhanh trên Wokwi.

### 4.2 Biên dịch bằng PlatformIO Core CLI
Từ thư mục gốc dự án:
```bash
# Build firmware
pio run -d pocs/poc5-cloud-device -e esp32dev

# Kiểm tra file binary sau build
test -f pocs/poc5-cloud-device/.pio/build/esp32dev/firmware.bin && echo "Build Success"
```

---

## 5. Nạp Firmware lên Board thật

```bash
# 1. Kiểm tra cổng Serial
pio device list

# 2. Nạp code (Nếu lỗi, giữ nút BOOT rồi nhấn EN một lần để vào Bootloader)
pio run -d pocs/poc5-cloud-device -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# 3. Mở Serial Monitor
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```

---

## 6. Mô phỏng & Test bằng Wokwi CLI

```bash
# Lint sơ đồ mạch
wokwi-cli lint pocs/poc5-cloud-device

# Chạy mô phỏng headless và kiểm tra marker khởi động
wokwi-cli --expect-text "Đang kiểm tra Wi-Fi" --timeout 20000 pocs/poc5-cloud-device
```

---

## 7. Chạy Unit Test Backend

```bash
cd pocs/poc5-cloud-device/server
python3 -m pytest tests
```
