# Sổ tay Kinh nghiệm & Xử lý sự cố Thực tế (Lessons Learned & Troubleshooting)

Tài liệu này đúc kết các bài học kinh nghiệm sâu sắc từ quá trình thiết kế, lập trình và sửa lỗi thực tế trên ESP32 trong môi trường kết hợp giữa Simulator và Phần cứng thật.

---

## 1. Bài học về Thư viện WebSockets (arduinoWebSockets)

### Hiện tượng "Nuốt lỗi SSL"
- **Triệu chứng:** Khi hàm kết nối TLS thất bại (`WiFiClientSecure::connect` trả về 0 sau timeout 5s), thư viện `WebSocketsClient` chỉ ghi log nội bộ qua macro `DEBUG_WEBSOCKETS` (mặc định bị tắt ở bản Release) và **không phát ra bất kỳ event `WStype_ERROR` nào**.
- Lập trình viên chỉ thấy event `WStype_DISCONNECTED reason="TCP connection cleanup"`, dẫn tới hiểu lầm là lỗi mất kết nối TCP thay vì lỗi bắt tay TLS.
- **Kinh nghiệm áp dụng:**
  1. Luôn chủ động viết các hàm thăm dò riêng (**Diagnostic Probes**): Gọi trực tiếp `WiFiClient::connect` (để kiểm tra TCP) và `WiFiClientSecure::connect` (để kiểm tra TLS + Root CA) trước khi trao quyền cho thư viện WebSocket.
  2. Log rõ ràng mã lỗi trả về và thời gian thực thi (`elapsed_ms`) của kết nối TLS.

---

## 2. Bài học về Bộ nhớ NVS (Non-Volatile Storage) & Cấu hình

### Cơ chế Commit an toàn (Safe Commit Pattern)
- **Vấn đề:** Nếu lưu ngay thông tin Wi-Fi / Server người dùng nhập vào NVS mà không kiểm tra, khi người dùng nhập sai mật khẩu Wi-Fi hoặc sai địa chỉ Server, ESP32 sẽ khởi động lại và vướng vào vòng lặp kết nối thất bại liên tục (Boot loop / Connection freeze).
- **Mô hình giải pháp:**
  1. Lưu cấu hình mới vào RAM dưới dạng **Pending Configuration**.
  2. Thử kết nối Wi-Fi ──► Thử lấy giờ NTP ──► Thử bắt tay TLS ──► Kết nối WebSocket thành công và nhận gói tin `ready` từ server.
  3. Chỉ khi toàn bộ chuỗi trên thành công mới gọi `Preferences.putString()` và đánh dấu `CONFIG_COMMITTED`.
  4. Nếu thất bại sau số lần thử nhất định, rollback về cấu hình cũ hoặc mở lại SoftAP portal để người dùng cấu hình lại.

---

## 3. Bài học về Quản lý Trạng thái Thiết bị (Desired State vs Reported State)

### Tránh Queue lệnh vô hạn
- **Vấn đề:** Khi thiết bị IoT bị ngắt kết nối tạm thời, nếu máy chủ lưu hàng chục lệnh bật/tắt liên tiếp vào hàng đợi (Queue), khi thiết bị online trở lại nó sẽ thực thi dồn dập toàn bộ các lệnh cũ (Replay Storm), gây chớp tắt tải và trạng thái không đoán trước được.
- **Mô hình giải pháp (Desired State Shadow):**
  - Máy chủ chỉ lưu **duy nhất 1 trạng thái mong muốn cuối cùng** (`desired_state`).
  - Khi thiết bị kết nối lại, máy chủ gửi duy nhất trạng thái này trong payload `hello_ack` / `sync`.
  - Thiết bị áp dụng GPIO rồi gửi phản hồi ACK `reported_state` để đồng bộ.

---

## 4. Bài học về Chứng chỉ TLS & Public Tunnel (ngrok)

### Tin cậy Root CA (Trust Anchor)
- **Vấn đề:** Khi public server local qua ngrok, ngrok sử dụng chứng chỉ TLS được cấp bởi Let's Encrypt (**ISRG Root X1**).
- **Lưu ý:**
  - ESP32 phải được nhúng chứng chỉ gốc `ISRG_ROOT_X1_CA_CERT` trong firmware (`WiFiClientSecure::setCACert`).
  - Cần đồng bộ thời gian thực qua NTP (`configTime(0, 0, "pool.ntp.org")`) trước khi bắt tay TLS; nếu đồng hồ hệ thống ESP32 ở năm 1970, chứng chỉ TLS sẽ bị mbedTLS từ chối do bị xem là chưa tới ngày hiệu lực (Certificate not yet valid).
  - Portal ESP32 chỉ nên nhận **Hostname** (ví dụ `example.ngrok-free.app`), không nhận kèm scheme `https://` hay cổng trong chuỗi host.

---

## 5. Bảng kiểm tra nhanh khi gặp lỗi hệ thống (Fast Checklist)

| Vấn đề | Điểm kiểm tra cốt lõi |
|---|---|
| **Build lỗi không tìm thấy header** | Kiểm tra `lib_deps` trong `platformio.ini` đã khai báo đúng tên package chưa. |
| **Serial in ký tự lạ / rác** | Kiểm tra baud rate trong code `Serial.begin(115200)` có khớp với `monitor_speed = 115200` không. |
| **ESP32 liên tục reset sau khi boot** | Kiểm tra nguồn điện có bị sụt áp khi bật Wi-Fi không; kiểm tra có chân GPIO nào bị dùng chạm vào Strapping pin (GPIO 6-11, GPIO 12) không. |
| **Không lấy được giờ NTP** | Kiểm tra router Wi-Fi có chặn cổng UDP 123 không; kiểm tra ESP32 đã nhận IP hợp lệ từ DHCP chưa. |
| **Lỗi TLS Handshake fail (code -0x2700...)** | Kiểm tra giờ hệ thống qua NTP đã đồng bộ chưa; kiểm tra nội dung PEM của Root CA có đúng định dạng kết thúc bằng newline không. |
