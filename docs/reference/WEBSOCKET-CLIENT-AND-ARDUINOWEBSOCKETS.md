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

### 3.2 Khởi tạo & Cấu hình Kết nối Chuẩn
```cpp
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>

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
  // 1. Cấu hình chế độ bảo mật TLS
  // Trong môi trường development / tunnel (Cloudflare, ngrok):
  webSocket.setInsecure();
  
  // Trong môi trường production với chứng chỉ Root CA cố định:
  // webSocket.setCACert(SERVER_ROOT_CA);

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

## 4. Những Sai sót Thường gặp & Bài học Đã Chứng thực

### 4.1 Lỗi Thiếu `setInsecure()` trên nhánh ESP32 của Thư viện
- **Hiện tượng:** Khi gọi `webSocket.setInsecure()`, ESP32 vẫn báo lỗi `WSS_CONNECT_FAILED`.
- **Nguyên nhân gốc rễ:** Trong thư viện `ArduinoWebsockets 0.5.4`, class `SecuredEsp32TcpClient` (trong `esp32_tcp.hpp`) bị thiếu hàm `setInsecure()`, dẫn đến việc `WiFiClientSecure` bên dưới vẫn cố xác thực với chuỗi CA rỗng.
- **Giải pháp:** Đảm bảo class `SecuredEsp32TcpClient` đã định nghĩa `void setInsecure() { this->client.setInsecure(); }` và nhánh `#elif defined(ESP32)` trong `websockets_client.cpp` gọi `client->setInsecure()` khi không truyền CA cert.

### 4.2 Lỗi Strict Schema Validation trên Máy chủ (Pydantic / TypeScript)
- **Hiện tượng:** ESP32 kết nối WSS thành công (`WSS_UPGRADED`), gửi gói tin `WSS_HELLO_SENT` nhưng máy chủ lập tức đóng socket (`WSS_DISCONNECTED`).
- **Nguyên nhân:** Máy chủ sử dụng Pydantic với cấu hình cấm trường lạ/thiếu trường (`extra="forbid"`). Gói tin JSON của ESP32 bị thiếu trường bắt buộc (ví dụ: thiếu `"firmware": "poc5-cloud-device-1.0.0"`).
- **Lưu ý:** Mọi struct JSON trên C++ phải khớp chính xác 100% với Schema định nghĩa trên Server Backend.

### 4.3 Đồng bộ Thời gian thực qua NTP trước khi Bắt tay TLS
- Khi sử dụng xác thực chứng chỉ Root CA cố định (`setCACert`), nếu ESP32 chưa lấy được giờ qua NTP (`configTime(0, 0, "pool.ntp.org")`), đồng hồ ESP32 mặc định ở năm 1970 $\rightarrow$ mbedTLS sẽ từ chối chứng chỉ vì xem là chưa tới ngày hiệu lực.

### 4.4 Cơ chế Reconnect với Exponential Backoff
- Không kết nối lại ngay lập tức khi mất mạng. Luôn áp dụng chu kỳ chờ tăng dần (2s $\rightarrow$ 4s $\rightarrow$ 8s $\rightarrow$ 16s $\rightarrow$ 30s) để tránh flood máy chủ và làm cạn kiệt tài nguyên bộ nhớ RAM của vi điều khiển.
