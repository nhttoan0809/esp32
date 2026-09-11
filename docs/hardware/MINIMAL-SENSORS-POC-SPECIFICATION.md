# Đặc Tả Kỹ Thuật & Hướng Dẫn Thiết Kế 4 Minimal POCs Cảm Biến (ESP32 DevKit V1 30-Pin)

Tài liệu này tổng hợp toàn bộ kết quả nghiên cứu chuyên sâu (Deep Research), thông số tính toán phần cứng, thuật toán xử lý tín hiệu và sơ đồ thiết kế cho 4 Proof of Concepts (POC) mức nhỏ nhất (Minimal Viable POCs) tương ứng với 4 module cảm biến có sẵn trong bộ kit thí nghiệm:
1. **Module Quang trở LDR với LM393 (Ánh sáng)** $\rightarrow$ [`pocs/poc-ldr-smart-light`](file:///Users/toannguyen/Documents/esp32-learning/pocs/poc-ldr-smart-light)
2. **Module Cảm biến Nhiệt độ & Độ ẩm DHT11** $\rightarrow$ [`pocs/poc-dht11-climate-monitor`](file:///Users/toannguyen/Documents/esp32-learning/pocs/poc-dht11-climate-monitor)
3. **Module Tránh vật cản hồng ngoại LM393** $\rightarrow$ [`pocs/poc-ir-obstacle-barrier`](file:///Users/toannguyen/Documents/esp32-learning/pocs/poc-ir-obstacle-barrier)
4. **Module Cảm biến chuyển động hồng ngoại thụ động PIR HC-SR501** $\rightarrow$ [`pocs/poc-pir-motion-alarm`](file:///Users/toannguyen/Documents/esp32-learning/pocs/poc-pir-motion-alarm)

---

## 1. Triết Lý Thiết Kế "Minimal Viable POC"

Mỗi POC được thiết kế theo các nguyên tắc kỹ thuật cốt lõi:
- **Đơn nhiệm & Cô đọng (Single Responsibility):** Tập trung chứng minh đúng tính năng và bản chất vật lý của cảm biến đó, không trộn lẫn logic phức tạp.
- **Tuân thủ Tuyệt đối Phần cứng ESP32 DevKit V1 (30 chân):**
  - Không sử dụng chân SPI Flash nội bộ (GPIO 6 – 11).
  - Tránh các chân Strapping (GPIO 0, 2, 12, 15) cho tải ngoài.
  - Luôn sử dụng kênh **ADC1** (GPIO 34, 35, 32, 33) cho cảm biến tương tự để tương thích hoàn toàn khi bật Wi-Fi.
  - Mọi đèn LED đều có điện trở hạn dòng $220\Omega$ nối tiếp.
- **Xử lý tín hiệu công nghiệp (Signal Conditioning):**
  - Khử nhiễu chập chờn ngưỡng (Chattering / Flickering) bằng thuật toán **Schmitt-Trigger Hysteresis**.
  - Khử kích hoạt kép / rung cơ học quang bằng **Lockout Dead-Time** và **Hardware Interrupts**.
  - Không bao giờ dùng `delay()` làm nghẽn CPU; toàn bộ định thời đều dùng `millis()`.
- **Hỗ trợ Song song 100% Wokwi Simulator & Board Thật:**
  - Định dạng chuẩn PlatformIO CLI, cấu hình `diagram.json` chuẩn Wokwi parts library.

---

## 2. Bảng Phân Bổ Chân Toàn Cục (Global Pin Map)

| Chân ESP32 | Loại chân | Thuộc tính | Dự án sử dụng | Chức năng cụ thể |
|---|---|---|---|---|
| **GPIO 34** | Input-Only | ADC1_CH6 | `poc-ldr-smart-light` | Đọc tín hiệu Analog LDR (`AO`), dải áp 0–3.3V |
| **GPIO 35** | Input-Only | ADC1_CH7 | `poc-ldr-smart-light` | Đọc tín hiệu Digital LDR (`DO`) qua ngưỡng LM393 |
| **GPIO 18** | Output | Hỗ trợ PWM LEDC | `poc-ldr-smart-light` | Xung PWM điều khiển độ sáng đèn LED phòng |
| **GPIO 19** | I/O | Digital I/O | `poc-dht11-climate-monitor` | Giao tiếp 1-Wire Single Bus đọc DHT11 |
| **GPIO 23** | Output | Digital Output | `poc-dht11-climate-monitor` | Đèn LED Xanh báo mức Lý tưởng (Comfort) |
| **GPIO 25** | Output | Digital Output | `poc-dht11-climate-monitor` | Đèn LED Vàng báo Cảnh báo nồm ẩm (Mold Alert) |
| **GPIO 26** | Output | Digital Output | `poc-dht11-climate-monitor` | Đèn LED Đỏ báo Cảnh báo quá nhiệt (Heat Alert) |
| **GPIO 27** | Input | Ext Interrupt | `poc-ir-obstacle-barrier` | Đọc tín hiệu Active LOW từ cảm biến vật cản IR |
| **GPIO 13** | Output | Digital Output | `poc-ir-obstacle-barrier` | Đèn LED Xanh chỉ thị vật cản tiệm cận |
| **GPIO 14** | Output | Digital Output | `poc-ir-obstacle-barrier` | Active Buzzer phát tiếng bíp xác nhận |
| **GPIO 33** | Input | ADC1_CH5 / IO | `poc-pir-motion-alarm` | Đọc tín hiệu 3.3V TTL từ PIR HC-SR501 |
| **GPIO 4** | Input | Internal Pullup | `poc-pir-motion-alarm` | Nút bấm chuyển chế độ (Normal / Armed) |
| **GPIO 21** | Output | Digital Output | `poc-pir-motion-alarm` | Đèn LED Vàng chiếu sáng tự động (Auto-Light) |
| **GPIO 22** | Output | Digital Output | `poc-pir-motion-alarm` | Active Buzzer báo động an ninh |

---

## 3. Chi Tiết Kỹ Thuật Từng POC

### 3.1 POC 1: `poc-ldr-smart-light` (Quang trở LDR với LM393)

#### Sơ đồ Mạch & Tính toán Dòng:
- Module LDR gồm quang trở CdS và trở phân áp $10\text{k}\Omega$.
- Khi cường độ sáng môi trường giảm (trời tối) $\rightarrow$ Điện trở quang CdS tăng $\rightarrow$ Điện áp trên chân `AO` tăng tiến dần về $3.3\text{V}$ ($ADC \rightarrow 4095$).
- Chân `AO` nối vào **GPIO 34** (ADC1).
- Chân `DO` nối vào **GPIO 35**.
- Đèn LED phòng nối từ **GPIO 18** qua trở $220\Omega$ về GND:
  $$I_{LED} = \frac{3.3\text{V} - 2.0\text{V}}{220\Omega} \approx 5.91\text{ mA}$$

#### Thuật toán Hysteresis & Auto-Dimming:
- **Schmitt-Trigger Hysteresis:**
  - Nếu trạng thái đèn đang **TẮT**: Chỉ bật đèn khi trời tối rõ rệt ($ADC \ge 2800$).
  - Nếu trạng thái đèn đang **BẬT**: Chỉ tắt đèn khi trời sáng rõ rệt ($ADC \le 2200$).
  - Vùng trễ $\Delta = 600$ đơn vị ADC ($0.48\text{V}$) loại bỏ hoàn toàn hiện tượng chớp tắt khi mây bay qua hoặc chạng vạng.
- **Ambient Auto-Dimming:**
  - Tần số PWM: $5\text{ kHz}$, độ phân giải 12-bit ($0 - 4095$).
  - Công thức tính Duty: $\text{Duty} = \text{constrain}\left(\frac{ADC - 1500}{4095 - 1500} \times 4095, 0, 4095\right)$.

---

### 3.2 POC 2: `poc-dht11-climate-monitor` (Nhiệt độ & Độ ẩm DHT11)

#### Giao thức & Định thời:
- Giao thức 1 dây (Single-Bus): ESP32 kéo LOW ít nhất $18\text{ms}$ để gửi tín hiệu Start, sau đó chuyển sang Input đón nhận 40-bit dữ liệu từ DHT11 (16-bit độ ẩm + 16-bit nhiệt độ + 8-bit checksum).
- Chu kỳ đo: $2000\text{ms}$ (tuân thủ giới hạn phần cứng $1\text{ Hz}$ của DHT11).

#### Tiêu chí Phân loại Khí hậu & Nồm Ẩm:
1. **Comfort Mode (Lý tưởng):** $20^\circ\text{C} \le T \le 29^\circ\text{C}$ và $40\% \le H \le 70\%$ $\rightarrow$ Bật LED Xanh (GPIO 23).
2. **Mold/Humid Alert (Cảnh báo nồm ẩm):** $H \ge 75\%$ $\rightarrow$ Nhấp nháy LED Vàng (GPIO 25).
3. **Severe Heat/Cold Alert (Khắc nghiệt):** $T \ge 35^\circ\text{C}$ hoặc $T \le 16^\circ\text{C}$ $\rightarrow$ Bật LED Đỏ (GPIO 26) + Bíp cảnh báo ngắn $50\text{ms}$.

---

### 3.3 POC 3: `poc-ir-obstacle-barrier` (Cảm biến Tránh vật cản hồng ngoại LM393)

#### Xử lý Tín hiệu & Ngắt:
- Cảm biến phát chùm tia hồng ngoại không nhìn thấy ($940\text{nm}$). Khi có vật cản phản xạ tia $\rightarrow$ Photodiode dẫn $\rightarrow$ LM393 xuất mức `LOW` trên chân `OUT`.
- Tín hiệu `OUT` được bắt bằng ngắt ngoài:
  ```cpp
  attachInterrupt(digitalPinToInterrupt(PIN_IR_IN), isrObstacleDetected, FALLING);
  ```
- **Khóa thời gian chết (Lockout Dead-Time):** Để tránh đếm lặp do mép vật thể rung hoặc di chuyển chậm, sau mỗi lần bắt ngắt, hệ thống khóa ngắt trong $1000\text{ms}$.
- **Phản hồi tức thì:** Bật LED Xanh (GPIO 13) và kích hoạt Active Buzzer (GPIO 14) kêu $40\text{ms}$ báo nhận diện, đồng thời tăng bộ đếm sản phẩm `count++`.

---

### 3.4 POC 4: `poc-pir-motion-alarm` (Cảm biến Chuyển động PIR HC-SR501)

#### Cấp nguồn & Logic Thời gian:
- **Nguồn cấp:** Cấp **5V (VIN)** từ chân VIN của ESP32 để IC BISS0001 và chip ổn áp 7133 trên module PIR hoạt động chính xác. Chân `OUT` xuất mức logic $3.3\text{V}$ hoàn toàn an toàn với GPIO ESP32.
- **Thời gian làm nóng (Warm-up Period):** Trong $30\text{s}$ đầu tiên sau khi khởi động, cảm biến tự ổn định nhiệt độ; firmware duy trì cờ `isWarmingUp = true` và bỏ qua các trigger ảo.
- **Chế độ hoạt động (FSM - 2 Modes):**
  - **Mode 1: Energy-Saving Auto-Light (Mặc định):**
    - Có chuyển động $\rightarrow$ Đèn LED Vàng (GPIO 21) BẬT.
    - Duy trì sáng liên tục khi người còn di chuyển.
    - Khi hết chuyển động $\rightarrow$ Giữ sáng thêm $8\text{s}$ (Hold Time), sau đó tự TẮT.
  - **Mode 2: Armed Security Alarm (Nhấn nút GPIO 4 để kích hoạt):**
    - Có chuyển động $\rightarrow$ Đèn chớp liên tục + Còi Buzzer (GPIO 22) phát chuỗi âm cảnh báo $100\text{ms}$ ON / $100\text{ms}$ OFF.
