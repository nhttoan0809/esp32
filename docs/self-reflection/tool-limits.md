# Tool limits register (giới hạn của công cụ bên ngoài)

Các hạn chế đến từ công cụ bên ngoài (Wokwi, ngrok, PlatformIO, ESP32
Arduino core, WebSockets lib...). Không do code của chúng ta gây nên, nhưng
cần ghi để (1) không phí thời gian cố "sửa" bằng code, (2) biết khi nào
phải đổi công cụ/phương pháp.

Cấu trúc như `mistakes.md`: mục đánh số, mỗi mục có `desc`, `root-cause`,
`pot-sol`, `selected-choice`.

---

## 1. Wokwi free không có Private IoT Gateway (2026-08-23)

### desc

Trên Wokwi tài khoản **free**, không có cách nào "Enable Private Gateway"
trong VS Code extension. Free tier chỉ có Public Gateway (chạy remote trên
cloud) + Virtual WiFi. Không thể điều khiển SoftAP portal từ trình duyệt,
cũng không có incoming connection. Private IoT Gateway (chạy local, có
incoming + TLS đầy đủ) thuộc bản **trả phí** (~€5.6/tháng).

### root-cause

Wokwi giới hạn tính năng gateway theo tier tài khoản (bảng giá chính thức:
Public Gateway = "All users"; Private IoT Gateway = "Paying user"). Đây là
quyết định mô hình kinh doanh của Wokwi, không phải bug.

### pot-sol

- (a) Nâng Wokwi lên bản trả phí để có Private Gateway.
- (b) **Bỏ qua portal** trong Wokwi bằng firmware preconfig (nối thẳng STA
  `Wokwi-GUEST` + host ngrok, skip SoftAP), dùng khi NVS trống.
- (c) Chờ Wokwi mở tính năng này cho free (không kiểm soát được).

### selected-choice

Chọn (b): thêm `WOKWI_PRECONFIG_ENABLED` + `WOKWI_PRECONFIG_*` trong
`secrets.h`, firmware nối thẳng không qua portal khi NVS trống. Đã build
thành công. Giữ (a) làm dự phòng nếu sau này cần incoming/local gateway.

---

## 2. Wokwi public gateway: bắt tay TLS outbound không hoàn tất (2026-08-23)

### desc

ESP32 mô phỏng (Wokwi) có thể **TCP thường** tới `ngrok:443` được
(`TCP_PROBE_OK`) và ra internet được bằng UDP (`TIME_SYNCED`), nhưng
**bắt tay TLS (HTTPS/WebSocket-secure) không hoàn tất**:
`TLS_PROBE_FAILED code=0 elapsed_ms=34148` dù cho ngân sách 20s. Kết quả:
không bao giờ có `WSS_UPGRADED`, đèn server (GPIO22) không sáng. Wokwi đã
ghi nhận dạng bug này (issue #721 "HTTPS / TLS requests not completing",
được đóng).

### root-cause

Wokwi Public Gateway (free, chạy remote trên cloud) có khả năng TLS outbound
bị hạn chế/không ổn định (khớp bug #721; docs ghi "Speed: Slower, Stability:
Medium"). TCP layer hoạt động nhưng TLS layer không hoàn tất trong
simulator. Không do firmware (CA thật, host đúng, protocol đúng — đã verify
phía server/ngrok/openssl đều OK).

### pot-sol

- (a) Nâng Wokwi Private Gateway (trả phí) → TLS đầy đủ, WSS xong trong
  simulator.
- (b) Flash **board ESP32 thật** + Wi-Fi thật → WSS/ngrok hoạt động, không
  qua gateway Wokwi.
- (c) Dùng chế độ **WS thường (không TLS)** chỉ cho Wokwi → đèn sáng được
  trong free tier nhưng mất TLS (đổi bản chất POC).
- (d) Tăng `WEBSOCKETS_TCP_TIMEOUT` (bản thân lib default 5s) — **đã loại**,
  vì probe TLS 20s vẫn fail nên không phải do timeout.

### selected-choice

Theo quyết định của người dùng (2026-08-23): **dừng ở kết luận** — bước
WSS-TLS không xác nhận trong Wokwi free, POC chưa nghiệm thu end-to-end.
Các hướng (a)/(b)/(c) ghi trong `STATUS.md` để chọn khi cần "sáng đèn"
thật. (b) khuyến nghị (miễn phí, đúng thiết kế nhất).

---

## 3. WebSockets lib nuốt lỗi SSL, không phát `WStype_ERROR` (2026-08-23)

### desc

Khi `connect(host, port, 5000ms)` (TCP+TLS gộp, timeout cứng 5s của thư
viện `WebSockets`) trả 0, lib chỉ log nội bộ (DEBUG, bị tắt ở release) và
**không phát sự kiện `WStype_ERROR` nào**. Log chỉ thấy
`WStype_DISCONNECTED reason="TCP connection cleanup"` (dòng 636,
`clientIsConnected`) — đây là thông báo *dọn dẹp*, không phải *nguyên
nhân*. Nên ban đầu "mù" vì không biết TLS chết vì lý do gì.

### root-cause

Thiết kế của thư viện `WebSocketsClient`: `connectFailedCb()` chỉ
`DEBUG_WEBSOCKETS(...)`; failure của `WiFiClientSecure::connect` không được
map thành event gửi lên callback của user. Muốn biết lý do thật phải tự viết
probe (gọi `WiFiClientSecure::connect` trực tiếp + log mã lỗi & thời gian).

### pot-sol

- (a) Thêm **probe TLS** riêng (đúng CA nhúng) để log `code` và
  `elapsed_ms` → phân biệt "chậm" vs "bị từ chối" vs "trễ".
- (b) Bật `DEBUG_WEBSOCKETS` (compile option) để thấy log nội bộ của lib.
- (c) Chuyển sang thư viện WebSocket khác có callback lỗi rõ ràng
  (VD `esp32websockets`/mbedTLS trực tiếp).

### selected-choice

Chọn (a): thêm `probeTcpConnectivity()` + `probeTlsConnectivity()` trong
`cloud_client.cpp` (dùng `WiFiClient`/`WiFiClientSecure`), log
`TCP_PROBE_*`/`TLS_PROBE_*`. Đây là cách có bằng chứng nhất, giữ được
architecture hiện tại.

---

## 4. `Wokwi-GUEST` Wi-Fi không phải mạng thật, không thấy từ điện thoại (2026-08-23)

### desc

Trong Wokwi, ESP32 mô phỏng nối STA vào `Wokwi-GUEST` (pass rỗng). Người
dùng không thấy mạng này từ điện thoại thật, và cũng không có
`ESP32-SETUP-000111` (SoftAP) để join. Đây là hành vi **đúng** (by design),
không phải lỗi.

### root-cause

Wokwi mô phỏng **radio/Wi-Fi nội bộ của chip** (STA join network ảo + ra
internet qua gateway). SoftAP do ESP32 phát ra **không phải mạng radio thật**,
nên thiết bị vật lý (điện thoại) không thấy. Muốn SoftAP thật + join bằng
điện thoại phải dùng **board thật** (không phải simulator).

### pot-sol

- (a) Chấp nhận: trong Wokwi chỉ validate STA → WSS flow (qua preconfig),
  bỏ phần "điện thoại join SoftAP".
- (b) Dùng board thật cho phần SoftAP/provisioning bằng điện thoại.
- (c) Upgrade Private Gateway để có incoming (điện thoại/browser reach
  thiết bị) — nhưng vẫn là radio ảo, không phải Wi-Fi thật.

### selected-choice

Chọn (a) cho phạm vi Wokwi: Wokwi chỉ validate STA→cloud. Phần "điện thoại
join SoftAP" thuộc **board thật** (ghi trong `STATUS.md` gate tiếp theo).

---

## 5. `localhost:8185` (forward Wokwi) không truy cập được từ browser (2026-08-23)

### desc

`wokwi.toml` khai báo `[[net.forward]] from="localhost:8185" to="target:80"`
(HTTP server trong ESP32). Tuy nhiên mở `http://localhost:8185` từ browser
thật không ra được trang.

### root-cause

Forward này chỉ có hiệu lực **bên trong** simulator/gateway của Wokwi để
định hướng traffic, không phải một port thật đang listen trên máy host
khi chạy headless/VS Code theo cách của chúng ta. Để browser thật reach
HTTP server trong ESP32 cần **Private IoT Gateway** (incoming) — free tier
không có.

### pot-sol

- (a) Dùng Private Gateway (trả phí) để incoming từ browser.
- (b) Chạy `wokwi-cli` với gateway local hỗ trợ forward (nếu có) và kiểm
  tra lại từ đúng host.
- (c) Không phụ thuộc `localhost:8185`; validate HTTP server trong ESP32
  bằng cách khác (gọi từ trong simulator / board thật).

### selected-choice

Chọn (c): không chặn vào `localhost:8185`; đây không nằm trong critical
path của POC5 (POC5 dùng WSS outbound, không cần reach HTTP server trong
ESP32 từ bên ngoài).

---