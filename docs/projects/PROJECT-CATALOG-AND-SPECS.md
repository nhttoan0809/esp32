# Danh Mục & Đặc Tả Kỹ Thuật Đề Tài Dự Án ESP32 (Project Catalog & Specifications)

> **Tài liệu nghiên cứu, phân loại và đặc tả kỹ thuật toàn diện** các đề tài ứng dụng vi điều khiển **ESP32 DevKit V1 (30 chân)** kết hợp linh kiện từ bộ **Kit Starter**, **Extended Components** và các linh kiện mở rộng tương thích.

---

## 📑 Mục Lục
1. [Ma Trận Đánh Giá & Phân Loại Đề Tài](#1-ma-trận-đánh-giá--phân-loại-đề-tài)
   - [1.1 Bảng Tổng Hợp Đề Tài & Chỉ Định Branch Git Worktree](#11-bảng-tổng-hợp-đề-tài--chỉ-định-branch-git-worktree)
   - [1.2 Quy Chuẩn Đặt Tên Branch & Hướng Dẫn Khởi Tạo Git Worktree](#12-quy-chuẩn-đặt-tên-branch--hướng-dẫn-khởi-tạo-git-worktree)
2. [Nhóm 1: Thiết Bị Di Chuyển & Xe Robot (Mobile Vehicles & Motion)](#2-nhóm-1-thiết-bị-di-chuyển--xe-robot)
3. [Nhóm 2: Điều Khiển Bằng Giọng Nói & Xử Lý Âm Thanh (Voice & Audio)](#3-nhóm-2-điều-khiển-bằng-giọng-nói--xử-lý-âm-thanh)
4. [Nhóm 3: Ứng Dụng Trí Tuệ Nhân Tạo (Edge AI & Cloud-Connected AI)](#4-nhóm-3-ứng-dụng-trí-tuệ-nhân-tạo)
5. [Nhóm 4: Cơ Điện Tử & Robotics (Robotics & Mechatronics)](#5-nhóm-4-cơ-điện-tử--robotics)
6. [Nhóm 5: Nhà Thông Minh & An Ninh Tự Động (Smart Home & Security)](#6-nhóm-5-nhà-thông-minh--an-ninh-tự-động)
7. [Nhóm 6: Nghiên Cứu Mở Rộng & Đa Vi Điều Khiển (Multi-MCU & Advanced Labs)](#7-nhóm-6-nghiên-cứu-mở-rộng--đa-vi-điều-khiển)
8. [Cẩm Nang Mua Sắm Bổ Sung & Dự Toán Ngân Sách](#8-cẩm-nang-mua-sắm-bổ-sung--dự-toán-ngân-sách)
9. [Nguyên Tắc An Toàn & Chuẩn Ghép Nối ESP32 30-Pin](#9-nguyên-tắc-an-toàn--chuẩn-ghép-nối-esp32-30-pin)

---

## 1. Ma Trận Đánh Giá & Phân Loại Đề Tài

### 1.1 Bảng Tổng Hợp Đề Tài & Chỉ Định Branch Git Worktree

| Mã Đề Tài | Tên Đề Tài | Nhóm Chủ Đề | Tên Branch Đề Xuất (Git Worktree) | Mức Độ Sẵn Sàng Phần Cứng | Chi Phí Bổ Sung (Ước tính) | Độ Phức Tạp |
|:---:|:---|:---:|:---|:---:|:---:|:---:|
| **MOB-01** | Robot Car Tự Hành Né Vật Cản Đa Hướng | Di chuyển | `feat/mob-01-range-scanning-rover` | 🟡 Thiếu Chassis & Pin | ~140.000 đ | ⭐⭐⭐ |
| **MOB-02** | Xe Thám Hiểm Điều Khiển Qua Web/BLE & Phanh An Toàn | Di chuyển | `feat/mob-02-telemetry-rover` | 🟡 Thiếu Chassis & Pin | ~140.000 đ | ⭐⭐⭐ |
| **MOB-03** | Bàn Xoay Quét 3D & Chụp Ảnh Sản Phẩm Chính Xác | Di chuyển | `feat/mob-03-precision-stepper-turntable` | 🟢 **Sẵn sàng 100%** | 0 đ | ⭐⭐ |
| **MOB-04** | Xe AGV Dẫn Hướng & Giao Hàng Điểm Trạm RFID | Di chuyển | `feat/mob-04-rfid-guided-agv` | 🟡 Thiếu Chassis & Pin | ~160.000 đ | ⭐⭐⭐⭐ |
| **VOI-01** | Trạm Điều Khiển Giọng Nói Hai Chiều (Voice Smart Hub) | Giọng nói | `feat/voi-01-voice-smart-hub` | 🟡 Cần Mic/Amp I2S | ~95.000 đ | ⭐⭐⭐⭐ |
| **VOI-02** | Công Tắc Bật Tắt Bằng Nhịp Vỗ Tay Đa Kênh (Clap Switch) | Giọng nói | `feat/voi-02-acoustic-clap-switch` | 🟢 **Sẵn sàng 100%** | 0 đ | ⭐⭐ |
| **VOI-03** | Máy Phát Nhạc & Tổng Hợp Âm Thanh TTS (Synthesizer/Talkie) | Giọng nói | `feat/voi-03-synth-tts-talkie` | 🟢 **Sẵn sàng 100%** (Passive Buzzer) / 🟡 Cần Loa | ~45.000 đ | ⭐⭐⭐ |
| **AI-01** | Thiết Bị Nhận Diện & Phân Loại Tiếng Động Bất Thường (Edge AI) | AI | `feat/ai-01-edge-sound-anomaly-detector` | 🟡 Cần Mic I2S | ~35.000 đ | ⭐⭐⭐⭐ |
| **AI-02** | Hệ Thống Dự Báo & Tối Ưu Môi Trường Bằng Mô Hình AI Riêng | AI | `feat/ai-02-predictive-climate-optimizer` | 🟢 **Sẵn sàng 100%** | 0 đ | ⭐⭐⭐ |
| **AI-03** | Trợ Lý Ảo Vật Lý AI Đa Phương Thức (Physical LLM Agent) | AI & Giọng nói | `feat/ai-03-physical-llm-agent` | 🟡 Cần Mic & Loa | ~95.000 đ | ⭐⭐⭐⭐⭐ |
| **AI-04** | Bàn Di Cảm Ứng Nhận Diện Cử Chỉ Bằng Machine Learning | AI | `feat/ai-04-capacitive-touch-ml` | 🟢 **Sẵn sàng 100%** | 0 đ | ⭐⭐⭐ |
| **ROB-01** | Cánh Tay Robot 4 Bậc Tự Do (4-DOF Robotic Arm) | Robotics | `feat/rob-01-4dof-robotic-arm` | 🟡 Cần 3 Servo & Khung | ~170.000 đ | ⭐⭐⭐⭐ |
| **ROB-02** | Dây Chuyền Phân Loại & Gắp Vật Thể Tự Động RFID | Robotics | `feat/rob-02-rfid-sorting-mechanism` | 🟡 Cần Khung Arm & Servo | ~200.000 đ | ⭐⭐⭐⭐⭐ |
| **ROB-03** | Tháp Camera Pan-Tilt Tự Động Bám Đuổi Đối Tượng | Robotics | `feat/rob-03-sentry-pan-tilt-cam` | 🟡 Cần ESP32-CAM & Khung | ~110.000 đ | ⭐⭐⭐⭐ |
| **SMH-01** | Hệ Thống An Ninh Giám Sát Đa Vùng Cảnh Báo Tức Thời | Smart Home | `feat/smh-01-multizone-defense-system` | 🟢 **Sẵn sàng 100%** | 0 đ | ⭐⭐⭐⭐ |
| **SMH-02** | Khóa Cửa Điện Tử Thông Minh 3 Lớp Bảo Mật | Smart Home | `feat/smh-02-trifactor-smart-lock` | 🟢 **Sẵn sàng 100%** | 0 đ | ⭐⭐⭐ |
| **SMH-03** | Thiết Bị Đo Điện Năng & Tự Ngắt Bảo Vệ Quá Tải | Smart Home | `feat/smh-03-smart-energy-meter` | 🟢 **Sẵn sàng 100%** | 0 đ | ⭐⭐⭐ |
| **SMH-04** | Trạm Giám Sát & Điều Hòa Nhà Kính Mini Tự Động | Smart Home | `feat/smh-04-micro-greenhouse` | 🟡 Cần Cảm biến độ ẩm đất & Bơm | ~45.000 đ | ⭐⭐⭐ |
| **EXP-01** | Trạm Điểm Danh Không Chạm RFID & Hiển Thị Đồ Họa Matrix | Mở rộng | `feat/exp-01-rfid-matrix-attendance` | 🟢 **Sẵn sàng 100%** | 0 đ | ⭐⭐⭐ |
| **EXP-02** | Hộp Câu Đố Tương Tác Thông Minh (Escape Room Puzzle Box) | Mở rộng | `feat/exp-02-escape-room-puzzle-box` | 🟢 **Sẵn sàng 100%** | 0 đ | ⭐⭐⭐ |
| **EXP-03** | Hệ Thống Phân Tán Đa Vi Điều Khiển (ESP32 + STM32 + Nano) | Mở rộng | `feat/exp-03-multi-mcu-communication` | 🟢 **Sẵn sàng 100%** | 0 đ | ⭐⭐⭐⭐ |

---

### 1.2 Quy Chuẩn Đặt Tên Branch & Hướng Dẫn Khởi Tạo Git Worktree

Nhằm phục vụ phát triển độc lập, không làm ảnh hưởng đến nhánh chính (`main`) và cho phép nhiều AI Agent / lập trình viên làm việc song song trên cùng repository, mỗi đề tài được gán cố định một nhánh Git chuẩn hóa.

#### a. Quy tắc định danh Branch
```text
feat/<mã_đề_tài_chữ_thường>-<tên_tiếng_anh_viết_tắt>
```
*Ví dụ:*
- Đề tài `MOB-03` $\rightarrow$ `feat/mob-03-precision-stepper-turntable`
- Đề tài `SMH-02` $\rightarrow$ `feat/smh-02-trifactor-smart-lock`

#### b. Lệnh CLI tạo nhanh một Worktree độc lập
Từ thư mục gốc của repository, bạn chỉ cần chạy lệnh sau để tạo một worktree riêng biệt nằm ngoài thư mục làm việc hiện tại:
```bash
# Cú pháp tổng quát:
git worktree add -b <tên_branch> <đường_dẫn_thư_mục_worktree> main

# Ví dụ triển khai ngay Đề tài MOB-03 (Bàn xoay Stepper):
git worktree add -b feat/mob-03-precision-stepper-turntable ../worktrees/mob-03-precision-stepper-turntable main
```

#### c. Dọn dẹp Worktree sau khi hoàn thành & merge
```bash
# Xóa worktree sau khi hoàn thành
git worktree remove ../worktrees/mob-03-precision-stepper-turntable
# Dọn dẹp chỉ mục worktree đã xoá
git worktree prune
```

---

## 2. Nhóm 1: Thiết Bị Di Chuyển & Xe Robot

### 2.1 Đề Tài MOB-01: Robot Car Tự Hành Né Vật Cản Đa Hướng (Autonomous Range-Scanning Rover)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/mob-01-range-scanning-rover`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/mob-01-range-scanning-rover ../worktrees/mob-01-range-scanning-rover main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Ứng dụng trong việc nghiên cứu thuật toán tránh vật cản tự động cho robot dọn dẹp vệ sinh trong nhà (Robot hút bụi), xe tự hành chở hàng trong hành lang hẹp hoặc robot thăm dò môi trường nguy hiểm.
- Giúp người học làm chủ kỹ thuật điều chế độ rộng xung (PWM) để kiểm soát vi sai tốc độ giữa hai bánh xe, giải quyết bài toán quán tính và lập bản đồ cự ly thời gian thực.

#### b. Danh Sách Thiết Bị Cần Dùng
* **Thiết bị đã có sẵn trong kho:**
  - 1× Bo mạch **ESP32 DevKit V1 (30 chân)** (Vi điều khiển trung tâm).
  - 1× Module cầu H **L298N** (Mạch kích công suất động cơ DC).
  - 2× **Động cơ giảm tốc vàng TT Motor DC (3V–6V)**.
  - 1× Cảm biến siêu âm **HC-SR04** (Mắt quét khoảng cách).
  - 1× Động cơ servo **SG90** (Xoay đầu cảm biến siêu âm góc 0°–180°).
  - 1× Màn hình **OLED 0.96" SSD1306 (I2C)** (Hiển thị khoảng cách, trạng thái chuyển động).
  - 1× Còi chíp **Active Buzzer** (Phát âm thanh cảnh báo khoảng cách).
  - Điện trở $1\text{k}\Omega$ và $2\text{k}\Omega$ (Tạo cầu phân áp chân Echo 5V $\rightarrow$ 3.3V cho ESP32).
  - Dây cắm Breadboard và dây Jumper đực-cái.
* **Thiết bị cần mua bổ sung:**
  - 1× **Khung xe robot 2 bánh (Chassis 2WD)** kèm gá bắt động cơ, bánh xe cao su và 1 bánh xe đa hướng (Caster wheel) *(Giá ~45.000 – 60.000 đ)*.
  - 1× **Hộp pin 2 cell 18650** có công tắc nguồn rời + **2 cell pin Li-ion 18650 (3.7V/cell $\rightarrow$ 7.4V)** *(Giá ~80.000 – 100.000 đ)*.
  - 1× Gá bắt cảm biến siêu âm lên động cơ servo SG90 (bằng mica hoặc in 3D) *(Giá ~10.000 đ hoặc tự chế)*.

#### c. Kiến Trúc Kết Nối & Luồng Dữ Liệu
```
[Nguồn Pin 7.4V] ───────────▶ [Cọc +12V L298N] ──(Jumper 5V-EN đóng)──▶ [Cọc +5V] ──▶ [VIN ESP32]
      │                                                                           │
   (Chung GND) ───────────────────────────────────────────────────────────── (Chung GND)
                                                                                  │
[ESP32 GPIO 25, 26, 27, 14] ────(Tín hiệu logic IN1-IN4)─────────────────────▶ [L298N] ──▶ [2× TT Motor]
[ESP32 GPIO 12, 13] ────────────(Xung PWM ENA, ENB điều tốc)─────────────────▶ [L298N]
[ESP32 GPIO 18 (PWM)] ──────────(Tín hiệu góc quét)──────────────────────────▶ [Servo SG90]
[ESP32 GPIO 5] ─────────────────(Xung kích Trig 10µs)───────────────────────▶ [HC-SR04]
[HC-SR04 Echo 5V] ──▶ [R1=1kΩ] ──▶ [ESP32 GPIO 19 Input] ──▶ [R2=2kΩ] ──▶ [GND] (Cầu Phân Áp)
[ESP32 I2C 21/22] ──────────────(SDA / SCL)──────────────────────────────────▶ [OLED 0.96"]
```

#### d. Thách Thức Kỹ Thuật & Giải Pháp An Toàn
- **Bảo vệ chống sốc áp & Brownout:** Khi động cơ đảo chiều đột ngột, dòng kẹt (stall current) có thể vượt 1A làm tụt áp hệ thống. Giải pháp: Cấp nguồn pin 7.4V riêng vào cọc công suất L298N, dùng tụ hóa $470\mu\text{F}$ lọc nguồn tại chân VIN của ESP32.
- **Bảo vệ ngõ vào Echo:** Chân Echo của HC-SR04 phát áp 5V, bắt buộc đi qua cầu phân áp $1\text{k}\Omega / 2\text{k}\Omega$ để bảo vệ GPIO 19 của ESP32.

---

### 2.2 Đề Tài MOB-02: Xe Thám Hiểm Điều Khiển Qua Web/BLE & Phanh An Toàn (Web/BLE Telemetry Rover)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/mob-02-telemetry-rover`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/mob-02-telemetry-rover ../worktrees/mob-02-telemetry-rover main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Thiết bị giám sát kho bãi, xe vận chuyển cỡ nhỏ điều khiển tầm xa qua mạng Wi-Fi nội bộ hoặc kết nối Bluetooth năng lượng thấp (BLE) từ smartphone/máy tính.
- Tích hợp lớp bảo vệ phản xạ tự động: khi người điều khiển cố tình bấm ga lao vào vật cản, cảm biến hồng ngoại trên xe sẽ can thiệp cưỡng bức ngắt động cơ để chống va chạm.

#### b. Danh Sách Thiết Bị Cần Dùng
* **Thiết bị đã có sẵn trong kho:**
  - 1× Bo mạch **ESP32 DevKit V1 (30 chân)**.
  - 1× Module cầu H **L298N** + 2× **TT Motor DC**.
  - 1× Module cảm biến tránh vật cản hồng ngoại **LM393 IR Obstacle Sensor** (Cảm biến phanh khẩn cấp).
  - 1× Màn hình **LED 7 đoạn 4 số TM1637** (Hiển thị tốc độ % và trạng thái kết nối Wi-Fi/BLE).
  - 1× Đèn **LED RGB** (Báo trạng thái: Xanh lá = Sẵn sàng, Vàng = Đang kết nối, Đỏ = Phanh khẩn cấp).
* **Thiết bị cần mua bổ sung:**
  - 1× Khung xe 2WD + Hộp 2 pin 18650 (Tương tự MOB-01).

#### c. Điểm Nổi Bật Về Phần Mềm
- ESP32 chạy chế độ **Wi-Fi SoftAP** (tự phát mạng Wi-Fi tên `Rover-AP`) kết hợp **AsyncWebServer** và **WebSocket**. Điện thoại kết nối vào Wi-Fi và truy cập `http://192.168.4.1` để mở giao diện Touch Joystick bằng HTML5/CSS hiện đại.
- Định kỳ 100ms, ESP32 gửi ngược thông số về trình duyệt: trạng thái pin ước tính, cảm biến hồng ngoại, tốc độ motor hiện tại.

---

### 2.3 Đề Tài MOB-03: Bàn Xoay Quét 3D & Chụp Ảnh Sản Phẩm Chính Xác (Precision Stepper Turntable)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/mob-03-precision-stepper-turntable`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/mob-03-precision-stepper-turntable ../worktrees/mob-03-precision-stepper-turntable main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Thiết bị phòng lab hỗ trợ chụp ảnh sản phẩm xoay 360° tự động để dựng mô hình 3D (Photogrammetry), làm catalogue thương mại điện tử, hoặc xoay mẫu vật quét cảm biến góc tròn.
- Độ chính xác cực cao nhờ động cơ bước kết hợp hộp giảm tốc 1:64 (sai số góc $< 0.1^\circ$).

#### b. Danh Sách Thiết Bị Cần Dùng
* **Thiết bị đã có sẵn trong kho (Sẵn sàng 100%):**
  - 1× Bo mạch **ESP32 DevKit V1 (30 chân)**.
  - 1× Động cơ bước **28BYJ-48 (5V)** kèm bo công suất **ULN2003 Driver**.
  - 1× **Bàn phím ma trận 4x4 (4x4 Matrix Keypad)** (Nhập góc quay mong muốn, ví dụ: 45°, 90°, 360°).
  - 1× Màn hình **OLED 0.96" SSD1306** (Hiển thị chế độ xoay, số khung hình, góc hiện tại).
  - 1× Module **Relay 1 kênh hoặc 2 kênh 5V** (Mô phỏng chân kích chụp ảnh màn trập máy ảnh Shutter Trigger).
  - 1× Module **Đồng hồ thời gian thực RTC DS3231/DS1307** (Đo chu kỳ và log thời gian chụp).
* **Thiết bị cần mua bổ sung:** Không bắt buộc. Có thể tận dụng bìa carton hoặc đĩa tròn mica gắn lên trục động cơ bước.

---

### 2.4 Đề Tài MOB-04: Xe AGV Tự Động Định Tuyến & Giao Hàng Điểm Trạm RFID (Automated Guided Vehicle)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/mob-04-rfid-guided-agv`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/mob-04-rfid-guided-agv ../worktrees/mob-04-rfid-guided-agv main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Mô phỏng mô hình xe tự hành dẫn đường tự động (AGV - Automated Guided Vehicle) phổ biến trong các nhà máy thông minh (Smart Factory/Amazon Robotics). Xe chạy tuần tra dọc hành lang hoặc line định sẵn, khi chạy qua thẻ RFID dán dưới sàn (đóng vai trò điểm dừng Station), xe tự dừng lại bốc dỡ hàng, kích hoạt servo mở chốt hàng hóa và gửi thông báo về máy chủ quản lý kho.

#### b. Danh Sách Thiết Bị Cần Dùng
* **Thiết bị đã có sẵn trong kho:**
  - 1× Bo mạch **ESP32 DevKit V1 (30 chân)**.
  - 1× Module **L298N** + 2× **TT Motor DC**.
  - 1× Module đọc thẻ **RFID RC522 (13.56MHz)** (Đặt quay xuống mặt sàn xe để quét thẻ).
  - 1× Động cơ servo **SG90** (Mở gạt thùng hàng tại trạm).
  - 1× Màn hình **LCD 1602 kèm I2C** (Gắn trên nóc xe hiển thị: Tên Trạm, Trạng Thái Giao Hàng).
  - 1× Module cảm biến hồng ngoại **LM393 IR Obstacle** (Dò chướng ngại vật phía trước đầu xe).
* **Thiết bị cần mua bổ sung:**
  - 1× Khung xe 2WD + Hộp 2 pin 18650.
  - Vài thẻ xu từ RFID Mifare 1K bổ sung dán tại các trạm *(Giá ~5.000 đ/thẻ)*.

---

## 3. Nhóm 2: Điều Khiển Bằng Giọng Nói & Xử Lý Âm Thanh

### 3.1 Đề Tài VOI-01: Trạm Điều Khiển Giọng Nói Hai Chiều (Voice Smart Hub)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/voi-01-voice-smart-hub`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/voi-01-voice-smart-hub ../worktrees/voi-01-voice-smart-hub main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Trở thành "Loa thông minh gia đình" (tương tự Google Home / Amazon Echo thu nhỏ) điều khiển các thiết bị điện trong nhà hoàn toàn bằng khẩu lệnh Tiếng Việt hoặc Tiếng Anh: bật/tắt quạt (Relay), mở/đóng rèm cửa (Servo), hỏi nhiệt độ độ ẩm trong phòng (DHT11).
- Trạm có khả năng phản hồi lại bằng giọng nói tổng hợp (Text-to-Speech) qua loa, báo cáo trạng thái phòng tức thời.

#### b. Danh Sách Thiết Bị Cần Dùng
* **Thiết bị đã có sẵn trong kho:**
  - 1× Bo mạch **ESP32 DevKit V1 (30 chân)**.
  - 1× Module **Relay 2 kênh 5V** (Điều khiển tải đèn / quạt điện).
  - 1× Động cơ servo **SG90** (Mô phỏng cơ cấu đóng mở chốt cửa / rèm).
  - 1× Cảm biến nhiệt độ độ ẩm **DHT11**.
  - 1× Màn hình **OLED 0.96" SSD1306** (Hiển thị text nhận dạng và biểu cảm trợ lý ảo).
  - 1× Đèn **LED RGB** (Chỉ thị trạng thái: Đang lắng nghe, Đang suy nghĩ, Đang phản hồi).
* **Thiết bị cần mua bổ sung (Rất quan trọng cho mảng Voice):**
  - 1× **Microphone số giao tiếp I2S INMP441** *(Giá ~30.000 – 40.000 đ)*.
  - 1× **Module khuếch đại âm thanh số I2S DAC MAX98357A** (3W Class D) *(Giá ~35.000 – 50.000 đ)*.
  - 1× **Loa mini 3W 4Ω hoặc 8Ω** *(Giá ~15.000 – 20.000 đ)*.

#### c. Kiến Trúc Hoạt Động & Giao Tiếp I2S
```
[Micro I2S INMP441] ────(I2S RX: SCK, WS, SD)────▶ [ESP32 Core 0: Ghi nhận luồng PCM 16kHz]
                                                             │
                                                    (Truyền WebSocket / REST)
                                                             │
                                                             ▼
                                                [Cloud STT Service / Local Server]
                                                (Whisper / Google STT / Wit.ai)
                                                             │
                                                    (Trả về Intent JSON)
                                                             ▼
[Thiết bị ngoại vi] ◀──(Đóng Relay / Kéo Servo)── [ESP32 Core 1: Phân tích & Thực thi]
                                                             │
                                                    (Nhận Stream MP3/WAV TTS)
                                                             ▼
[Loa Mini 3W] ◀────(Âm thanh analog)──── [I2S DAC MAX98357A] ◀──(I2S TX)── [ESP32]
```

---

### 3.2 Đề Tài VOI-02: Công Tắc Kích Hoạt Bằng Nhịp Vỗ Tay Đa Kênh (Clap Switch)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/voi-02-acoustic-clap-switch`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/voi-02-acoustic-clap-switch ../worktrees/voi-02-acoustic-clap-switch main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Thiết bị gia dụng tiện ích cho phòng ngủ, phòng khách hoặc người già/người hạn chế vận động: vỗ tay 1 tiếng để bật/tắt đèn ngủ, vỗ tay 2 tiếng nhịp nhanh để bật quạt, vỗ tay 3 tiếng để tắt toàn bộ thiết bị.
- Dự án khai thác **trực tiếp 100% linh kiện sẵn có** mà không cần mua thêm bất kỳ phụ kiện âm thanh nào.

#### b. Danh Sách Thiết Bị Cần Dùng (Sẵn sàng 100%)
- 1× Bo mạch **ESP32 DevKit V1 (30 chân)**.
- 1× Cảm biến âm thanh **Sound Sensor HW-484 (LM393)** (Sử dụng ngõ ra số `DO` kết hợp ngắt ngoài `attachInterrupt`).
- 1× Module **Relay 2 kênh 5V**.
- 1× Còi bíp **Passive Buzzer** (Phát âm xác nhận: 1 bíp = nhận diện clap 1, 2 bíp ngắn = thực thi lệnh).
- 1× Màn hình **OLED 0.96" SSD1306** (Vẽ đồng hồ đo mức nhạy âm thanh và hiển thị trạng thái relay).

#### c. Thuật Toán Lọc Nhiễu Nhịp Vỗ Tay (Acoustic Debouncing Algorithm)
- Âm thanh vỗ tay có đặc trưng: biên độ sóng tăng dốc rất nhanh (< 10ms) sau đó tắt dần.
- Thuật toán định thời `millis()`:
  - Khi phát hiện cạnh xuống ngắt tại chân `DO`, kích hoạt cửa sổ phân tích (Window) $600\text{ms}$.
  - Áp dụng thời gian trễ chống rung (Blanking period) $120\text{ms}$ sau mỗi tiếng vỗ để triệt tiêu tiếng vang dội của chính tiếng vỗ đó.
  - Đếm tổng số đỉnh xung trong cửa sổ: Nếu $N=1 \rightarrow$ Đảo Relay 1; Nếu $N=2 \rightarrow$ Đảo Relay 2; Nếu $N \ge 3 \rightarrow$ Tắt cả 2 Relay.

---

### 3.3 Đề Tài VOI-03: Máy Phát Nhạc Điện Tử & Bộ Đọc Giọng Nói Tổng Hợp (Synthesizer & TTS Talkie)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/voi-03-synth-tts-talkie`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/voi-03-synth-tts-talkie ../worktrees/voi-03-synth-tts-talkie main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Tạo hệ thống chuông báo đa âm điệu, máy phát thông báo bằng giọng nói ngoại tuyến không cần internet cho thiết bị IoT (ví dụ: máy đọc "Nhiệt độ hiện tại ba mươi độ C", "Cảnh báo mở cửa", "Đã xác thực thẻ thành công").

#### b. Danh Sách Thiết Bị Cần Dùng
* **Phiên bản Offline 100% (Đã có sẵn linh kiện):**
  - Bo ESP32 + **Passive Buzzer** (Sử dụng thư viện `Talkie` nén âm vị Linear Predictive Coding - LPC) hoặc phát các bản nhạc đa âm qua kênh phần cứng LEDC PWM (`ledcWriteTone`).
  - **Bàn phím ma trận 4x4** đóng vai trò bàn phím đàn Organ điện tử 16 nốt.
  - **Biến trở xoay 10kΩ** đóng vai trò núm điều chỉnh cao độ (Pitch Bend) hoặc âm lượng.
  - Màn hình **TM1637** hiển thị nốt nhạc (C4, D4, E4...) đang ngân vang.

---

## 4. Nhóm 3: Ứng Dụng Trí Tuệ Nhân Tạo (Edge AI & Cloud-Connected AI)

### 4.1 Đề Tài AI-01: Thiết Bị Nhận Diện & Phân Loại Tiếng Động Bất Thường (Edge AI Sound Anomaly Detector)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/ai-01-edge-sound-anomaly-detector`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/ai-01-edge-sound-anomaly-detector ../worktrees/ai-01-edge-sound-anomaly-detector main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Thiết bị an ninh cao cấp lắp đặt tại gia đình hoặc xưởng máy: Tự động lắng nghe và phân loại tiếng động môi trường 24/7 trực tiếp trên vi điều khiển mà không cần gửi âm thanh ra ngoài internet, đảm bảo tính riêng tư tuyệt đối (Privacy-First).
- Phát hiện các tình huống nguy hiểm:
  - Tiếng kính vỡ (Glass Breaking) $\rightarrow$ Báo trộm đột nhập.
  - Tiếng trẻ em khóc (Baby Crying) $\rightarrow$ Thông báo cho phụ huynh.
  - Tiếng chó sủa hoặc tiếng còi báo động khói $\rightarrow$ Kích hoạt cảnh báo.

#### b. Danh Sách Thiết Bị Cần Dùng
* **Thiết bị đã có sẵn trong kho:**
  - 1× Bo mạch **ESP32 DevKit V1 (30 chân)** (Nhờ chip 2 nhân 240MHz hỗ trợ chạy mô hình mạng nơ-ron thu nhỏ).
  - 1× Màn hình **LED Ma trận 8x8 MAX7219** (Hiển thị icon động tương ứng: Icon ngọn lửa, Icon mặt trời, Icon chuông).
  - 1× Module **Relay 2 kênh 5V** + 1× **Active Buzzer**.
* **Thiết bị cần mua bổ sung:**
  - 1× **Micro I2S INMP441** *(Giá ~35.000 đ)*.

#### c. Quy Trình Huấn Luyện & Triển Khai Mô Hình (Edge Impulse Pipeline)
1. **Thu thập dữ liệu mẫu (Data Acquisition):** Dùng ESP32 nạp sketch ghi âm I2S 16kHz truyền dữ liệu mẫu qua Serial lên nền tảng **Edge Impulse Studio**.
2. **Tiền xử lý tín hiệu số (DSP):** Trích xuất ma trận đặc trưng phổ âm thanh **MFE (Mel-Frequency Energy)** hoặc **MFCC**.
3. **Huấn luyện mô hình (Neural Network):** Xây dựng mạng nơ-ron tích chập 1D/2D (1D-CNN) kích thước nhẹ (< 40KB RAM, < 150KB Flash).
4. **Triển khai C++ Cốt lõi:** Biên dịch mô hình sang thư viện C++ bằng TensorFlow Lite for Microcontrollers (TFLite-Micro), nạp trực tiếp vào firmware ESP32 qua PlatformIO.

---

### 4.2 Đề Tài AI-02: Hệ Thống Dự Báo & Tối Ưu Môi Trường Bằng Mô Hình AI Riêng (Predictive Climate Optimization)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/ai-02-predictive-climate-optimizer`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/ai-02-predictive-climate-optimizer ../worktrees/ai-02-predictive-climate-optimizer main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Phù hợp trực tiếp với nhu cầu: **"Tôi có sẵn model AI"**.
- Ứng dụng trong việc dự báo thời tiết cục bộ, quản lý vi khí hậu phòng server, vườn ươm nông nghiệp công nghệ cao. Thay vì chỉ phản ứng cơ học (nhiệt độ > 30°C mới bật quạt), mô hình AI tiếp nhận chuỗi dữ liệu thời gian (Time-series) gồm Nhiệt độ, Độ ẩm, Cường độ sáng, Tốc độ biến thiên trong 2 giờ qua và dự báo xu hướng tiếp theo để điều chỉnh quạt và rèm che đón đầu, tiết kiệm điện năng tối đa.

#### b. Danh Sách Thiết Bị Cần Dùng (Sẵn sàng 100%)
- 1× Bo mạch **ESP32 DevKit V1 (30 chân)**.
- 1× Cảm biến nhiệt độ & độ ẩm **DHT11**.
- 1× Module quang trở **LDR Sensor LM393**.
- 1× Module thời gian thực **RTC DS3231/DS1307** (Đóng dấu thời gian chuẩn Timestamp cho tập dữ liệu).
- 1× Màn hình **OLED 0.96" SSD1306** (Vẽ đồ thị xu hướng nhiệt độ và kết quả dự báo AI).
- 1× Module **Relay 2 kênh 5V** (Điều khiển tải quạt / đèn sưởi theo khuyến nghị AI).

#### c. Sơ Đồ Kiến Trúc Kết Nối Với Model AI Của Bạn
```
[Cảm biến DHT11 + LDR + RTC] 
             │
      (Đọc dữ liệu định kỳ mỗi 5 giây)
             ▼
      [ESP32 DevKit V1]
             │
      (Gói tin JSON qua WebSocket WSS bảo mật)
             ▼
[Server AI Riêng Của Bạn (Python FastAPI / Node.js)]
   └── Chạy Mô Hình Học Máy (Scikit-Learn / PyTorch / LSTM)
   └── Phân tích chuỗi số liệu thời gian thực
   └── Dự báo: "Nhiệt độ dự kiến tăng +3°C trong 30 phút tới, xu hướng oi bức"
   └── Khuyến nghị: {"relay1": true, "fan_speed": 80, "alert": false}
             │
      (Phản hồi kết quả điều khiển)
             ▼
      [ESP32 DevKit V1] ──▶ Kích hoạt Relay đóng mở tải + Cập nhật màn hình OLED
```

---

### 4.3 Đề Tài AI-03: Trợ Lý Ảo Vật Lý AI Đa Phương Thức (Physical LLM Agent)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/ai-03-physical-llm-agent`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/ai-03-physical-llm-agent ../worktrees/ai-03-physical-llm-agent main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Đây là đề tài tích hợp đỉnh cao: Kết hợp một mô hình ngôn ngữ lớn (LLM như GPT-4o, Claude 3.5, Gemini 1.5 Flash hoặc mô hình Local LLM Llama-3 qua Ollama) với thực thể phần cứng vật lý qua cơ chế **Tool Calling / Function Calling**.
- Người dùng trò chuyện tự nhiên: *"Chào bạn, phòng tôi đang hơi tối và bí bách, bạn kiểm tra giúp tôi nhé"*.
- Mô hình AI suy luận tự động sinh lời gọi hàm (Function Call) để ESP32:
  1. Đọc cảm biến LDR và DHT11.
  2. Phát hiện trời tối $\rightarrow$ Bật đèn Relay 1.
  3. Phát hiện phòng bí $\rightarrow$ Mở góc servo 90° nâng rèm cửa.
  4. Trả lời bằng giọng nói qua loa: *"Tôi đã bật đèn và mở hé cửa sổ giúp bạn rồi nhé!"*.

#### b. Danh Sách Thiết Bị Cần Dùng
* **Thiết bị đã có sẵn:** ESP32, Relay 2CH, Servo SG90, DHT11, LDR, OLED SSD1306, Đèn LED RGB.
* **Thiết bị bổ sung:** Micro I2S INMP441, Mạch I2S DAC MAX98357A, Loa mini 3W.

---

### 4.4 Đề Tài AI-04: Bàn Cảm Ứng Nhận Diện Cử Chỉ Cào / Vuốt Bằng Machine Learning

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/ai-04-capacitive-touch-ml`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/ai-04-capacitive-touch-ml ../worktrees/ai-04-capacitive-touch-ml main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Thiết kế bàn phím điều khiển ẩn vô hình dưới mặt bàn gỗ hoặc bề mặt kính mica: Người dùng vuốt ngón tay từ trái sang phải để chuyển bài hát, gõ 2 nhịp để bật đèn, vuốt vòng tròn để tăng giảm âm lượng.
- Ứng dụng công nghệ **Capacitive Touch Pins** có sẵn trong chip ESP32 (không cần module cảm biến ngoài).

#### b. Danh Sách Thiết Bị Cần Dùng (Sẵn sàng 100%)
- 1× Bo mạch **ESP32 DevKit V1 (30 chân)** (Sử dụng 4 đến 6 chân cảm ứng: `T0` (GPIO4), `T4` (GPIO13), `T5` (GPIO12), `T6` (GPIO14), `T7` (GPIO27)).
- 4–6 miếng giấy bạc dán dưới bàn làm các điện cực cảm ứng.
- 1× Màn hình **LED Ma trận 8x8 MAX7219** (Vẽ mũi tên di chuyển tương ứng theo cử chỉ vuốt nhận diện được).
- 1× Module **Relay 2 kênh 5V**.

---

## 5. Nhóm 4: Cơ Điện Tử & Robotics

### 5.1 Đề Tài ROB-01: Cánh Tay Robot 4 Bậc Tự Do (4-DOF Robotic Arm)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/rob-01-4dof-robotic-arm`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/rob-01-4dof-robotic-arm ../worktrees/rob-01-4dof-robotic-arm main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Mô phỏng cánh tay robot công nghiệp gắp phôi tự động trong dây chuyền sản xuất cơ khí.
- Người học tiếp cận bài toán động học thuận (Forward Kinematics) và động học ngược (Inverse Kinematics) cơ bản: Tính toán góc quay của từng khớp vai, khớp khuỷu và góc kẹp để đầu gắp chạm đúng tọa độ không gian $(X, Y, Z)$ mong muốn.

#### b. Danh Sách Thiết Bị Cần Dùng
* **Thiết bị đã có sẵn trong kho:**
  - 1× Bo mạch **ESP32 DevKit V1 (30 chân)**.
  - 1× Động cơ servo **SG90** (Dùng cho khớp kẹp Gripper).
  - 1× **Biến trở xoay 10kΩ** (Dùng làm núm điều khiển góc manual trên breadboard).
  - 1× **Bàn phím ma trận 4x4** (Dùng gõ tọa độ hoặc phím nóng chọn góc lưu sẵn).
  - 1× Màn hình **LCD 1602 kèm I2C** (Hiển thị góc $\theta_1, \theta_2, \theta_3, \theta_4$ của 4 khớp).
* **Thiết bị cần mua bổ sung:**
  - 3× **Động cơ Servo loại tốt (SG90 hoặc bánh răng kim loại MG90S)** *(Giá ~60.000 đ cho 3 chiếc)*.
  - 1× **Khung cánh tay robot mica 4-DOF hoặc in 3D** kèm ốc vít *(Giá ~90.000 – 140.000 đ)*.
  - 1× **Nguồn adapter 5V 3A rời** nuôi 4 động cơ servo (Không cấp từ ESP32) *(Giá ~40.000 đ)*.
  - *(Tùy chọn nâng cấp):* 1× Mạch mở rộng 16 kênh PWM I2C **PCA9685** *(Giá ~35.000 đ)* để giải phóng hoàn toàn chân GPIO cho ESP32.

---

### 5.2 Đề Tài ROB-02: Dây Chuyền Phân Loại & Gắp Vật Thể Tự Động RFID (Automated RFID Sorting Mechanism)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/rob-02-rfid-sorting-mechanism`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/rob-02-rfid-sorting-mechanism ../worktrees/rob-02-rfid-sorting-mechanism main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Hệ thống tự động hóa nhà kho thông minh: Vật thể đi qua bàn kiểm tra, đầu đọc RFID quét mã định danh loại hàng (ví dụ: Hàng loại A, Loại B, Hàng hỏng). Động cơ bước xoay bàn phân loại đến đúng ô chứa, sau đó cánh tay servo gắp vật thể thả vào khay tương ứng.

#### b. Danh Sách Thiết Bị Cần Dùng
* **Thiết bị đã có sẵn:**
  - ESP32 DevKit V1.
  - 1× Động cơ bước **28BYJ-48 + Bo đệm ULN2003** (Xoay đĩa phân loại).
  - 1× Module đầu đọc thẻ **RFID RC522 (3.3V)** (Quét thẻ gắn trên vật phẩm).
  - 1× Động cơ servo **SG90** (Gạt/kẹp vật phẩm).
  - 1× Cảm biến siêu âm **HC-SR04** (Phát hiện có vật thể xuất hiện trên bàn cân để kích hoạt đọc thẻ).
  - 1× Màn hình **OLED 0.96" SSD1306** (Hiển thị UID thẻ, loại sản phẩm và số lượng thống kê).
* **Thiết bị cần mua bổ sung:**
  - Thẻ xu hoặc nhãn dán RFID Sticker nhỏ dán lên hộp sản phẩm mini *(Giá ~15.000 đ/tập 5 cái)*.

---

### 5.3 Đề Tài ROB-03: Tháp Camera Pan-Tilt Tự Động Bám Đuổi Đối Tượng (Intelligent Sentry Camera)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/rob-03-sentry-pan-tilt-cam`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/rob-03-sentry-pan-tilt-cam ../worktrees/rob-03-sentry-pan-tilt-cam main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Tháp canh an ninh thông minh: Sử dụng động cơ bước quét 360° theo phương ngang (Pan) và servo gật gù góc lên xuống (Tilt). Khi cảm biến chuyển động PIR phát hiện có người xâm nhập ở một hướng, tháp tự quay súng camera về hướng đó, bật đèn pha chiếu rọi (LED công suất qua Relay) và stream video trực tiếp về điện thoại.

#### b. Danh Sách Thiết Bị Cần Dùng
* **Thiết bị đã có sẵn:**
  - 1× Bo mạch **ESP32 DevKit V1 (30 chân)**.
  - 1× Động cơ bước **28BYJ-48 + Bo ULN2003** (Trục xoay ngang 360° vô tận).
  - 1× Động cơ servo **SG90** (Trục xoay đứng gật lên xuống $0^\circ–90^\circ$).
  - 1× Cảm biến chuyển động hồng ngoại **PIR HC-SR501**.
  - 1× Module **Relay 1 kênh 5V** (Bật đèn pha rọi ban đêm).
  - 1× Còi bíp **Active Buzzer** (Hú còi xua đuổi).
* **Thiết bị cần mua bổ sung:**
  - 1× Bo mạch **ESP32-CAM (kèm camera OV2640)** để ghi hình truyền stream Wi-Fi *(Giá ~85.000 đ)*.
  - 1× Bộ giá đỡ gimbal Pan-Tilt 2 bậc bằng nhựa *(Giá ~25.000 đ)*.

---

## 6. Nhóm 5: Nhà Thông Minh & An Ninh Tự Động

### 6.1 Đề Tài SMH-01: Hệ Thống An Ninh Giám Sát Đa Vùng Cảnh Báo Tức Thời (Multi-Zone Defense System)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/smh-01-multizone-defense-system`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/smh-01-multizone-defense-system ../worktrees/smh-01-multizone-defense-system main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Dự án an ninh toàn diện bảo vệ ngôi nhà với 3 vùng giám sát:
  - **Vùng 1 (Hành lang):** Cảm biến chuyển động hồng ngoại PIR HC-SR501.
  - **Vùng 2 (Cửa sổ / Hàng rào):** Tia hồng ngoại tránh vật cản LM393 IR Beam.
  - **Vùng 3 (Phòng khách):** Cảm biến âm thanh HW-484 nghe tiếng cậy cửa, vỡ kính.
- Cung cấp cơ chế Bật/Tắt bảo vệ (Arm/Disarm) bằng thẻ từ RFID hoặc mật mã bàn phím 4x4. Khi có xâm nhập: Hú còi báo động, nhấp nháy đèn cảnh báo đỏ, ghi log thời gian chính xác vào RTC và gửi tin nhắn cảnh báo tức thì qua Telegram Bot tới điện thoại gia chủ.

#### b. Danh Sách Thiết Bị Cần Dùng (Sẵn sàng 100% trong kho!)
- 1× Bo mạch **ESP32 DevKit V1 (30 chân)**.
- 1× Cảm biến chuyển động **PIR HC-SR501** (Nguồn 5V từ VIN, chân OUT nối GPIO 4).
- 1× Cảm biến vật cản hồng ngoại **LM393 IR Sensor** (GPIO 16).
- 1× Cảm biến âm thanh **Sound Sensor HW-484** (GPIO 17).
- 1× Đầu đọc thẻ từ **RFID RC522 (3.3V)** (Bus VSPI).
- 1× **Bàn phím ma trận 4x4** (8 chân GPIO).
- 1× Đồng hồ thời gian thực **RTC DS3231/DS1307** (I2C SDA/SCL).
- 1× Màn hình **OLED 0.96" SSD1306** (Hiển thị trạng thái an ninh, log vi phạm).
- 1× Module **Relay 2 kênh 5V** (Kích còi hú công suất lớn hoặc đèn pha).
- 1× Đèn **LED RGB** + 1× **Buzzer**.
* **Thiết bị bổ sung:** Không cần mua thêm.

---

### 6.2 Đề Tài SMH-02: Khóa Cửa Điện Tử Thông Minh 3 Lớp Bảo Mật (Tri-Factor Smart Access Lock)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/smh-02-trifactor-smart-lock`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/smh-02-trifactor-smart-lock ../worktrees/smh-02-trifactor-smart-lock main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Thiết kế bộ khóa cửa điện tử tích hợp vào cửa phòng riêng hoặc tủ đồ cá nhân an toàn cao:
  - Phương thức 1: Quẹt thẻ từ **RFID Mifare 13.56MHz**.
  - Phương thức 2: Nhập mã PIN cá nhân từ **Bàn phím ma trận 4x4**.
  - Phương thức 3: Mở khóa từ xa qua **Giao diện Web / Ứng dụng điện thoại** bảo mật mã hóa.
- Động cơ Servo SG90 đóng vai trò chốt khóa cơ khí (xoay 0° = Khóa, 90° = Mở khóa). Màn hình TM1637 hoặc OLED hiển thị đồng hồ thời gian đếm ngược tự động khóa lại sau 5 giây.

#### b. Danh Sách Thiết Bị Cần Dùng (Sẵn sàng 100% trong kho!)
- ESP32, RFID RC522, Keypad 4x4, Servo SG90, OLED SSD1306, RTC DS3231, Active Buzzer. Không cần mua thêm.

---

### 6.3 Đề Tài SMH-03: Thiết Bị Đo Điện Năng & Tự Ngắt Bảo Vệ Quá Tải (Smart Energy Meter)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/smh-03-smart-energy-meter`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/smh-03-smart-energy-meter ../worktrees/smh-03-smart-energy-meter main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Đo lường công suất tiêu thụ điện của các thiết bị trong phòng (quạt, đèn, máy tính, tủ lạnh mini), tính toán chỉ số tiêu thụ điện (kWh) và ước tính tiền điện hàng tháng.
- Tự động ngắt relay bảo vệ khi phát hiện dòng điện vượt ngưỡng cài đặt (bảo vệ chống cháy chập, quá tải thiết bị).

#### b. Danh Sách Thiết Bị Cần Dùng (Sẵn sàng 100%)
- 1× Bo mạch **ESP32 DevKit V1 (30 chân)**.
- 1× Cảm biến dòng điện hiệu ứng Hall **ACS712 (Phiên bản 5A/20A/30A)**.
- 1× Module **Relay 1 kênh hoặc 2 kênh 5V** (Đóng cắt tải an toàn < 24V DC).
- Điện trở $1\text{k}\Omega$ và $2\text{k}\Omega$ (Mắc cầu phân áp $\frac{2}{3}$ hạ áp chân OUT ACS712 từ tối đa 4.5V xuống dưới 3.0V để cấp vào kênh **ADC1 GPIO 32/33** của ESP32).
- 1× Màn hình **LCD 1602 kèm I2C** (Hiển thị Ampe, Watt, kWh real-time).
- ⚠️ **Lưu ý an toàn tuyệt đối:** Chỉ thử nghiệm với nguồn điện DC an toàn (như ắc quy 12V, nguồn máy tính 12V, pin mặt trời). **Tuyệt đối không đấu nối trực tiếp vào điện lưới xoay chiều 220V AC**.

---

### 6.4 Đề Tài SMH-04: Trạm Giám Sát & Điều Hòa Nhà Kính Mini Tự Động (Automated Micro-Greenhouse)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/smh-04-micro-greenhouse`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/smh-04-micro-greenhouse ../worktrees/smh-04-micro-greenhouse main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Trạm điều dưỡng cây cảnh thông minh tự động hoàn toàn: Theo dõi nhiệt độ & độ ẩm không khí (DHT11), độ ẩm của đất, cường độ ánh sáng mặt trời (LDR). Tự động bật máy bơm tưới nước khi đất khô và kéo servo mở rèm che khi nắng gắt.

#### b. Danh Sách Thiết Bị Cần Dùng
* **Thiết bị đã có sẵn:** ESP32, DHT11, LDR Sensor, Relay 2CH, Servo SG90, OLED SSD1306, RTC DS3231.
* **Thiết bị cần mua bổ sung:**
  - 1× **Cảm biến độ ẩm đất (Soil Moisture Sensor)** loại chống ăn mòn điện dung (Capacitive v1.2) *(Giá ~20.000 đ)*.
  - 1× **Máy bơm nước chìm mini 5V DC** kèm 1m ống dẻo *(Giá ~25.000 đ)*.

---

## 7. Nhóm 6: Nghiên Cứu Mở Rộng & Đa Vi Điều Khiển

### 7.1 Đề Tài EXP-01: Trạm Điểm Danh Không Chạm RFID & Hiển Thị Đồ Họa Matrix

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/exp-01-rfid-matrix-attendance`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/exp-01-rfid-matrix-attendance ../worktrees/exp-01-rfid-matrix-attendance main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Ứng dụng làm máy chấm công/điểm danh học sinh sinh viên tại cửa lớp học: Quét thẻ học sinh $\rightarrow$ LED ma trận MAX7219 chạy chữ chào mừng hoặc biểu tượng mặt cười $\rightarrow$ Màn hình OLED hiển thị Mã Sinh Viên, Họ Tên và giờ vào lớp lấy từ RTC DS3231 $\rightarrow$ ESP32 đẩy bản ghi về Google Sheets / Database qua Wi-Fi.

#### b. Danh Sách Thiết Bị Cần Dùng (Sẵn sàng 100%)
- Bo ESP32, Module RFID RC522, LED Ma trận 8x8 MAX7219, Màn hình OLED SSD1306, RTC DS3231, Active Buzzer.

---

### 7.2 Đề Tài EXP-02: Hộp Câu Đố Tương Tác Điện Tử (Interactive Escape Room Puzzle Box)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/exp-02-escape-room-puzzle-box`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/exp-02-escape-room-puzzle-box ../worktrees/exp-02-escape-room-puzzle-box main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Trò chơi trí tuệ giải đố tương tác vật lý (Phòng thoát hiểm Escape Room hoặc hộp quà tặng công nghệ): Người chơi muốn mở được nắp hộp (do servo SG90 chốt giữ) phải giải lần lượt chuỗi nhiệm vụ:
  1. *Thử thách 1:* Nhập đúng mật mã dãy số trên Bàn phím 4x4.
  2. *Thử thách 2:* Vặn chiết áp xoay 10kΩ đến đúng giá trị góc bí mật hiển thị trên LED 7 đoạn TM1637.
  3. *Thử thách 3:* Vỗ tay đúng nhịp điệu (Clap pattern) vào cảm biến âm thanh HW-484.
  4. *Thử thách 4:* Quẹt đúng thẻ từ RFID được cất giấu.
- Khi hoàn thành cả 4 bước: Còi buzzer phát giai điệu chúc mừng Victory Melody, LED ma trận 8x8 nháy hình trái tim và Servo gạt chốt mở bung nắp hộp!

#### b. Danh Sách Thiết Bị Cần Dùng (Sẵn sàng 100% linh kiện trong kho!)
- ESP32, Keypad 4x4, Biến trở 10kΩ, TM1637, Sound Sensor HW-484, RFID RC522, Servo SG90, MAX7219 8x8, Passive Buzzer, OLED SSD1306.

---

### 7.3 Đề Tài EXP-03: Hệ Thống Phân Tán Đa Vi Điều Khiển (Distributed Multi-MCU Communication Lab)

> 🌿 **Đặc tả Git Worktree:**
> - **Tên Branch chỉ định:** `feat/exp-03-multi-mcu-communication`
> - **Lệnh khởi tạo Worktree:**
>   ```bash
>   git worktree add -b feat/exp-03-multi-mcu-communication ../worktrees/exp-03-multi-mcu-communication main
>   ```

#### a. Tính Ứng Dụng Thực Tiễn
- Tận dụng trọn vẹn cả 3 bo mạch vi điều khiển bạn đang sở hữu (**ESP32**, **STM32F4 Black Pill**, **Arduino Nano**) để xây dựng mô hình mạng nhúng phân tán chuyên nghiệp (tương tự kiến trúc mạng ECU trên xe hơi ô tô):
  - **ESP32 (Master Gateway):** Quản lý kết nối Wi-Fi, host Web Dashboard, đồng bộ thời gian NTP và phát lệnh điều phối.
  - **STM32F401/411 (Coprocessor xử lý toán học & Động cơ):** Xử lý thuật toán điều khiển góc quay động cơ bước tốc độ cao hoặc xử lý tín hiệu lọc số DSP nhờ nhân ARM Cortex-M4 mạnh mẽ 100MHz có FPU phần cứng.
  - **Arduino Nano (I/O Expander & Sub-Controller):** Đọc các cảm biến analog 5V trực tiếp (ACS712, biến trở, nút bấm), điều khiển relay đóng cắt tải công nghiệp.
- Mạch nạp **USB-UART HW-896** được dùng làm kênh theo dõi Serial độc lập cho STM32 hoặc Nano để debug song song trên máy tính.

#### b. Giao Thức Kết Nối Giữa Các Bo Mạch
```
                      ┌─────────────────────────────────────────┐
                      │        ESP32 DevKit V1 (Master)         │
                      │       Wi-Fi Web Server + Gateway        │
                      └────────────────────┬────────────────────┘
                                           │
                       ┌───────────────────┴───────────────────┐
                       │                                       │
            Bus I2C (3.3V Logic)                     Bus UART2 (Mức 3.3V)
                       │                                       │
                       ▼                                       ▼
        ┌─────────────────────────────┐        ┌───────────────────────────────┐
        │   STM32F4 Black Pill        │        │      Arduino Nano (5V)        │
        │   (Điều khiển Bước / Servo) │        │   (Đọc Cảm biến & Kích Relay) │
        └─────────────────────────────┘        └───────────────────────────────┘
                                                *Chân TX Nano (5V) qua phân áp
                                                 vào chân RX2 ESP32 (3.3V)
```

---

## 8. Cẩm Nang Mua Sắm Bổ Sung & Dự Toán Ngân Sách

Nếu bạn muốn mở rộng làm các đề tài thuộc nhóm **Xe di chuyển**, **Giọng nói chuyên sâu** hoặc **Robotics**, dưới đây là danh sách linh kiện gợi ý bổ sung theo từng gói tối ưu chi phí:

### Gói A: "Nâng Cấp Giọng Nói & AI Audio" (Mở khóa VOI-01, AI-01, AI-03)
| Linh kiện | Thông số khuyến nghị | Giá tham khảo | Tác dụng |
|:---|:---|:---:|:---|
| **Micro I2S INMP441** | Ngõ ra số 24-bit I2S, SNR 61dB | ~35.000 đ | Thu giọng nói trong trẻo phục vụ STT và Edge AI |
| **I2S DAC Amp MAX98357A** | Công suất 3.2W, chuẩn I2S số | ~40.000 đ | Khuếch đại âm thanh không bị rè nhiễu |
| **Loa mini 3W 4Ω/8Ω** | Đường kính 40mm hoặc khoang loa hộp | ~15.000 đ | Phát giọng nói phản hồi TTS / còi báo động |
| **Tổng Gói A:** | | **~90.000 đ** | |

### Gói B: "Nâng Cấp Xe Robot Tự Hành" (Mở khóa MOB-01, MOB-02, MOB-04)
| Linh kiện | Thông số khuyến nghị | Giá tham khảo | Tác dụng |
|:---|:---|:---:|:---|
| **Khung xe 2WD mica** | Gồm khung, 2 bánh xe, 1 bánh mắt trâu | ~55.000 đ | Lắp đặt động cơ vàng TT motor và bánh xe |
| **Hộp pin 2 cell 18650** | Có nắp đậy và công tắc ON/OFF | ~20.000 đ | Nguồn động lực cho motor L298N |
| **2 cell pin Li-ion 18650** | Điện áp 3.7V/cell (ghép nối tiếp = 7.4V) | ~70.000 đ | Cung cấp dòng xả cao không bị sụt áp |
| **Tổng Gói B:** | | **~145.000 đ** | |

### Gói C: "Nâng Cấp Robotics & Thị Giác Máy Tính" (Mở khóa ROB-01, ROB-03, SMH-04)
| Linh kiện | Thông số khuyến nghị | Giá tham khảo | Tác dụng |
|:---|:---|:---:|:---|
| **3× Động cơ Servo MG90S** | Bánh răng kim loại bền bỉ | ~75.000 đ | Dựng khớp cánh tay robot 4-DOF |
| **Bộ khung cánh tay robot** | Khung mica 4 khớp lắp ghép | ~90.000 đ | Kết cấu cơ khí cánh tay kẹp |
| **Module ESP32-CAM OV2640** | Tích hợp camera và khe cắm thẻ nhớ | ~85.000 đ | Nhận diện hình ảnh và stream camera Wi-Fi |
| **Cảm biến độ ẩm đất + Bơm** | Cảm biến điện dung + Bơm mini 5V | ~45.000 đ | Hoàn thiện đề tài Nhà kính mini |
| **Tổng Gói C:** | | **~295.000 đ** | |

---

## 9. Nguyên Tắc An Toàn & Chuẩn Ghép Nối ESP32 30-Pin

Mọi dự án trên khi triển khai bắt buộc phải tuân thủ nghiêm ngặt các quy chuẩn kỹ thuật của bo mạch **ESP32 DevKit V1 (30 chân)** (theo chuẩn quy định tại [AGENTS.md](file:///Users/toannguyen/Documents/esp32-learning/AGENTS.md)):

1. **Nguyên Tắc Logic 3.3V:**
   - Các chân GPIO của ESP32 **không có khả năng chịu áp 5V**.
   - Mọi tín hiệu 5V ngõ vào (như chân Echo của HC-SR04, chân TX của Arduino Nano, chân OUT của ACS712) **bắt buộc phải qua cầu phân áp** ($R_1 = 1\text{k}\Omega, R_2 = 2\text{k}\Omega$):
     $$V_{\text{out}} = V_{\text{in}} \times \frac{R_2}{R_1 + R_2} = 5\text{V} \times \frac{2\text{k}\Omega}{3\text{k}\Omega} \approx 3.33\text{V}$$
2. **Quy Tắc Cách Ly Nguồn Động Lực (Common GND):**
   - Động cơ DC, động cơ bước và servo khi khởi động ăn dòng từ 500mA – 1.5A và sinh ra xung điện áp cảm ứng âm ngược (Back-EMF).
   - **Tuyệt đối không cấp nguồn nuôi motor/servo từ chân 3V3 hay VIN của ESP32.** Luôn dùng nguồn pin hoặc adapter ngoài và **nối chung cực âm (GND) giữa nguồn ngoài và ESP32**.
3. **Quy Tắc Đọc Cảm Biến Analog Khi Có Wi-Fi:**
   - Khi bật Wi-Fi (hoặc Web Server / WebSocket), **toàn bộ 10 chân thuộc ADC2 (GPIO 4, 0, 2, 15, 13, 12, 14, 27, 25, 26) bị vô hiệu hóa**.
   - Mọi cảm biến analog (Cảm biến âm thanh AO, Cảm biến dòng ACS712, Biến trở xoay) **bắt buộc cắm vào nhóm ADC1 (GPIO 32, 33, 34, 35, 36/VP, 39/VN)**.
4. **Quy Tắc Chân Input-Only:**
   - Các chân **GPIO 34, 35, 36, 39** chỉ dùng làm ngõ vào, **không có trở kéo nội bộ `INPUT_PULLUP`** và không thể xuất mức HIGH/LOW.
5. **Cấm Dùng GPIO 6 Đến 11:**
   - Đây là các chân kết nối trực tiếp với bộ nhớ Flash SPI nội của chip ESP32. Can thiệp vào các chân này sẽ gây reset và treo chip ngay lập tức.
