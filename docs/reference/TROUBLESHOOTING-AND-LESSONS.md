# Sổ tay Kinh nghiệm & Xử lý sự cố Thực tế (Lessons Learned & Troubleshooting)

Tài liệu này đúc kết toàn bộ các bài học kinh nghiệm sâu sắc từ quá trình thiết kế, lập trình, cấu hình mạng và sửa lỗi thực tế trên vi điều khiển **ESP32 DevKit V1 (30 chân)**.

---

## 1. Bài học về Wi-Fi: Giới hạn Single RF PHY & SoftAP Handover

### 1.1 Hiện tượng Kẹt kênh Vô tuyến (RF Channel Lock)
- **Triệu chứng:** ESP32 mở SoftAP ở Kênh 1 (`CH1`). Điện thoại kết nối vào nhập Wi-Fi nhà ở Kênh 10 (`CH10`). Khi bấm Submit, ESP32 treo cứng ở trạng thái `WIFI_CONNECTING` cho đến khi timeout mà không có log ngắt kết nối.
- **Nguyên nhân:** ESP32 chỉ có **1 bộ thu phát RF 2.4GHz duy nhất**. Khi đang giữ sóng với điện thoại ở Kênh 1, phần cứng không thể nhảy sang Kênh 10 để bắt tay WPA2 với Router.
- **Kinh nghiệm áp dụng:**
  1. Sử dụng thư viện tiêu chuẩn **`tzapu/WiFiManager`** để quản lý chu trình Provisioning.
  2. Áp dụng mô hình **Save & Clean Connect**: Đóng hoàn toàn SoftAP, giải phóng 100% công suất chip về chế độ `WIFI_STA` thuần túy trước khi bắt tay với Router.
  3. ESP32 chỉ hỗ trợ băng tần **2.4GHz (802.11 b/g/n)**, không hỗ trợ 5GHz.

### 1.2 Quản lý LED Chỉ báo Client Kết nối SoftAP (GPIO 19)
- **Triệu chứng:** Khi chuyển từ custom portal sang `WiFiManager`, LED 19 (báo có điện thoại/máy tính join SoftAP) không còn sáng khi client kết nối.
- **Nguyên nhân:** Thư viện `WiFiManager` đóng gói toàn bộ luồng mạng bên trong `wm.autoConnect()`, không tự động set chân GPIO khi có station kết nối vào SoftAP của ESP32.
- **Kinh nghiệm áp dụng:** Sử dụng hệ thống sự kiện cốt lõi của Arduino-ESP32 `WiFi.onEvent()` để lắng nghe:
  - `ARDUINO_EVENT_WIFI_AP_STACONNECTED`: Bật LED 19 (`digitalWrite(LED_CLIENT_PIN, HIGH)`).
  - `ARDUINO_EVENT_WIFI_AP_STADISCONNECTED`: Tắt LED 19 (`digitalWrite(LED_CLIENT_PIN, LOW)`).

---

## 2. Bài học về WebSocket Client: `gilmaimon/ArduinoWebsockets` vs `Links2004`

### 2.1 Lỗi Header `Host:` với Reverse Proxy (Cloudflare / ngrok)
- **Triệu chứng:** Thư viện cũ `Links2004/arduinoWebSockets` liên tục bị ngắt kết nối với mã lỗi `WSS_DISCONNECTED reason=TCP connection cleanup`.
- **Nguyên nhân:** Thư viện cũ tự ý chèn cổng `:443` vào Header HTTP Upgrade (`Host: example.com:443`). Các reverse proxy hiện đại coi đây là vi phạm định dạng và lập tức reset kết nối TCP.
- **Kinh nghiệm áp dụng:** Chuẩn hoá toàn bộ dự án sang thư viện **`gilmaimon/ArduinoWebsockets`** (hỗ trợ truyền trực tiếp URL `wss://...` và tạo Header `Host:` chuẩn RFC).

### 2.2 Sửa lỗi `setInsecure()` trên nhánh ESP32 của `ArduinoWebsockets`
- Trong file `esp32_tcp.hpp` và `websockets_client.cpp`, đảm bảo class `SecuredEsp32TcpClient` định nghĩa `void setInsecure() { this->client.setInsecure(); }` và gọi `setInsecure()` khi không truyền CA cert, giúp kết nối linh hoạt với mọi Public Tunnel (Cloudflare, ngrok).

---

## 3. Bài học về Giao thức & Bảo mật TLS/WSS

### 3.1 Chứng chỉ TLS Đa dạng giữa các Nhà cung cấp Tunnel
- **ngrok:** Thường sử dụng chứng chỉ của **Let's Encrypt** (Intermediate CA `YE2` hoặc `R10/R11`).
- **Cloudflare Tunnel (`trycloudflare.com`):** Thường sử dụng chứng chỉ của **Google Trust Services** (`WE1`).
- **Kinh nghiệm:** Trong môi trường thử nghiệm với Tunnel động, sử dụng `webSocket.setInsecure()`. Trong môi trường Production với domain cố định, nhúng trực tiếp chuỗi PEM Root CA tương ứng (`setCACert`).

### 3.2 Bắt buộc Thêm Header Bypass cho Tunnel
- Khi kết nối qua ngrok, luôn thêm Header:
  - `ngrok-skip-browser-warning: 69420`
  - `User-Agent: ESP32-Client`
  để tránh bị proxy trả về trang cảnh báo HTML Interstitial thay vì chuyển tiếp WebSocket Upgrade `HTTP 101`.

### 3.3 Đồng bộ Thời gian thực qua NTP
- Phải đồng bộ thời gian hệ thống (`configTime(0, 0, "pool.ntp.org")`) trước khi bắt tay TLS nếu có kiểm tra thời hạn chứng chỉ CA.

---

## 4. Bài học về Schema Validation (Backend & Firmware)

### 4.1 Lỗi Thiếu trường Gói tin Handshake (Pydantic Strict Mode)
- **Triệu chứng:** ESP32 kết nối WSS thành công (`WSS_UPGRADED`), gửi gói `WSS_HELLO_SENT` nhưng Server lập tức đóng socket (`WSS_DISCONNECTED`).
- **Nguyên nhân:** Backend FastAPI dùng Pydantic `StrictModel` (`extra="forbid"`). Gói tin JSON của ESP32 bị thiếu trường bắt buộc `"firmware": "poc5-cloud-device-1.0.0"`.
- **Kinh nghiệm:** Mọi cấu trúc JSON trên ESP32 C++ (ArduinoJson) phải khớp chính xác 100% từng trường dữ liệu với Schema trên Backend.

### 4.2 Lỗi Mismatch Schema `set_state` (Flat vs Nested JSON)
- **Triệu chứng:** Khi người dùng bật/tắt thiết bị trên Dashboard hoặc qua REST API `/api/devices/{id}/state`, ESP32 nhận được tin nhắn nhưng không kích hoạt GPIO 23 mà ngắt kết nối WebSocket (`webSocket_.close()`).
- **Nguyên nhân:** Backend FastAPI gửi gói lệnh phẳng `{"type": "set_state", "command_id": "...", "on": true}`, trong khi firmware ESP32 lại parse cấu trúc lồng `document["desired"]["on"]`. Khi không tìm thấy trường `desired`, firmware coi đây là lệnh không hợp lệ và ngắt socket.
- **Kinh nghiệm:** Thiết kế bộ parse JSON của Firmware linh hoạt (hỗ trợ cả `document["on"]` và `document["desired"]["on"]`) để đảm bảo tính tương thích ngược và ngăn chặn drop socket ngoài ý muốn.

---

## 5. Bài học về Quản lý Mã nguồn & Bộ nhớ Flash

### 5.1 Xoá và Cập nhật NVS khi Đổi Server Host
Khi ESP32 đã lưu thông tin Wi-Fi/Server cũ vào NVS, nó sẽ tự động kết nối và bỏ qua Portal. Áp dụng 3 phương án:
1. **Dùng lệnh CLI (Khuyên dùng):** `pio run -d pocs/poc5-cloud-device -e esp32dev -t erase --upload-port /dev/cu.usbserial-XXXX` để format sạch Flash trong 2 giây.
2. **Factory Reset Nút bấm:** Nhấn giữ nút GPIO 25 trong >= 5 giây (`wm.resetSettings()`, `configStore.clear()`).
3. **On-Demand Portal:** Nhấn ngắn nút GPIO 25 để mở lại Portal sửa Server Host mà không mất Wi-Fi.

### 5.2 Xử lý Tệp Dead Code trong PlatformIO
- **Vấn đề:** PlatformIO tự động quét và biên dịch **toàn bộ** các file `.cpp` nằm trong thư mục `src/`, bất kể file đó có được `#include` hay không.
- **Hệ quả:** Các file thử nghiệm cũ làm tăng đáng kể thời gian build và làm phình dung lượng bộ nhớ Flash của vi điều khiển.
- **Kinh nghiệm:** Luôn xóa hoặc di chuyển các file thử nghiệm không sử dụng ra ngoài thư mục `src/`.

---

## 6. Bảng Tra cứu Sự cố Nhanh (Fast Troubleshooting Matrix)

| Triệu chứng | Nguyên nhân cốt lõi | Cách xử lý |
|---|---|---|
| **`WIFI_CONNECTING` bị treo 90s** | Kẹt kênh RF giữa SoftAP (CH1) và Router (CH10) | Dùng `WiFiManager` hoặc tắt hẳn SoftAP trước khi kết nối Station |
| **LED 19 không sáng khi có client join SoftAP** | Chưa đăng ký event listener với Wi-Fi driver | Bổ sung `WiFi.onEvent()` cho `ARDUINO_EVENT_WIFI_AP_STACONNECTED` |
| **`WSS_DISCONNECTED reason=TCP connection cleanup`** | Thư viện cũ gửi sai Header `Host: domain:443` | Chuyển sang dùng `gilmaimon/ArduinoWebsockets` |
| **`WSS_CONNECT_FAILED` ngay lập tức** | `WiFiClientSecure` cố xác thực với CA rỗng | Gọi `webSocket.setInsecure()` hoặc nạp đúng CA Cert |
| **`WSS_HELLO_SENT` xong bị ngắt kết nối** | JSON thiếu trường (`firmware`, `device_id`) khiến Pydantic báo lỗi | Bổ sung đầy đủ các trường theo đúng Model trên Backend |
| **Nhấn toggle trên Dashboard làm WSS bị disconnect** | JSON Schema mismatch (`on` vs `desired.on`) | Cập nhật firmware parse cả flat `on` và nested `desired.on` |
| **Không đổi được Server Host mới** | NVS vẫn đang lưu cấu hình Server cũ | Chạy lệnh `pio run -t erase` hoặc nhấn giữ nút GPIO 25 >= 5s |
| **Serial in ký tự lạ / rác khi boot** | Baud rate không khớp | Cấu hình `Serial.begin(115200)` và `monitor_speed = 115200` |
| **ESP32 liên tục reset khi bật Wi-Fi** | Sụt áp nguồn điện (Brownout Reset) | Đổi cáp USB chất lượng cao, cấp đủ nguồn >= 500mA |
| **DHT11 timeout: `Không đọc được dữ liệu từ cảm biến`** | 1. Nhầm thứ tự chân Module 3 chân (đấu nhầm `S` vào 3V3).<br>2. Build nhầm driver DHT22 thay vì DHT11. | 1. Cắm chuẩn Kiểu A: `S` (Data) - `+` (VCC) - `-` (GND).<br>2. Cấu hình mặc định firmware là `DHT11`, chỉ dùng `DHT22` cho Wokwi qua cờ `-DWOKWI_SIMULATION`. |

---

## 7. Bài học về Cảm biến & Ngoại vi Phần cứng (Sensors & Peripherals)

### 7.1 Sự cố Module DHT11: Nhầm lẫn Chân cắm & Driver Giữa Board thật và Mô phỏng
- **Triệu chứng:** Serial Monitor in lỗi `[ERROR] Không đọc được dữ liệu từ cảm biến DHT!` hoặc `DHT timeout waiting for start signal high pulse` lặp đi lặp lại mỗi chu kỳ đo. Đo mức logic chân GPIO luôn đọc giá trị `1` (HIGH) nhưng không hề có xung phản hồi (`Pulse Transitions: 0`).
- **Nguyên nhân 1 (Phần cứng):** Đấu nhầm thứ tự chân Module 3 chân:
  - Cảm biến trong Kit thí nghiệm là **Module 3 chân chuẩn Kiểu A: `S` (Signal) — `+` (VCC) — `-` (GND)**.
  - Nếu người dùng cắm theo thói quen (tưởng chân `+` ở ngoài cùng), vô tình cắm chân `S` vào `3V3` và chân `+` vào GPIO. Khi đó chân tín hiệu DATA bị nối cứng vào nguồn 3.3V qua trở pull-up nội, cảm biến không thể kéo chân xuống mức LOW để trả lời xung Start Signal từ ESP32.
- **Nguyên nhân 2 (Phần mềm):** Code sử dụng macro phủ định `!defined(REAL_HARDWARE_DHT11)` khiến mọi lần build bình thường đều tự gán driver `DHT22`. Vì timing xung bắt tay Start Signal của DHT22 (1ms - 10ms) ngắn hơn yêu cầu tối thiểu của DHT11 (18ms), cảm biến DHT11 vật lý không được đánh thức.
- **Kinh nghiệm cốt lõi:**
  1. **Hardware-First Defaulting:** Mọi firmware phải lấy **Board thật làm mục tiêu mặc định**. Môi trường mô phỏng Wokwi phải tách thành environment riêng `[env:wokwi]` với cờ tường minh `-DWOKWI_SIMULATION`.
  2. **Quy chuẩn chân Kiểu A:** Luôn kiểm tra chữ in trên mặt mạch PCB của Module DHT11:
     - `S` -> GPIO (Data 1-Wire, đã có sẵn trở kéo 10kΩ).
     - `+` -> 3V3 (hoặc VIN 5V nếu sensor bị sụt áp).
     - `-` -> GND.
  3. **Wokwi Labels:** Luôn đặt thuộc tính `"label"` trong `diagram.json` để người dùng đối chiếu trực quan 1-1 giữa sơ đồ ảo và board mạch thật.

