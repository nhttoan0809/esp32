# Kiến trúc Secure WebSockets (WSS) & Thư viện Cốt lõi ArduinoWebsockets

Tài liệu này đặc tả mô hình kết nối WebSocket bảo mật hai chiều (WSS qua TLS/HTTPS) cho ESP32, chỉ định **`gilmaimon/ArduinoWebsockets`** là thư viện cốt lõi tiêu chuẩn, đồng thời đúc kết các cấu hình thiết yếu và những lỗi dễ gặp khi kết nối qua các Public Tunnel (Cloudflare Tunnel, ngrok).

---

## 1. Nguồn Tài liệu & Thư viện Chuẩn

- **Thư viện Cốt lõi:** [`gilmaimon/ArduinoWebsockets`](https://github.com/gilmaimon/ArduinoWebsockets) (Phiên bản `^0.5.4`).
- **PlatformIO Package:** `gilmaimon/ArduinoWebsockets @ ^0.5.4`
- **Mục tiêu:** Thiết lập kết nối WebSocket Outbound từ ESP32 đến Cloud Server (FastAPI / Node.js) thông qua cổng bảo mật 443 (WSS).

---

## 2. Vì sao chọn `gilmaimon/ArduinoWebsockets`?

Trong quá trình thử nghiệm thực tế, thư viện cũ (`Links2004/arduinoWebSockets`) bộc lộ nhiều điểm hạn chế nghiêm trọng khi đi qua Reverse Proxy:

| Tiêu chí | `Links2004/arduinoWebSockets` (Cũ) | `gilmaimon/ArduinoWebsockets` (Chuẩn mới) ⭐ |
| :--- | :--- | :--- |
| **Cú pháp URL** | Phải tách rời `host`, `port`, `path` | Truyền trực tiếp `wss://domain.com/path` |
| **Header `Host:`** | Tự động chèn `:443` (`Host: domain.com:443`) gây lỗi từ chối ở ngrok/Cloudflare | Tạo Header `Host: domain.com` chuẩn RFC |
| **Tùy biến Header** | Ghép chuỗi thô `\r\n` dễ sai sót | Hàm `addHeader("Key", "Value")` tường minh |
| **Kiến trúc C++** | C thô sơ, khó quản lý Event | C++11 hiện đại, Lambda Callbacks (`onMessage`, `onEvent`) |
| **Xử lý TLS** | Dễ bị nghẽn buffer, gây lỗi `TCP connection cleanup` | Tối ưu riêng cho `WiFiClientSecure` trên ESP32 |

---

## 3. Cấu hình Thiết yếu & Mô hình Triển khai

### 3.1 Khai báo trong `platformio.ini`
```ini
lib_deps =
  gilmaimon/ArduinoWebsockets@^0.5.4
  bblanchon/ArduinoJson@7.4.3
```

### 3.2 Khởi tạo & Cấu hình Kết nối Chuẩn (Cách 1: Native API `setCACert`)
```cpp
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include "tls_ca.h"

using namespace websockets;

WebsocketsClient webSocket;

void onMessageCallback(WebsocketsMessage message) {
  Serial.printf("[WS] Nhận tin nhắn: %s\r\n", message.data().c_str());
  // Xử lý JSON lệnh từ Server
}

void onEventCallback(WebsocketsEvent event, String data) {
  if (event == WebsocketsEvent::ConnectionOpened) {
    Serial.println("[WS] 🔗 Đã kết nối WSS thành công!");
  } else if (event == WebsocketsEvent::ConnectionClosed) {
    Serial.println("[WS] ❌ Mất kết nối WSS!");
  } else if (event == WebsocketsEvent::GotPing) {
    webSocket.pong();
  }
}

void setupWebSocket(const String &serverHost, uint16_t serverPort, const String &socketPath, const String &token) {
  // 1. Cấu hình bảo mật TLS bằng Root CA Bundle chuẩn (Cách 1)
  // Sử dụng API chính thức của gilmaimon/ArduinoWebsockets trên ESP32:
  webSocket.setCACert(SERVER_ROOT_CA);

  // 2. Thêm các Header xác thực & vượt rào cản Tunnel
  webSocket.addHeader("Authorization", "Bearer " + token);
  webSocket.addHeader("ngrok-skip-browser-warning", "69420");
  webSocket.addHeader("User-Agent", "ESP32-Client");

  // 3. Đăng ký Callbacks
  webSocket.onMessage(onMessageCallback);
  webSocket.onEvent(onEventCallback);

  // 4. Kết nối WSS
  String scheme = (serverPort == 443) ? "wss://" : "ws://";
  String portPart = (serverPort == 443 || serverPort == 80) ? "" : (":" + String(serverPort));
  String url = scheme + serverHost + portPart + socketPath;

  Serial.printf("[WS] Đang kết nối tới: %s\r\n", url.c_str());
  webSocket.connect(url);
}

void loop() {
  // Bắt buộc gọi poll() trong loop để xử lý frame
  if (webSocket.available()) {
    webSocket.poll();
  }

  // Gửi Ping định kỳ (mỗi 20s) để giữ kết nối qua Cloudflare/Proxy
  static uint32_t lastPing = 0;
  if (millis() - lastPing > 20000) {
    lastPing = millis();
    webSocket.ping();
  }
}
```

---

## 4. Bản chất của `tls_ca.h`: Nguồn Gốc & Cách Thu Thập

### 4.1 Nội dung `tls_ca.h` có phải lấy trên mạng hay tự tạo ra?

> [!IMPORTANT]
> **Chứng chỉ Root CA trong `tls_ca.h` KHÔNG THỂ tự sinh ngẫu nhiên (Self-Generated) bằng lệnh cục bộ nếu server sử dụng Public CA.**

- **Cơ chế hoạt động:** Khi ESP32 thiết lập bắt tay TLS với Cloudflare Tunnel, ngrok hoặc các dịch vụ đám mây công cộng (AWS, Google Cloud, Heroku...), máy chủ từ xa gửi về một chuỗi chứng chỉ số (**Certificate Chain**) được ký bởi một tổ chức cấp phát chứng chỉ uy tín thế giới (**Public Certificate Authority - CA**).
- **Tại sao không tự tạo?** Nếu lập trình viên tự tạo chứng chỉ giả định (self-signed cert) bằng `openssl req -x509` rồi gán vào `SERVER_ROOT_CA`, tầng `mbedTLS` trên ESP32 sẽ xác thực chữ ký toán học thất bại và ngắt kết nối ngay lập tức (`WSS_CONNECT_FAILED`). Trừ trường hợp bạn tự dựng toàn bộ Private CA và cài Root CA đó lên server riêng của bạn, còn khi dùng Cloudflare/ngrok, bạn bắt buộc phải cung cấp đúng Root CA của tổ chức đã cấp phát chứng chỉ cho tunnel đó.
- **Root CA là gì?** Root CA là các chứng chỉ công khai (**Public Trust Anchors**) được quốc tế thừa nhận và nhúng sẵn trong hệ điều hành máy tính/điện thoại (như Google Trust Services, Let's Encrypt, GlobalSign, DigiCert). ESP32 không có sẵn ổ cứng chứa hàng trăm Root CA như Windows hay macOS, do đó ta phải nạp trực tiếp Root CA cần thiết vào `tls_ca.h`.

---

### 4.2 Các Phương Pháp Thu Thập Root CA Chuẩn

Có 3 cách chính thống để lấy nội dung chuẩn cho `tls_ca.h`:

#### Cách 1: Trích xuất trực tiếp bằng OpenSSL CLI (Chính xác & Khuyên dùng nhất)
Chạy lệnh OpenSSL trực tiếp từ terminal trỏ đến domain tunnel mà bạn đang dùng:
```bash
# 1. Xem cấu trúc cây chứng chỉ (Leaf -> Intermediate -> Root)
echo | openssl s_client -showcerts -servername <domain> -connect <domain>:443 2>/dev/null | grep -E "(s:|i:)"

# 2. Xuất toàn bộ chứng chỉ dạng PEM
echo | openssl s_client -showcerts -servername <domain> -connect <domain>:443 < /dev/null
```
*Ví dụ:* Khi chạy kiểm tra `products-roses-ticket-predict.trycloudflare.com`:
```text
0 s:CN = trycloudflare.com, i:CN = WE1 (Google Trust Services)
1 s:CN = WE1,              i:CN = GTS Root R4
2 s:CN = GTS Root R4,        i:CN = GlobalSign Root CA
```
Ta thấy ngay chuỗi tin cậy neo vào **GTS Root R4** và **GlobalSign Root CA**.

#### Cách 2: Tải từ Kho Lưu Trữ Chính Thức của các Public CA
Các tổ chức cấp chứng chỉ lớn đều công khai file `.pem` của họ:
- **Google Trust Services (Dùng cho Cloudflare Tunnel):** 
  - Portal: <https://pki.goog/>
  - Toàn bộ Root Bundle: `https://pki.goog/roots.pem` (chứa `GTS Root R1`, `GTS Root R4`).
- **Let's Encrypt (Dùng cho ngrok, Certbot, SSL miễn phí phổ biến):**
  - Portal: <https://letsencrypt.org/certificates/>
  - Root Certificate: `ISRG Root X1`.
- **GlobalSign:**
  - `GlobalSign Root CA`: <https://secure.globalsign.com/cacert/root.crt>.

#### Cách 3: Trích xuất từ Trình Duyệt Web (Chrome / Edge / Safari)
1. Mở trang web (ví dụ `https://your-tunnel.trycloudflare.com`).
2. Bấm vào biểu tượng **Ổ khoá** trên thanh địa chỉ $\rightarrow$ Chọn **Connection is secure** $\rightarrow$ **Certificate is valid**.
3. Chọn tab **Certification Path** $\rightarrow$ Click vào chứng chỉ ở đỉnh cao nhất (**Root CA**).
4. Nhấn **Export** / **Details** $\rightarrow$ Lưu dưới định dạng **Base-64 encoded X.509 (.CER / .PEM)**.
5. Mở file bằng text editor và dán vào `tls_ca.h`.

---

### 4.3 Kỹ Thuật Multi-Root CA Bundle trong `tls_ca.h`

Tầng TLS của ESP32 (`mbedTLS` thông qua `WiFiClientSecure`) hỗ trợ ghép nhiều khối `-----BEGIN CERTIFICATE----- ... -----END CERTIFICATE-----` liên tiếp trong cùng một chuỗi C-string. Khi thực hiện handshake, `mbedTLS` sẽ tự động duyệt qua chuỗi để tìm Root CA khớp với chứng chỉ mà server gửi về.

Do đó, `tls_ca.h` nên cấu hình dạng **Bundle đa dụng**:
```cpp
#pragma once

// Multi-Root CA Bundle: Hỗ trợ đồng thời Cloudflare (GTS/GlobalSign) và ngrok (ISRG Root X1)
constexpr char SERVER_ROOT_CA[] = R"CERT(
-----BEGIN CERTIFICATE-----
[GTS Root R4 - Google Trust Services ECC]
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
[GlobalSign Root CA - Cross-signing Root]
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
[GTS Root R1 - Google Trust Services RSA]
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
[ISRG Root X1 - Let's Encrypt / ngrok]
-----END CERTIFICATE-----
)CERT";
```

---

### 4.4 Tự Động Hoá Cập Nhật Root CA Khi Tunnel Host Thay Đổi (`update_tls_ca.py`)

Trong quá trình phát triển (development), việc sử dụng Cloudflare Quick Tunnels (`cloudflared tunnel --url http://localhost:8000`) mang lại tính linh hoạt cao nhưng đi kèm một đặc tính: **Mỗi lần tunnel được khởi chạy lại, Cloudflare sẽ tự động cấp một URL hoàn toàn mới** (ví dụ `products-roses-ticket-predict.trycloudflare.com` $\rightarrow$ `random-subdomain.trycloudflare.com`).

Hơn nữa, nếu nhà phát triển chuyển đổi provider (từ Cloudflare sang ngrok, Pinggy, hoặc Custom Domain), Certificate Chain có thể thay đổi nhà phát hành.

Để giải quyết triệt để vấn đề này, dự án cung cấp script tự động hoá:
👉 [`products/control-lamp-through-voice/scripts/update_tls_ca.py`](file:///Users/toannguyen/Documents/esp32-learning/products/control-lamp-through-voice/scripts/update_tls_ca.py)

#### Quy trình hoạt động của Script:
1. **Trích xuất chuỗi chứng chỉ thực tế:** Sử dụng `openssl s_client` kết nối đến endpoint máy chủ mới để lấy toàn bộ certificate chain đang được cấp phát trực tiếp.
2. **Nhận diện Provider & Root Trust Anchors:** Tự động phát hiện nhà cung cấp (Cloudflare $\rightarrow$ Google Trust Services + GlobalSign; ngrok $\rightarrow$ Let's Encrypt ISRG) và ghép thành Multi-Root Bundle tối ưu.
3. **Cập nhật mã nguồn C++:** Ghi đè file [`tls_ca.h`](file:///Users/toannguyen/Documents/esp32-learning/products/control-lamp-through-voice/include/tls_ca.h) với định dạng raw string literal chuẩn C++.
4. **Đồng bộ cấu hình mặc định:** Tự động cập nhật `WOKWI_PRECONFIG_SERVER_HOST` trong [`secrets.h`](file:///Users/toannguyen/Documents/esp32-learning/products/control-lamp-through-voice/include/secrets.h).
5. **Tuỳ chọn Build & Upload 1-chạm:** Hỗ trợ tham số `--build` hoặc `--upload` để biên dịch và nạp thẳng firmware vào ESP32.

#### Các câu lệnh sử dụng thực tế:
```bash
# 1. Chỉ cập nhật tls_ca.h từ URL mới
python3 products/control-lamp-through-voice/scripts/update_tls_ca.py --host your-new-subdomain.trycloudflare.com

# 2. Cập nhật tls_ca.h và tự động biên dịch lại firmware
python3 products/control-lamp-through-voice/scripts/update_tls_ca.py --host your-new-subdomain.trycloudflare.com --build

# 3. Cập nhật tls_ca.h, biên dịch và nạp thẳng firmware vào ESP32 (1-chạm hoàn tất)
python3 products/control-lamp-through-voice/scripts/update_tls_ca.py --host your-new-subdomain.trycloudflare.com --upload --upload-port /dev/cu.usbserial-0001
```

---

## 5. Những Sai Sót Thường Gặp & Bài Học Thực Tiễn

### 5.1 So sánh Giữa Cách 1 (`setCACert`) và Cách 2 (`setInsecure`)
- **Cách 2 (Vá lõi thư viện `.pio/libdeps`):** Thư viện `ArduinoWebsockets 0.5.4` không có sẵn hàm `setInsecure()` cho ESP32. Việc tự vào thư mục ẩn `.pio/libdeps/esp32dev/...` để thêm code là "dirty hack". Khi chạy `pio run -t clean`, xoá cache hoặc clone repo sang máy khác, code sửa sẽ biến mất $\rightarrow$ build lỗi.
- **Cách 1 (Chuẩn khuyến nghị ⭐):** Sử dụng `webSocket.setCACert(SERVER_ROOT_CA)`. Đây là API chuẩn có sẵn 100% trong mã nguồn gốc của `ArduinoWebsockets`, giữ nguyên tính toàn vẹn của mã nguồn, tương thích tuyệt đối với mọi CI/CD pipeline và bảo mật trước các cuộc tấn công MitM.

### 5.2 Cơ Chế Nhắc Nhở Chẩn Đoán Thông Minh (`[TLS_DIAG]`) Trên Serial Monitor
Khi ESP32 gặp sự cố bắt tay TLS qua cổng 443 (`WSS_CONNECT_FAILED`), firmware không chỉ ghi nhận lỗi mà còn xuất thông báo chẩn đoán trực quan cùng câu lệnh khắc phục trực tiếp:
```text
WSS_CONNECT_FAILED retry_ms=2000
--------------------------------------------------------------------------------
[TLS_DIAG] ⚠️ WSS Handshake failed for host: your-subdomain.trycloudflare.com
[TLS_DIAG] 💡 If Cloudflare tunnel host changed or restarted, run to update Root CA:
           python3 products/control-lamp-through-voice/scripts/update_tls_ca.py --host your-subdomain.trycloudflare.com
           pio run -d products/control-lamp-through-voice -e esp32dev -t upload
--------------------------------------------------------------------------------
```
Thông báo này giúp lập trình viên không phải đoán lỗi hay mở tài liệu: chỉ cần copy câu lệnh gợi ý từ Serial Monitor và dán vào terminal để tự động cập nhật chứng chỉ và nạp lại code.

### 5.3 Đồng Bộ Thời Gian Thực Qua NTP Trước Khi Bắt Tay TLS
Khi xác thực chứng chỉ (`setCACert`), `mbedTLS` sẽ so sánh thời gian hiện tại của ESP32 với trường `NotBefore` và `NotAfter` trong chứng chỉ số.
- Nếu ESP32 chưa đồng bộ NTP, thời gian mặc định là ngày 01/01/1970 $\rightarrow$ Handshake lập tức bị mbedTLS từ chối vì chứng chỉ xem như "chưa đến ngày hiệu lực".
- **Bắt buộc:** Phải gọi `configTime(0, 0, "pool.ntp.org", "time.nist.gov")` và kiểm tra `time(nullptr) >= 1700000000` trước khi gọi `webSocket.connect()`.

### 5.4 Lỗi Strict Schema Validation trên Máy Chủ (Pydantic / TypeScript)
- **Hiện tượng:** ESP32 kết nối WSS thành công (`WSS_UPGRADED`), gửi gói tin `WSS_HELLO_SENT` nhưng máy chủ lập tức đóng socket (`WSS_DISCONNECTED`).
- **Nguyên nhân:** Máy chủ sử dụng Pydantic với cấu hình cấm trường lạ/thiếu trường (`extra="forbid"`). Gói tin JSON của ESP32 bị thiếu trường bắt buộc (ví dụ: thiếu `"firmware": "smart-lamp-voice-1.0.0"`).
- **Lưu ý:** Mọi struct JSON trên C++ phải khớp chính xác 100% với Schema định nghĩa trên Server Backend.

### 5.5 Cơ Chế Reconnect với Exponential Backoff
- Không kết nối lại dồn dập khi mất mạng. Luôn áp dụng chu kỳ chờ tăng dần (2s $\rightarrow$ 4s $\rightarrow$ 8s $\rightarrow$ 16s $\rightarrow$ 30s) để tránh flood máy chủ và tránh rò rỉ bộ nhớ RAM (heap fragmentation) trên ESP32.
