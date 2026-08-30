# Cẩm nang phát triển ESP32 bằng CLI (CLI Workflow Guide)

Tài liệu này cung cấp toàn bộ quy trình phát triển, biên dịch, mô phỏng và nạp code cho ESP32 hoàn toàn thông qua giao diện dòng lệnh (**Command Line Interface - CLI**), tối ưu cho lập trình viên và Coding Agent.

---

## 1. Triết lý CLI-First & Lựa chọn công cụ

Thay vì phụ thuộc vào các extension UI trên IDE (nút bấm đồ hoạ, panel tương tác):
- **PlatformIO Core CLI (`pio`)**: Đóng vai trò quản lý build system, toolchain, thư viện phụ thuộc (`lib_deps`), nạp firmware qua USB (`upload`) và giám sát Serial (`device monitor`).
- **Wokwi CLI (`wokwi-cli`)**: Đóng vai trò mô phỏng phần cứng headless, lint sơ đồ `diagram.json`, và chạy kiểm thử tự động (assert Serial output).

---

## 2. Thiết lập môi trường CLI trên macOS

### 2.1 Cài đặt PlatformIO Core
Nếu chưa có lệnh `pio` trong PATH:
```bash
# Cách 1: Cài đặt qua Homebrew hoặc pip
brew install platformio
# hoặc: pip install -U platformio

# Cách 2: Nếu đã có PlatformIO IDE extension, pio nằm tại:
export PATH="${HOME}/.platformio/penv/bin:${PATH}"
```
Kiểm tra:
```bash
pio --version
```

### 2.2 Cài đặt Wokwi CLI
```bash
# Cài đặt qua Homebrew hoặc curl script chính thức
brew install wokwi-cli
# hoặc: curl -L https://wokwi.com/ci/install.sh | sh
```
Kiểm tra:
```bash
wokwi-cli --version
```
> Để chạy mô phỏng với `wokwi-cli`, bạn cần API Token từ <https://wokwi.com/dashboard/ci>:
> ```bash
> export WOKWI_CLI_TOKEN="<your-token-here>"
> ```

---

## 3. Cấu hình dự án chuẩn

### 3.1 `platformio.ini` (Root / Per-Project)
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
build_flags = -DCORE_DEBUG_LEVEL=0
; Khai báo thư viện phụ thuộc (nếu có):
; lib_deps =
;   bblanchon/ArduinoJson @ ^7.0.0
;   links2004/WebSockets @ ^2.4.0
```

### 3.2 `wokwi.toml`
```toml
[wokwi]
version = 1
elf = ".pio/build/esp32dev/firmware.elf"
firmware = ".pio/build/esp32dev/firmware.bin"
rfc2217ServerPort = 4000
```

---

## 4. Danh mục lệnh CLI chuẩn (Command Cheat Sheet)

### 4.1 Biên dịch & Quản lý Build (PlatformIO)
| Tác vụ | Lệnh CLI thực thi |
|---|---|
| **Build dự án hiện tại** | `pio run` hoặc `pio run -e esp32dev` |
| **Build thư mục con (ví dụ POC5)** | `pio run -d pocs/poc5-cloud-device -e esp32dev` |
| **Clean cache build** | `pio run -t clean` |
| **Kiểm tra file binary sau build** | `test -f .pio/build/esp32dev/firmware.bin && echo "Build Success"` |
| **Liệt kê thư viện đã cài** | `pio pkg list` |

### 4.2 Nạp code & Debug trên Board thật
| Tác vụ | Lệnh CLI thực thi |
|---|---|
| **Xem danh sách cổng Serial** | `pio device list` |
| **Flash firmware lên board** | `pio run -t upload --upload-port /dev/cu.usbserial-XXXX` |
| **Mở Serial Monitor (115200 baud)** | `pio device monitor -p /dev/cu.usbserial-XXXX -b 115200` |
| **Thoát Serial Monitor** | Nhấn tổ hợp phím: `Ctrl + ]` |
| **Xoá toàn bộ Flash / NVS partition** | `pio run -t erase --upload-port /dev/cu.usbserial-XXXX` |

### 4.3 Mô phỏng & Kiểm thử tự động (Wokwi CLI)
| Tác vụ | Lệnh CLI thực thi |
|---|---|
| **Lint kiểm tra file diagram.json** | `wokwi-cli lint` hoặc `wokwi-cli lint pocs/poc5-cloud-device` |
| **Khởi chạy mô phỏng headless** | `wokwi-cli .` |
| **Chạy và đợi chuỗi Serial mong đợi** | `wokwi-cli --expect-text "Hello ESP32!" --timeout 15000 .` |
| **Chạy kiểm thử có bẫy lỗi (fail-text)**| `wokwi-cli --expect-text "WIFI_CONNECTED" --fail-text "AUTH_FAILED" --timeout 30000 pocs/poc5-cloud-device` |

---

## 5. Quy trình làm việc tự động cho AI Coding Agent

Khi thực hiện một nhiệm vụ trên codebase ESP32, Agent tuân thủ chu trình 4 bước khép kín:

```text
1. Đọc code/diagram ──► 2. pio run (Biên dịch) ──► 3. wokwi-cli lint ──► 4. wokwi-cli --expect-text (Xác minh)
```

1. **Step 1:** Kiểm tra cú pháp JSON: `node -e 'JSON.parse(require("fs").readFileSync("diagram.json"))'`
2. **Step 2:** Build firmware: `pio run -e esp32dev`
3. **Step 3:** Xác nhận binary tồn tại: `test -f .pio/build/esp32dev/firmware.bin`
4. **Step 4:** Lint diagram: `wokwi-cli lint`
5. **Step 5:** Assert Serial output bằng `wokwi-cli --expect-text "<marker>"` (nếu có WOKWI_CLI_TOKEN).
