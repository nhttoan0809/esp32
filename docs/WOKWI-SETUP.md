# Hướng dẫn Giả lập Wokwi — ESP32 trên VS Code (không cần board)

> Để build, lint và chạy các POC từ terminal mà không đổi workspace trong VS
> Code, xem [workflow dành cho POC](POC-TERMINAL-WORKFLOW.md).

> **Mục đích của file này:** ghi lại *ý định* cài extension Wokwi (giả lập ESP32
> ngay trong VS Code) và *cấu hình cụ thể* của 2 file `wokwi.toml` + `diagram.json`
> với code hoàn chỉnh — để lần sau không phải tìm lại từ đầu.

---

## 1. Tại sao cần Wokwi?

- Bạn **chưa có board ESP32** nên không thể Upload và xem LED / Serial Monitor thật.
- **Wokwi** chạy *một con chip ESP32 ảo* ngay trong VS Code:
  - Thấy **LED nháy** như trên board thật
  - Đọc được **Serial Monitor** in `Hello ESP32!`
- Dùng **chính project + code PlatformIO** của bạn, không viết lại gì.
- **Giống web:** như Chrome DevTools / Storybook — xem UI mà không cần deploy.

## 2. Cài đặt (đã làm, ghi lại cho sau này)

1. VS Code → icon **Extensions** (`Cmd+Shift+X`) → tìm **Wokwi for VS Code** → **Install** → Reload nếu cần.
2. `Cmd+Shift+P` → **"Wokwi: Request a new License"** → mở website → **GET YOUR LICENSE** → đăng nhập (hoặc tạo tài khoản **miễn phí**) → xác nhận → *"License activated"*.

> License làm **1 lần duy nhất**, dùng chung cho mọi dự án sau.

## 3. Hai file cấu hình cần tạo

Đặt ở **thư mục gốc dự án** (cạnh `platformio.ini`).

### 3a. `wokwi.toml` — "package.json của Wokwi"

```toml
[wokwi]
version = 1
elf = ".pio/build/esp32dev/firmware.elf"
firmware = ".pio/build/esp32dev/firmware.bin"
rfc2217ServerPort = 4000
```

| Dòng | Ý nghĩa |
|---|---|
| `version = 1` | Phiên bản định dạng file — cứ để nguyên |
| `elf = ".pio/build/esp32dev/firmware.elf"` | File chương trình đã build (PlatformIO tạo ra) — Wokwi dùng cái này để mô phỏng |
| `firmware = ".pio/build/esp32dev/firmware.bin"` | Bản "đóng gói" của chương trình — cùng nơi PlatformIO tạo |
| `rfc2217ServerPort = 4000` | Mở cổng Serial ảo của Wokwi tại `localhost:4000` để kiểm tra khi cần |

> Đường dẫn có dấu `.` ở đầu = **thư mục ẩn** — giống `node_modules`, do công cụ tạo, đừng sửa tay.

### 3b. `diagram.json` — "bản vẽ mạch" của board ảo

```json
{
  "version": 1,
  "author": "toannguyen",
  "editor": "wokwi",
  "parts": [
    { "type": "board-esp32-devkit-c-v4", "id": "esp", "top": 0, "left": 0, "attrs": {} },
    { "type": "wokwi-led", "id": "led1", "top": 100, "left": 300, "attrs": { "color": "red" } }
  ],
  "connections": [
    [ "esp:2", "led1:A", "red", [] ],
    [ "led1:C", "esp:GND.1", "black", [] ],
    [ "esp:TX", "$serialMonitor:RX", "", [] ],
    [ "esp:RX", "$serialMonitor:TX", "", [] ]
  ]
}
```

| Phần | Ý nghĩa |
|---|---|
| `"parts"` | Danh sách linh kiện: board ESP32 DevKit v4 + 1 LED đỏ |
| `"connections"` | Nối GPIO 2 → LED → GND và nối UART TX/RX → Serial Monitor ảo |
| `"esp:2"` / `"esp:GND.1"` | Địa chỉ chân trên board — **phải khớp với code**: code `digitalWrite(2, ...)` ⇔ dây nối chân 2 |
| `"red"` / `"black"` | Màu dây trong sơ đồ (đỏ = tín hiệu, đen = đất) — chỉ để nhìn cho rõ |

> **Quy tắc vàng:** *chân trong diagram* = *chân trong code*. Code dùng chân 2 thì diagram cũng nối chân 2.

## 4. Code hoàn chỉnh (Bài 1 + Serial)

`src/main.cpp`:

```cpp
// Chương trình đầu tiên: nháy LED trên board

#include <Arduino.h> // nạp các hàm có sẵn của framework Arduino

void setup()
{
    pinMode(2, OUTPUT);           // chân LED (số 2) dùng làm ngõ ra
    Serial.begin(115200);         // mở "kênh nói chuyện" với máy tính
}

void loop()
{
    digitalWrite(2, HIGH);              // bật LED
    delay(1000);                        // chờ 1 giây
    digitalWrite(2, LOW);               // tắt LED
    delay(1000);                        // chờ 1 giây
    Serial.println("Hello ESP32!");     // in chữ ra Serial Monitor
}
```

## 5. Cách chạy giả lập (khi đã có đủ 3 file)

1. **Build** như bình thường (`Ctrl+S` lưu → bấm **Build ✓**) — Wokwi cần file firmware đã build.
2. Bấm icon **▶ Wokwi** (góc dưới phải VS Code, cạnh nút Build).
3. Tab **Wokwi Simulator** mở ra — bạn sẽ thấy:
   - **LED đỏ nháy** mỗi 1 giây
   - Terminal tên **Wokwi Terminal** in `Hello ESP32!` mỗi 2 giây

> Nếu không thấy icon ▶: dùng `Cmd+Shift+P` → gõ `wokwi` → **"Wokwi: Start Simulator"**.

## 6. Ghi chú

- Wokwi dùng **file đã build** — sửa code xong phải **Build lại** rồi mới bấm ▶ lại.
- Wokwi for VS Code 3.6 đưa Serial vào terminal riêng của VS Code, không hiện một bảng Serial bên dưới sơ đồ. Nếu panel terminal đang ẩn, mở **View → Terminal**, rồi chọn **Wokwi Terminal** trong danh sách terminal.
- Không dùng **PlatformIO: Serial Monitor** cho simulator. Lệnh đó tìm cổng USB/Bluetooth của thiết bị thật (ví dụ `/dev/cu.Bluetooth-Incoming-Port`), không phải ESP32 ảo.
- Đây là *bước chuẩn bị* — khi có board thật, quy trình vẫn y như cũ: Build → Upload → Serial Monitor.
- Link tham khảo: [Wokwi VS Code getting started](https://docs.wokwi.com/vscode/getting-started) · [Wokwi project config](https://docs.wokwi.com/vscode/project-config)
