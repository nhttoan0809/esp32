# Kit ESP32 Basic Starter — Danh mục thiết bị

Ghi nhận các linh kiện có trong bộ kit "ESP32 Basic Starter - Cho người mới
bắt đầu" (đặt mua 2026-08-26). Mục đích: tra nhanh kit có gì, linh kiện nào
làm được gì, và cái nào cần lưu ý khi dùng.

> File này chỉ liệt kê *cái có trong kit* và gợi ý dùng chung. Sơ đồ nối dây
> cho từng POC và cách nạp code từ Mac nằm trong file setup tương ứng, ví dụ
> [HARDWARE-SETUP-POC5.md](HARDWARE-SETUP-POC5.md).

## 1. Danh sách linh kiện

### 1.1 Board, nguồn và cáp

| Linh kiện | Số lượng | Ghi chú |
|---|---:|---|
| Board ESP32 | 1 | Ghi nhãn **ESP32 DevKit V1, 30 chân** (kiểm tra vật lý 2026-08-27). Hàng trái (từ trên): EN, VP, VN, D34, D35, D32, D33, D25, D26, D27, D14, D12, D13, GND, VIN. |
| Cáp truyền dữ liệu (cáp nạp code) | 1 | Vừa cấp nguồn vừa truyền serial; phải là cáp dữ liệu (xem mục 5). |
| Breadboard MB102 | 1 | Gắn mạch không cần hàn. |

### 1.2 Màn hình

| Linh kiện | Số lượng | Ghi chú |
|---|---:|---|
| OLED 0.96 inch | 1 | Thường là SSD1306 128×64, giao tiếp I2C. |

### 1.3 Cảm biến

| Linh kiện | Số lượng | Đo gì |
|---|---:|---|
| Module tránh chướng ngại vật LM393 | 1 | Khoảng cách vật cản (cặp hồng ngoại, so sánh ngưỡng bằng LM393). |
| Module quang trở | 1 | Cường độ ánh sáng (quang trở/LDR, có so sánh). |
| DHT11 | 1 | Nhiệt độ + độ ẩm. |
| PIR HC-SR501 | 1 | Phát hiện chuyển động. |

### 1.4 Đầu ra (output)

| Linh kiện | Số lượng | Ghi chú |
|---|---:|---|
| Buzzer thụ động | 1 | Tự sinh tone, phải cấp sóng vuông. |
| Buzzer hoạt động | 1 | Tự chạy khi có điện, chỉ bật/tắt. |
| Relay 2 kênh 5V | 1 | Đóng cắt mạch bằng tiếp điểm, chỉ dùng tải điện áp thấp. |

### 1.5 Linh kiện thụ động

| Linh kiện | Số lượng | Dùng để |
|---|---:|---|
| Điện trở 220Ω | 10 | Hạn dòng cho LED (nối series với GPIO). |
| Điện trở 1KΩ | 10 | Pull-up/pull-down, các mạch khác. |
| Điện trở 10KΩ | 10 | Pull-up/pull-down, phân áp. |
| Biến trở 10K | 1 | Nối analog vào chân ADC của ESP32. |

### 1.6 LED và nút

| Linh kiện | Số lượng | Ghi chú |
|---|---:|---|
| LED đỏ | 5 | Ngõ ra trạng thái / tải đơn giản. |
| LED vàng | 5 | Ngõ ra trạng thái / tải đơn giản. |
| LED xanh lá | 5 | Ngõ ra trạng thái / tải đơn giản. |
| LED RGB | 1 | Ngõ ra 3 màu (nối 3 kênh qua điện trở). |
| Nút nhấn 12×12 | 6 | Ngõ vào số (button). |

### 1.7 Cáp nhảy (jumper)

| Linh kiện | Số lượng |
|---|---:|
| Cáp Đực – Cái | 10 |
| Cáp Cái – Cái | 10 |
| Cáp Đực – Đực | 10 |

> Đầu "đực" (bạc, dài) cắm vào chân nam của breadboard; đầu "cái" (vòng, ngắn)
> cắm vào chân cái. Chọn loại đúng vị trí bạn cắm.

## 2. Ứng dụng gợi ý theo POC

Bảng này gợi ý *linh kiện nào phù hợp với project nào* trong repo — đây là
tra cứu chung, **không** phải sơ đồ nối dây. Sơ đồ nối dây cụ thể nằm trong
file setup của từng POC.

| POC / đề án | Linh kiện có thể dùng |
|---|---|
| POC1 (Wi-Fi HTTP), POC3 (web LED), POC4 (SoftAP) | ESP32, LED, điện trở, nút, breadboard, cáp nhảy. |
| POC5 (cloud WebSocket) | ESP32, LED + điện trở 220Ω, nút setup, breadboard, cáp nạp. Chi tiết: [HARDWARE-SETUP-POC5.md](HARDWARE-SETUP-POC5.md). |
| LLM-VOICE-03 (giọng nói + kịch bản năng lượng) | PIR HC-SR501 (đầu vào cảnh), relay 2 kênh (đóng cắt tải thấp áp), LED (tải mô phỏng). |
| Học thêm (chưa có POC trong repo) | OLED (hiển thị), DHT11 (nhiệt/ẩm), quang trở (đo sáng), biến trở + ADC, LM393 (tránh vật cản), buzzer. |

## 3. Yêu cầu ngoài kit (phải tự chuẩn bị)

| Vật | Dùng để |
|---|---|
| Router Wi-Fi có sẵn | ESP32 chạy chế độ station và kết nối internet. |
| Máy tính chạy server | Chạy FastAPI + ngrok (ở POC5). |
| Điện thoại | Join SoftAP của ESP32 để provision và mở dashboard. |
| Adapter/Power bank 5V (nếu chạy độc lập) | Cấp nguồn cho board khi không cắm máy tính. |

## 4. Lưu ý an toàn và cách dùng chung

- LED thường **luôn** nối series điện trở 220Ω với GPIO (GPIO 3.3V, LED thường
  ~2V → dòng ~6mA, an toàn).
- GPIO ESP32 chỉ ra 3.3V/5V và vài chục mA: **không nối trực tiếp tải lớn hay
  điện lưới** vào GPIO.
- Relay module 5V (chân COMMON/NO/NC) là tiếp điểm: chỉ dùng cho tải thấp áp,
  **không** dùng đóng cắt điện lưới trong phạm vi học.
- Khi nối vào breadboard, nguồn có thể lấy từ GPIO của ESP32, không bắt buộc
  bật rail 5V/3V3 của breadboard.

## 5. Trạng thái xác minh

- Board ESP32 thật ghi nhãn **ESP32 DevKit V1, 30 chân** (kiểm tra vật lý 2026-08-27). Đã đối chiếu 6 GPIO POC5 (D25 hàng trái; D18/D19/D21/D22/D23 hàng phải) khớp với layout chuẩn + phần tử Wokwi `board-esp32-devkit-v1`.
- Cáp "truyền dữ liệu" phải kiểm tra thực tế: cắm vào Mac, chạy `pio device list` (xem [HARDWARE-SETUP-POC5.md](HARDWARE-SETUP-POC5.md) mục 3) — có ra `/dev/cu.*` thì mới là cáp dữ liệu.
- Khi board ESP32 về, đối chiếu đủ các GPIO mà POC cần (POC5: 18/19/21/22/23/25) và loại chip USB-serial trên board; chi tiết xử lý khi thiếu nằm trong file setup POC5.
