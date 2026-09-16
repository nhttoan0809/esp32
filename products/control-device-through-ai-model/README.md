# Smart Lamp Controller qua Voice Wake-Up & Cloud Relay

Sản phẩm điều khiển **Bóng Đèn Thông Minh (Smart Lamp)** kết hợp:
1. **Nền tảng Cloud WebSocket (WSS) & WiFiManager từ POC 5:** Kết nối an toàn hai chiều, lưu cấu hình NVS Flash, hệ thống LED chỉ thị mạng (Portal, Wi-Fi, Cloud).
2. **Nhận diện giọng nói hai giai đoạn từ POC 6:** Always-Listening chạy trực tiếp trên Web Dashboard bằng Native Web Speech API & Web Audio API (100% Tiếng Anh chuẩn POC 6).
3. **Mạch đóng ngắt Relay & Nút bấm vật lý từ POC Relay AC Fan:** Relay Module 5V cách ly quang (Active-LOW), nút bấm vật lý tại chỗ với tính năng **Safety Force-OFF**, và cơ chế đồng bộ trạng thái hai chiều (Bidirectional Sync) lên Cloud.

---

## 1. Sơ Đồ Kết Nối Phần Cứng & Pinout

### 1.1 Sơ đồ đấu nối thực tế (DC & AC 220V)

```text
Phía DC (Vi điều khiển ESP32 30-Pin - Điện áp thấp an toàn):
ESP32 DevKit V1
│
├── GPIO 26 ──────────────────────────► IN1 Relay Module (Active LOW)
├── GPIO 14 ──[Lamp Button]───────────► GND (INPUT_PULLUP, nhấn = LOW)
├── GPIO 18 ──[220Ω]──[LED Vàng]─────► GND (Setup Portal)
├── GPIO 21 ──[220Ω]──[LED Xanh lá]──► GND (Wi-Fi Connected)
├── GPIO 22 ──[220Ω]──[LED Trắng]────► GND (Cloud WSS Ready)
├── VIN (5V) ─────────────────────────► VCC Relay Module
└── GND ──────────────────────────────► GND Relay Module

Phía AC 220V (Điện áp cao nguy hiểm — Làm khi ĐÃ NGẮT NGUỒN ĐIỆN):
[Ổ điện tường 220V]
│
├── Dây Pha (L) ──────────────► COM (Chân chung Relay)
│                                    ↕ (Tiếp điểm đóng/ngắt cơ khí NO)
│                               NO ───────────────► Dây 1 ──► [Bóng đèn 220V (realDevice)] ──► Dây 2
└── Dây Nguội (N) ─────────────────────────────────────────────────────────────────────────────► Dây 2
```

### 1.2 Bảng phân bổ chân (Pinout Map)

| GPIO | Thành Phần | Chế Độ | Mức Logic / Trạng Thái | Ghi Chú |
|:---:|---|:---:|---|---|
| **26** | Relay Module IN1 | OUTPUT | **Active LOW**: `LOW` = Relay Hút (Đóng COM-NO $\rightarrow$ Bật Đèn), `HIGH` = Relay Nhả (Hở COM-NO $\rightarrow$ Tắt Đèn) | Cách ly quang Optocoupler |
| **14** | Nút Bấm Đèn (Manual Lamp) | INPUT_PULLUP | Nhấn = `LOW`. Nhấn ngắn: Toggle. Giữ ≥ 3s: Force-OFF | Phím tắt Wokwi: `L` |
| **18** | LED Vàng (Setup Portal) | OUTPUT | `HIGH` = Setup Portal SoftAP đang mở | Nối tiếp trở 220Ω |
| **21** | LED Xanh Lá (Wi-Fi) | OUTPUT | `HIGH` = ESP32 STA đã kết nối Wi-Fi | Nối tiếp trở 220Ω |
| **22** | LED Trắng (Cloud WSS) | OUTPUT | `HIGH` = WSS Upgrade & Ready | Nối tiếp trở 220Ω |
| — | **realDevice** (Bóng đèn 220V) | AC Load | **Nuôi bởi tiếp điểm Relay NO** (Không nối vào GPIO ESP32) | Trong Wokwi: tải sau Relay NO |

> [!IMPORTANT]
> - **Fail-Safe tuyệt đối:** Mặc định khi khởi động hoặc khi ESP32 mất nguồn, relay luôn nhả tiếp điểm NO (`GPIO 26 = HIGH` do pullup/firmware init) $\rightarrow$ Bóng đèn 220V ngắt hoàn toàn khỏi dây pha L.
> - **Cách ly hoàn toàn:** ESP32 không bao giờ tiếp xúc trực tiếp với điện lưới 220V, chỉ điều khiển qua cuộn coil và optocoupler của relay.

---

## 2. Khẩu Lệnh Điều Khiển Giọng Nói (English Only)

Hệ thống hoạt động theo mô hình **Always-Listening State Machine** trực tiếp trong trình duyệt (Chrome/Edge):

1. **Giai đoạn 1 — Chờ Wake Word (Sleeping):**
   - Trình duyệt chạy chế độ nền, tiêu thụ ít CPU và lắng nghe từ khóa kích hoạt:
     - **Wake Word:** `"Wake Up"`, `"Hey Lamp"`, `"Smart Lamp"`
   - **Hỗ trợ lệnh trực tiếp:** `"Turn on"`, `"Turn off"`, `"Toggle"`
   - **Hỗ trợ lệnh 1 hơi:** `"Wake up turn on"`, `"Wake up turn off"`
2. **Giai đoạn 2 — Cửa sổ nhận lệnh 8 giây (Awake):**
   - Khi phát hiện từ đánh thức:
     - Phát chuông chào đón (`Chime Tone` qua Web Audio API).
     - Hiển thị badge đếm ngược 8 giây (`8s countdown`) và dải sóng âm chuyển động (`soundwaves`).
   - Các câu lệnh được hỗ trợ:
     - **Đảo trạng thái:** `"Change status"` hoặc `"Toggle"`
     - **Bật đèn:** `"Turn on"` hoặc `"Light on"`
     - **Tắt đèn:** `"Turn off"` hoặc `"Light off"`
3. **Thực thi (Executing) & Chống Lặp Lệnh:**
   - Tự động gọi API `PUT /api/devices/{id}/state` cập nhật trạng thái lên Cloud và gửi xuống ESP32.
   - Cơ chế khóa chống dội lệnh (Debounce Cooldown 1.5s).
   - Phát âm báo thành công (`Success Tone`) và tự động quay về trạng thái `Sleeping`.
   - Nếu quá 8 giây không có lệnh: phát âm báo hết giờ (`Timeout Tone`) và quay về `Sleeping`.

---

## 3. Điều Khiển Bằng Nút Bấm Vật Lý Tại Chỗ (Offline-First)

- **Nhấn ngắn (< 3s):** Đảo trạng thái đèn ngay lập tức trên phần cứng (Zero Latency, hoạt động ngay cả khi không có Wi-Fi/Internet).
- **Nhấn giữ ≥ 3s (Safety Force-OFF):** Lập tức ngắt cuộn hút relay (`GPIO 26 = HIGH`) và tắt LED chỉ thị (`GPIO 23 = LOW`), bất kể trạng thái trước đó.
- **Đồng bộ Cloud:** Khi có kết nối WebSocket, ESP32 lập tức phát bản tin `state_report` lên Cloud để Dashboard cập nhật trạng thái theo thời gian thực.

---

## 4. Hướng Dẫn Khởi Chạy Backend Server (Next.js & TypeScript)

```bash
cd products/control-device-through-ai-model/web

# 1. Cài đặt dependencies
pnpm install

# 2. Thiết lập biến môi trường (hoặc sao chép từ .env.example)
export LAMP_DASHBOARD_API_KEY='lamp-dashboard-secret'
export LAMP_DEVICE_TOKENS_JSON='{"esp32-smart-lamp":"lamp-secret-token"}'
export PORT=8000

# 3. Khởi động server phát triển (Custom Server hỗ trợ WebSocket)
pnpm dev

# Hoặc build và khởi chạy bản production
pnpm build
pnpm start
```

- Web Dashboard: `http://127.0.0.1:8000/dashboard`
- Login: `http://127.0.0.1:8000/login`
- Health: `http://127.0.0.1:8000/health`

### Public Server qua Cloudflare Tunnel:
```bash
cloudflared tunnel --url http://localhost:8000
```
Lấy tên miền `xxxx.trycloudflare.com` để nhập vào Portal Wi-Fi của ESP32 (`192.168.4.1`).

> [!TIP]
> **Khi Cloudflare Tunnel khởi động lại và sinh URL mới:**
> Chạy script tự động hoá sau để cập nhật Root CA (`tls_ca.h`), cấu hình mặc định (`secrets.h`) và nạp lại vào ESP32 trong 1 câu lệnh:
> ```bash
> python3 products/control-device-through-ai-model/scripts/update_tls_ca.py --host xxxx.trycloudflare.com --upload --upload-port /dev/cu.usbserial-0001
> ```


---

## 5. Biên Dịch Firmware & Kiểm Thử

### 5.1 Biên dịch Firmware bằng PlatformIO
Từ thư mục gốc repository:
```bash
# Build firmware
pio run -d products/control-device-through-ai-model -e esp32dev

# Kiểm tra binary artifact
test -f products/control-device-through-ai-model/.pio/build/esp32dev/firmware.bin && echo "BIN OK"
```

### 5.2 Kiểm tra sơ đồ mạch Wokwi
```bash
# Lint cú pháp JSON
node -e 'JSON.parse(require("fs").readFileSync("products/control-device-through-ai-model/diagram.json", "utf8"))' && echo "JSON OK"

# Lint sơ đồ qua wokwi-cli
wokwi-cli lint products/control-device-through-ai-model
```

### 5.3 Chạy Kiểm Thử & Kiểm Tra Cú Pháp Toàn Diện (Next.js)
```bash
cd products/control-device-through-ai-model/web

# Chạy toàn bộ Unit & Integration tests (Vitest)
pnpm test

# Kiểm tra TypeScript type checking (Build-time syntax check)
pnpm type-check

# Kiểm tra ESLint
pnpm lint

# Chạy toàn bộ pipeline kiểm chứng (type-check, lint, test, build)
pnpm check
```

### 5.4 Nạp code lên Board thật
```bash
# Liệt kê cổng serial
pio device list

# Nạp code
pio run -d products/control-device-through-ai-model -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# Theo dõi Serial Monitor
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```
