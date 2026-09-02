# Kiến trúc Wi-Fi Provisioning & Thư viện Cốt lõi WiFiManager

Tài liệu này đặc tả mô hình kiến trúc, quy trình tiêu chuẩn và các bài học kinh nghiệm sâu sắc để thiết lập cấu hình Wi-Fi cho ESP32 trong môi trường thực tế, chỉ định **`tzapu/WiFiManager`** là thư viện cốt lõi tiêu chuẩn của dự án.

---

## 1. Bản chất Phần cứng: Giới hạn Single-PHY RF trên ESP32

Vi điều khiển ESP32 chỉ sở hữu **một bộ thu phát sóng RF 2.4GHz vật lý duy nhất** dùng chung cho cả giao diện SoftAP và Station (STA):

```text
               ┌─────────────────────────────────────┐
               │    ESP32 Single 2.4GHz RF PHY       │
               │ (Chỉ hoạt động trên DUY NHẤT 1 kênh)│
               └──────────────────┬──────────────────┘
                                  │
                 ┌────────────────┴────────────────┐
                 ▼                                 ▼
      ┌────────────────────┐            ┌────────────────────┐
      │  SoftAP Interface  │            │  Station Interface │
      │ (Kênh bắt buộc: X) │            │ (Kênh bắt buộc: X) │
      └────────────────────┘            └────────────────────┘
```

### Hiện tượng "Khóa kênh RF" (RF Channel Lock)
1. Khi ESP32 chạy chế độ `WIFI_AP_STA`, SoftAP phát sóng ở **Kênh 1** (`CH1`).
2. Khi điện thoại kết nối vào SoftAP, phần cứng vô tuyến bị khoá chặt ở Kênh 1 để duy trì kết nối với điện thoại.
3. Nếu người dùng chọn mạng Wi-Fi gia đình/quán cà phê nằm ở **Kênh 10** (`CH10`), vi điều khiển không thể vừa phát Kênh 1 vừa nhảy sang Kênh 10 để bắt tay WPA2 $\rightarrow$ **Ngăn xếp Wi-Fi của ESP-IDF bị treo hoặc timeout hoàn toàn mà không có sự kiện ngắt kết nối rõ ràng**.

### Mô hình Tiêu chuẩn Công nghiệp: Save & Clean Connect
Để triệt tiêu 100% xung đột Single-PHY:
- **Giai đoạn Cấu hình:** Chạy chế độ SoftAP (kèm Captive Portal).
- **Giai đoạn Lưu & Kết nối:** Sau khi nhận cấu hình từ người dùng, lập tức đóng hoàn toàn SoftAP, giải phóng toàn bộ tài nguyên RF sang chế độ `WIFI_STA` thuần túy và kết nối vào Router.

---

## 2. Thư viện Cốt lõi: `tzapu/WiFiManager`

Dự án quy chuẩn sử dụng thư viện **`tzapu/WiFiManager`** (phiên bản `>= 2.0.17`) cho toàn bộ tác vụ quản lý kết nối và cấp phát Wi-Fi.

### 2.1 Ưu điểm Vượt trội
1. **Captive Portal Tự động:** Khi điện thoại kết nối vào mạng Wi-Fi của ESP32, màn hình đăng nhập tự động bung lên ngay lập tức (không cần người dùng mở trình duyệt gõ `192.168.4.1`).
2. **Quét Mạng Trực quan:** Tự động quét các mạng 2.4GHz xung quanh và hiển thị dạng danh sách chọn kèm cường độ tín hiệu (RSSI).
3. **Hỗ trợ Tham số Tùy chỉnh (Custom Parameters):** Cho phép bổ sung các ô nhập liệu cấu hình máy chủ Cloud (`Server Host`, `Port`, `Token`, `Path`).
4. **Tự động Quản lý NVS:** Tự động lưu SSID và mật khẩu vào phân vùng NVS của ESP-IDF và nạp lại ở các lần khởi động sau.

---

## 3. Cấu hình Thiết yếu & Quy trình Triển khai

### 3.1 Khai báo trong `platformio.ini`
```ini
lib_deps =
  https://github.com/tzapu/WiFiManager.git
```

### 3.2 Khởi tạo và Cấu hình Tham số
```cpp
#include <WiFiManager.h>

WiFiManager wm;
char serverHost[128] = "width-beaver-brad-chairman.trycloudflare.com";
char serverPort[8]   = "443";
char serverPath[64]   = "/ws/devices";
bool shouldSaveConfig = false;

// Callback khi người dùng bấm Save trên Portal
void saveConfigCallback() {
  shouldSaveConfig = true;
}

// Callback khi ESP32 bật chế độ SoftAP
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.printf("[WM] Đã bật SoftAP: %s (IP: %s)\r\n",
                myWiFiManager->getConfigPortalSSID().c_str(),
                WiFi.softAPIP().toString().c_str());
  // Bật đèn LED chỉ thị trạng thái Portal
}

void setup() {
  Serial.begin(115200);

  // 1. Khai báo các tham số tuỳ biến
  WiFiManagerParameter custom_host("host", "Server Host", serverHost, 128);
  WiFiManagerParameter custom_port("port", "Server Port", serverPort, 8);
  WiFiManagerParameter custom_path("path", "WebSocket Path", serverPath, 64);

  wm.addParameter(&custom_host);
  wm.addParameter(&custom_port);
  wm.addParameter(&custom_path);

  // 2. Thiết lập thời gian chờ (Timeout)
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setAPCallback(configModeCallback);
  wm.setConnectTimeout(25);       // Thử kết nối router trong 25 giây
  wm.setConfigPortalTimeout(180); // Mở portal trong 3 phút nếu không có ai cấu hình

  // 3. Tự động kết nối hoặc bật Portal
  String apSsid = "ESP32-SETUP-" + String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);
  apSsid.toUpperCase();

  bool res = wm.autoConnect(apSsid.c_str(), "configure-me");

  if (!res) {
    Serial.println("[WM] Kết nối thất bại hoặc quá thời gian chờ.");
  } else {
    Serial.println("[WM] Kết nối Wi-Fi thành công!");
    if (shouldSaveConfig) {
      // Lưu các tham số tùy biến vào NVS (Preferences)
    }
  }
}
```

---

## 4. Xử lý NVS: Xoá & Cập nhật Cấu hình khi Đổi Server

Khi ESP32 đã lưu thông tin Wi-Fi và Server vào NVS, ở các lần khởi động tiếp theo nó sẽ tự động kết nối và **bỏ qua Portal**. Khi cần đổi cấu hình Server hoặc Wi-Fi, áp dụng 3 phương án sau:

### 4.1 Phương án 1: Xoá trắng Flash/NVS bằng CLI (Nhanh nhất)
```bash
# Xoá sạch toàn bộ Flash (Wi-Fi, NVS Preferences)
pio run -d pocs/poc5-cloud-device -e esp32dev -t erase --upload-port /dev/cu.usbserial-XXXX

# Nạp lại firmware
pio run -d pocs/poc5-cloud-device -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX
```

### 4.2 Phương án 2: Factory Reset bằng Nút bấm vật lý (GPIO 25)
Nhấn giữ nút bấm nối vào GPIO 25 trong $\ge 5$ giây:
```cpp
void factoryReset() {
  wm.resetSettings(); // Xoá Wi-Fi trong NVS
  configStore.clear(); // Xoá Server host trong NVS
  delay(1000);
  ESP.restart();
}
```

### 4.3 Phương án 3: Mở lại On-Demand Config Portal
Nhấn ngắn nút bấm GPIO 25 khi thiết bị đang chạy để mở lại Portal cấu hình mà **không làm mất mật khẩu Wi-Fi cũ**:
```cpp
wm.startConfigPortal("ESP32-SETUP-POC5", "configure-me");
```

---

## 5. Những Sai sót Thường gặp & Lưu ý Sống còn

1. **Băng tần 2.4GHz vs 5GHz:**
   - ESP32 **chỉ hỗ trợ băng tần 2.4GHz (802.11 b/g/n)**. Không thể quét hoặc kết nối vào mạng Wi-Fi 5GHz thuần túy.
   - Nếu router phát sóng gộp băng tần (Dual-Band Smart Connect), đảm bảo router cho phép thiết bị 2.4GHz kết nối.
2. **Không tự viết WebServer AP/STA giằng co:**
   - Tránh tự viết WebServer SoftAP duy trì đồng thời với Station kết nối router trong cùng một luồng vì sẽ vướng lỗi RF Channel Lock và sụt áp (Brownout Reset).
3. **Cấp nguồn khi bật Wi-Fi:**
   - Khi bật Wi-Fi và truyền nhận sóng, ESP32 tiêu thụ dòng điện tức thời lên đến $300\text{mA} - 500\text{mA}$. Luôn sử dụng cáp USB chất lượng tốt và nguồn cấp ổn định để tránh sụt áp gây reset (`rst:0x10 (RTCWDT_RTC_RESET)`).
