# POC AI-04: Bàn Cảm Ứng Nhận Diện Cử Chỉ Bằng Machine Learning (Capacitive Touch TinyML)

Dự án thuộc **Nhóm 3: Ứng dụng Trí tuệ Nhân tạo (Edge AI & TinyML)** trên vi điều khiển **ESP32 DevKit V1 (30 chân)**. Hệ thống biến bề mặt bàn gỗ hoặc kính mica thông thường thành giao diện người - máy (HMI) ẩn vô hình: nhận diện các cử chỉ vuốt, gõ đúp và chạm giữ thông qua mạng nơ-ron học máy nhúng (Embedded TinyML) để điều khiển phát nhạc, đèn bàn và quạt làm mát.

---

## 1. Điểm Nổi Bật & Tính Năng Kỹ Thuật

- **Zero-Sensor Touch Interface:** Tận dụng 100% các kênh cảm ứng điện dung nội tại (**Capacitive Touch Peripheral**) của ESP32, chỉ cần 4 miếng giấy bạc nhôm (Aluminium foil) dán dưới mặt bàn, không cần thêm bất kỳ module cảm biến cảm ứng chuyên dụng nào.
- **Embedded TinyML Inference Engine:**
  - Mô hình mạng nơ-ron truyền thẳng **Multi-Layer Perceptron (MLP)** kích thước siêu nhẹ: 8 ngõ vào đặc trưng $\to$ 16 nơ-ron ẩn (ReLU) $\to$ 5 phân lớp đầu ra (Softmax).
  - Thuật toán nhân ma trận C++ thuần (Pure C++) tối ưu trực tiếp trên Flash `PROGMEM`, chiếm dưới **1 KB Flash**, thời gian suy luận (Inference Latency) $< \mathbf{0.1\text{ ms}}$ trên chip 240MHz.
  - Đi kèm công cụ Python (`scripts/train_gesture_model.py`) tự động sinh dữ liệu huấn luyện, đánh giá độ chính xác (Test Accuracy $100\%$) và xuất mã nguồn C++ header `touch_model_weights.h`.
- **Phản hồi Trực quan & Đóng cắt Tải thực tế:**
  - **Màn hình LED Ma trận 8x8 MAX7219:** Driver phần cứng SPI tốc độ cao (10 MHz), hiển thị hoạt họa mũi tên trượt mượt mà (chuyển bài), icon bóng đèn phát sáng (bật đèn) và cánh quạt quay tròn (bật quạt).
  - **Module Relay 2 kênh 5V (Active LOW):** Đóng cắt tải thật cách ly quang: Kênh 1 (Đèn bàn làm việc) và Kênh 2 (Quạt tản nhiệt).

---

## 2. Bảng Phân Bổ & Ánh Xạ Chân Phần Cứng (Hardware Pinout Map)

> Tuân thủ nghiêm ngặt **Hardware Pinout Verification Rule** (Quy tắc 7 của `AGENTS.md`):

| Chân ESP32 DevKit V1 (30p) | Chức năng Kỹ thuật | Ký hiệu Bo mạch Module | Chân Wokwi (`diagram.json`) | Ghi chú & Lưu ý Kỹ thuật |
|---|---|---|---|---|
| **GPIO 4** | Touch Channel 0 (`T0`) | Pad 0 (Left Electrode) | `btn0:1.r` (Nối qua Button tới GND) | Điện cực trái cùng; đo bằng `touchRead(4)` |
| **GPIO 13** | Touch Channel 4 (`T4`) | Pad 1 (Mid-Left) | `btn1:1.r` (Nối qua Button tới GND) | Điện cực giữa trái; đo bằng `touchRead(13)` |
| **GPIO 14** | Touch Channel 6 (`T6`) | Pad 2 (Mid-Right) | `btn2:1.r` (Nối qua Button tới GND) | Điện cực giữa phải; đo bằng `touchRead(14)` |
| **GPIO 27** | Touch Channel 7 (`T7`) | Pad 3 (Right Electrode) | `btn3:1.r` (Nối qua Button tới GND) | Điện cực phải cùng; đo bằng `touchRead(27)` |
| **GPIO 23** | VSPI MOSI | `DIN` (MAX7219 Matrix) | `matrix1:DIN` | Tín hiệu dữ liệu SPI điều khiển ma trận LED 8x8 |
| **GPIO 18** | VSPI SCK | `CLK` (MAX7219 Matrix) | `matrix1:CLK` | Xung đồng hồ SPI (10 MHz) |
| **GPIO 5** | VSPI CS / SS | `CS` / `LOAD` (MAX7219) | `matrix1:CS` | Tín hiệu chốt khung hiển thị ma trận |
| **GPIO 25** | Digital Output | `IN1` (Relay Kênh 1) | `relay1:IN` | Điều khiển Đèn bàn (Active LOW: kéo LOW đóng tiếp điểm) |
| **GPIO 26** | Digital Output | `IN2` (Relay Kênh 2) | `relay2:IN` | Điều khiển Quạt bàn (Active LOW: kéo LOW đóng tiếp điểm) |
| **GPIO 2** | Digital Output | `LED_BUILTIN` | LED xanh tích hợp | Nháy sáng khi nhận diện thành công cử chỉ |
| **VIN (5V)**| Nguồn cấp DC 5V | `V+` (MAX7219) & `VCC` (Relay)| `matrix1:V+`, `relay1:VCC`, `relay2:VCC` | Cấp nguồn 5V từ cổng USB máy tính qua chân VIN |
| **GND** | Mass chung | `GND` tất cả module | Nối chung toàn bộ bus GND | Đảm bảo đẳng thế mass cho toàn bộ hệ thống |

> ⚠️ **Lưu ý an toàn phần cứng:**
> - Dự án không sử dụng GPIO 12 (`T5`) vì đây là chân MTDI strapping pin; nếu bị kéo sai mức áp khi khởi động có thể gây lỗi điện áp nạp Flash SPI.
> - Tiếp điểm Relay chỉ sử dụng để điều khiển tải an toàn điện áp thấp (< 24V DC / pin), tuyệt đối không đóng cắt trực tiếp điện lưới AC 220V trong phạm vi thực hành.

---

## 3. Kiến Trúc Thuật Toán Học Máy (TinyML Pipeline)

```
[4x Touch Electrodes (Giấy bạc)]
              │
    (Lấy mẫu 50Hz / 20ms)
              ▼
    [TouchSampler Module]
              │ (Tự cân chỉnh Baseline: Bi - Si(t))
              ▼
   [FeatureExtractor (Cửa sổ 20 frames = 400ms)]
              │ Vector đặc trưng 8 chiều:
              │ [t_peak(0..3), EnergyAsymmetry, SwipeGradient, PeakCount, HoldRatio]
              ▼
   [MLInferencer (Embedded MLP 8 -> 16 -> 5)]
              │ (Inference Time < 0.1ms)
              ▼
        [Cử Chỉ Dự Đoán]
      ┌───────┴────────────────────────┐
      ▼                                ▼
[MAX7219 8x8 LED Matrix]      [Module Relay 2 Kênh]
- Mũi tên trượt phải (>>>)     - Gõ đúp -> Đảo Đèn bàn (CH1)
- Mũi tên trượt trái (<<<)     - Chạm giữ -> Đảo Quạt bàn (CH2)
- Icon Bóng đèn sáng/tắt
- Icon Cánh quạt quay
```

### 3.1 Bảng Phân Lớp Cử Chỉ (Gesture Taxonomy)

| Cử chỉ (Gesture) | Thao tác vật lý | Hành động hệ thống | Hiệu ứng Ma trận MAX7219 |
|---|---|---|---|
| `SWIPE_RIGHT` | Vuốt ngón tay từ Pad 0 sang Pad 3 | Chuyển bài hát tiếp theo (`Next Track`) | Mũi tên nhấp nháy chạy trượt sang phải |
| `SWIPE_LEFT` | Vuốt ngón tay từ Pad 3 sang Pad 0 | Lùi bài hát trước đó (`Previous Track`) | Mũi tên nhấp nháy chạy trượt sang trái |
| `DOUBLE_TAP` | Gõ 2 nhịp nhanh liên tiếp trên pad | Bật/tắt Đèn bàn làm việc (`Relay CH1`) | Icon bóng đèn tròn phát sáng / tắt |
| `HOLD` | Chạm giữ ngón tay liên tục $> 700\text{ ms}$ | Bật/tắt Quạt làm mát bàn (`Relay CH2`) | Icon cánh quạt 4 cánh quay tròn |
| `IDLE` | Không chạm hoặc rung nhiễu môi trường | Chế độ chờ, không tác động | 4 chấm trung tâm hiển thị nhịp tim (Heartbeat) |

---

## 4. Hướng Dẫn Thao Tác CLI (Command-Line Interface)

### 4.1 Huấn Luyện Lại Mô Hình TinyML (Tùy chọn)
Script Python sử dụng thư viện chuẩn của Python (không cần cài thêm `numpy` hay `scikit-learn`):
```bash
python3 pocs/poc-ai-04-capacitive-touch-ml/scripts/train_gesture_model.py
```
*Kết quả:* Huấn luyện 120 epochs đạt độ chính xác $100\%$, tự động ghi đè bộ trọng số mới vào `include/touch_model_weights.h`.

### 4.2 Biên Dịch Firmware (PlatformIO)
```bash
# Biên dịch cho board phần cứng thật
pio run -d pocs/poc-ai-04-capacitive-touch-ml -e esp32dev

# Biên dịch cho môi trường mô phỏng Wokwi
pio run -d pocs/poc-ai-04-capacitive-touch-ml -e wokwi

# Kiểm tra file nhị phân sau khi build
test -f pocs/poc-ai-04-capacitive-touch-ml/.pio/build/esp32dev/firmware.bin && echo "BIN OK"
```

### 4.3 Nạp Code & Theo Dõi Serial Monitor Trên Board Thật (macOS)
```bash
# Liệt kê cổng USB Serial
pio device list

# Nạp code lên ESP32
pio run -d pocs/poc-ai-04-capacitive-touch-ml -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# Mở Serial Monitor với baudrate 115200
pio device monitor -d pocs/poc-ai-04-capacitive-touch-ml -b 115200
```

### 4.4 Kiểm Thử Mô Phỏng (Wokwi CLI & VS Code Extension)
```bash
# Kiểm tra cú pháp sơ đồ mạch diagram.json
wokwi-cli lint pocs/poc-ai-04-capacitive-touch-ml

# Khởi chạy mô phỏng tự động (Yêu cầu WOKWI_CLI_TOKEN)
wokwi-cli pocs/poc-ai-04-capacitive-touch-ml
```
*Trên giao diện Wokwi (Web hoặc VS Code Extension):*
- Nhấn các phím nóng `1`, `2`, `3`, `4` tương ứng với 4 nút bấm trên sơ đồ để kích hoạt thao tác chạm điện dung.
- Hoặc nhập các phím ký tự trực tiếp qua Serial Monitor để giả lập cử chỉ:
  - Phím `r`: Giả lập `SWIPE_RIGHT`
  - Phím `l`: Giả lập `SWIPE_LEFT`
  - Phím `d`: Giả lập `DOUBLE_TAP`
  - Phím `h`: Giả lập `HOLD`
  - Phím `s`: In trạng thái hiện tại của Đèn, Quạt và các kênh cảm ứng
