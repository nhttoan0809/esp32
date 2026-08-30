# Mô phỏng Wokwi & Giới hạn Kỹ thuật (Simulation & Tool Limits)

Tài liệu này ghi nhận nguyên lý hoạt động, cách thức cấu hình và các **giới hạn thực tế** của nền tảng mô phỏng Wokwi Simulator đối với các dự án IoT ESP32.

---

## 1. Kiến trúc mô phỏng Wokwi

Wokwi mô phỏng lõi Xtensa Dual-Core của ESP32, bộ nhớ, ngoại vi GPIO, Timer, UART, I2C, SPI, và radio Wi-Fi ảo.

### 1.1 Hai tệp cấu hình cốt lõi
1. **`wokwi.toml`**: Khai báo đường dẫn đến firmware ELF và binary được biên dịch từ PlatformIO:
   ```toml
   [wokwi]
   version = 1
   elf = ".pio/build/esp32dev/firmware.elf"
   firmware = ".pio/build/esp32dev/firmware.bin"
   rfc2217ServerPort = 4000
   ```
2. **`diagram.json`**: Mô tả linh kiện phần cứng và sơ đồ kết nối chân (Wiring map):
   ```json
   {
     "version": 1,
     "author": "Antigravity",
     "editor": "wokwi",
     "parts": [
       { "type": "board-esp32-devkit-v1", "id": "esp", "top": 0, "left": 0, "attrs": {} },
       { "type": "wokwi-led", "id": "led1", "top": 100, "left": 250, "attrs": { "color": "red" } }
     ],
     "connections": [
       [ "esp:23", "led1:A", "red", [] ],
       [ "led1:C", "esp:GND.1", "black", [] ],
       [ "esp:TX", "$serialMonitor:RX", "", [] ],
       [ "esp:RX", "$serialMonitor:TX", "", [] ]
     ]
   }
   ```

---

## 2. Các giới hạn kỹ thuật đã được xác nhận (Tool Limits Register)

Những hạn chế dưới đây xuất phát từ đặc tính của Wokwi (đặc biệt là tài khoản miễn phí - Free tier), không phải là lỗi trong source code:

### 2.1 Wokwi Free không hỗ trợ Private IoT Gateway
- **Hiện tượng:** Không thể truy cập các dịch vụ web server chạy bên trong ESP32 từ trình duyệt máy tính host (ví dụ mở portal SoftAP tại `http://localhost:8185` hay `http://192.168.4.1`).
- **Nguyên nhân:** Wokwi Free sử dụng **Public Gateway** (chạy remote trên cloud) chỉ cho phép lưu lượng outbound đơn giản. **Private IoT Gateway** (chạy bridge local, hỗ trợ incoming connection và TLS đầy đủ) chỉ dành cho gói trả phí (Paying users).
- **Giải pháp xử lý:** Trong firmware thử nghiệm trên Wokwi, thiết kế cơ chế **Preconfig** (bỏ qua SoftAP portal khi NVS rỗng, kết nối thẳng tới AP ảo `Wokwi-GUEST` và host server).

### 2.2 Wokwi Public Gateway: Bắt tay TLS Outbound không hoàn tất (Issue #721)
- **Hiện tượng:** ESP32 mô phỏng có thể kết nối TCP thường (`TCP_PROBE_OK`) và đồng bộ giờ NTP qua UDP (`TIME_SYNCED`), nhưng **bắt tay TLS (HTTPS / WSS)** tới các public tunnel như ngrok thường không hoàn tất hoặc bị timeout (`TLS_PROBE_FAILED code=0 elapsed_ms > 20000`).
- **Nguyên nhân:** Lớp mạng Public Gateway của Wokwi gặp độ trễ lớn và giới hạn khi xử lý TLS handshake phức tạp của một số proxy/cloud endpoints (Wokwi GitHub Issue #721).
- **Giải pháp:** 
  - Chấp nhận xác nhận logic Wi-Fi/NTP trên Wokwi và nghiệm thu WSS/TLS end-to-end trên **Board thật**.
  - Hoặc sử dụng WebSocket không mã hoá (`ws://` plain port) nếu chỉ test trong môi trường mô phỏng.

### 2.3 Mạng Wi-Fi ảo `Wokwi-GUEST` & Radio SoftAP
- **Hiện tượng:** Điện thoại thật không thể quét thấy mạng Wi-Fi `Wokwi-GUEST` hoặc `ESP32-SETUP-XXXX` do Wokwi phát ra.
- **Nguyên nhân:** Wokwi chỉ mô phỏng Wi-Fi ở tầng phần mềm (Virtual Stack). ESP32 ảo không phát ra sóng vô tuyến 2.4 GHz vật lý ra không gian xung quanh máy tính.
- **Giải pháp:** Các tính năng cần tương tác qua sóng Wi-Fi thật (điện thoại join SoftAP, captive portal) bắt buộc kiểm thử trên **Board ESP32 thật**.

### 2.4 Wokwi không mô phỏng Bluetooth / BLE
- Wokwi hiện tại **chưa hỗ trợ** ngăn xếp Bluetooth Classic và BLE trên ESP32. Mọi tính năng BLE bắt buộc phải build và test trên phần cứng thực tế.

---

## 3. Bảng so sánh Môi trường Thử nghiệm

| Tiêu chí | Mô phỏng Wokwi CLI | Board thật ESP32 DevKit V1 |
|---|:---:|:---:|
| **Biên dịch & cú pháp code** | ✅ 100% giống thật | ✅ 100% giống thật |
| **Logic GPIO, LED, Nút bấm, I2C OLED** | ✅ Hoạt động hoàn hảo | ✅ Hoạt động hoàn hảo |
| **Log Serial UART 115200** | ✅ Thu thập tức thì qua CLI | ✅ Thu thập qua `pio device monitor` |
| **Kiểm thử tự động CI/CD** | ✅ Xuất sắc (`wokwi-cli --expect-text`) | ⚠️ Cần rig phần cứng testbed |
| **Giao tiếp SoftAP với điện thoại** | ❌ Không hỗ trợ | ✅ Hoạt động đầy đủ |
| **Bảo mật WSS / TLS khắt khe** | ⚠️ Bị giới hạn bởi Gateway | ✅ Hoạt động hoàn hảo với mbedTLS |
| **Bluetooth / BLE** | ❌ Không hỗ trợ | ✅ Hoạt động đầy đủ |
