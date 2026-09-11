# POC: IR Obstacle Avoidance Barrier (Cảm Biến Tiệm Cận & Đếm Không Chạm)

Bản Proof of Concept (POC) thực nghiệm ứng dụng **Module Cảm biến Tránh vật cản hồng ngoại (LM393 IR Obstacle Sensor)** kết hợp ngắt phần cứng (`Hardware Interrupt`) và thuật toán **Lockout Dead-Time** để xây dựng hệ thống tiệm cận không chạm (Touchless Trigger), cảnh báo barrier an toàn hoặc đếm sản phẩm băng chuyền trên **ESP32 DevKit V1 (30 chân)**.

---

## 1. Thông Số Tính Toán Kỹ Thuật

- **Module Tránh vật cản LM393:**
  - Phát chùm tia hồng ngoại qua LED phát và nhận tia phản xạ qua Photodiode.
  - Ngõ ra `OUT`: Mức `LOW` khi phát hiện vật cản (Active LOW), mức `HIGH` khi thông thoáng.
  - Nối vào **GPIO 27**: Hỗ trợ ngắt ngoài `attachInterrupt()` cạnh xuống (`FALLING`).
- **Khóa thời gian chết (Lockout Dead-Time):**
  - Ngăn ngừa hiện tượng mép vật thể rung cơ học hoặc người di chuyển chậm kích hoạt ngắt lặp lại nhiều lần trong cùng 1 sự kiện.
  - Khoảng thời gian khóa: $1000\text{ms}$.
- **Phản hồi tức thì:**
  - Bật đèn LED Xanh (GPIO 13 qua trở $220\Omega$) trong $400\text{ms}$.
  - Kích hoạt Active Buzzer (GPIO 14) kêu tiếng bíp ngắn $40\text{ms}$.
  - Tăng bộ đếm sản phẩm `objectCount++`.

---

## 2. Bản Đồ Nối Dây (Pinout Map)

| Chân ESP32 | Linh kiện | Chân linh kiện | Chức năng |
|---|---|---|---|
| **3V3** | IR Obstacle Sensor | `VCC` | Nguồn cấp 3.3V |
| **GND** | IR Obstacle Sensor | `GND` | Nối đất |
| **D27 (GPIO 27)** | IR Obstacle Sensor | `OUT` | Tín hiệu ngắt Active LOW |
| **D13 (GPIO 13)** | Điện trở $220\Omega$ $\rightarrow$ LED Xanh (A) | Anode (+) | Đèn hiển thị tiệm cận |
| **GND** | LED Xanh | Cathode (-) | Nối đất |
| **D14 (GPIO 14)** | Active Buzzer | Cực dương (+) | Còi phát tiếng bíp xác nhận |
| **GND** | Active Buzzer | Cực âm (-) | Nối đất |

---

## 3. Lệnh Thao Tác Dòng Lệnh (CLI-First)

### 3.1 Kiểm tra sơ đồ Wokwi
```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc-ir-obstacle-barrier/diagram.json", "utf8"))'
wokwi-cli lint pocs/poc-ir-obstacle-barrier
```

### 3.2 Biên dịch Firmware
```bash
pio run -d pocs/poc-ir-obstacle-barrier -e esp32dev
test -f pocs/poc-ir-obstacle-barrier/.pio/build/esp32dev/firmware.bin && echo "BIN OK"
```

### 3.3 Mô phỏng trên Wokwi Simulator
- Trên sơ đồ mô phỏng, nhấn phím `o` (hoặc click vào nút **Simulate Obstacle**) để tạo xung Active LOW giả lập vật cản xuất hiện.
- Quan sát đèn LED sáng, còi bíp và Serial in `[EVENT] >>> OBSTACLE DETECTED!`.

### 3.4 Nạp lên Board thật
```bash
pio run -d pocs/poc-ir-obstacle-barrier -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```

---

## 4. Wokwi Simulation — Giới Hạn Cần Biết & Cơ Chế Giả Lập

### 4.1 Tại sao Wokwi không có linh kiện cảm biến hồng ngoại tránh vật cản?
- Trong thư viện chuẩn của Wokwi (**Wokwi Parts Catalog**), hiện tại **chưa có component mô phỏng module IR Obstacle LM393** (như module TCRT5000 hay FC-51).
- Nguyên nhân kỹ thuật: Cảm biến vật cản hồng ngoại hoạt động dựa trên hiện tượng phản xạ quang học 3D trong không gian thực (LED phát chùm tia $940\text{nm}$ và Photodiode thu tia phản xạ từ bề mặt vật thể). Môi trường mô phỏng 2D trong trình duyệt không thể tính toán được độ phản xạ bề mặt quang học này.

### 4.2 Tại sao việc dùng Pushbutton lại tương đương 100% về mặt điện học?
- **Trên Module Cảm Biến Thật:**
  - Chân `OUT` chỉ là ngõ ra số **1-bit**.
  - Khi **KHÔNG CÓ** vật cản: Chân `OUT` ở mức **$3.3\text{V}$ (HIGH)**.
  - Khi **CÓ** vật cản: IC so sánh LM393 trên module kéo tụt chân `OUT` xuống **$0\text{V}$ (GND / LOW)** $\rightarrow$ Gọi là cơ chế **Active LOW**.
- **Trên Sơ đồ Mô phỏng Wokwi:**
  - Ta sử dụng một nút bấm (`wokwi-pushbutton`, gán phím tắt `o`) nối giữa `GPIO 27` và `GND`.
  - Trong firmware, chân GPIO 27 được cấu hình `INPUT_PULLUP`:
    - Khi **chưa bấm**: Điện trở kéo nội bộ giữ chân ở mức **$3.3\text{V}$ (HIGH)**.
    - Khi **bấm nút** (nhấn phím `o`): Nút đóng mạch kéo chân tụt xuống **$0\text{V}$ (LOW)**.
- **Kết luận:**
  - Về góc nhìn của vi điều khiển ESP32 và Firmware: **Tín hiệu điện áp, cạnh sườn ngắt kích hoạt (`FALLING` edge) và luồng xử lý là hoàn toàn giống nhau $100\%$**.
  - Khi chuyển từ Wokwi sang bo mạch thật: **Không cần sửa một dòng code nào**, chỉ việc cắm dây `OUT` của module cảm biến IR thật vào `GPIO 27`.

