# Danh mục & Thông số Linh kiện Phần cứng Mở rộng (Extended Components Reference)

Tài liệu này tổng hợp toàn bộ thông số kỹ thuật, sơ đồ phân bổ chân, cảnh báo an toàn điện áp và hướng dẫn ghép nối với vi điều khiển **ESP32 DevKit V1 (30 chân)** cho 19 thiết bị, module cảm biến, động cơ và phụ kiện mở rộng phục vụ học tập và phát triển dự án IoT.

---

## 1. Màn hình & Hiển thị (Displays)

### 1.1 Màn hình LCD 1602 (16x2 Character LCD Display & Ba lô I2C PCF8574)
- **Độ phân giải & Khả năng hiển thị:** 16 ký tự × 2 dòng (tổng cộng 32 ô ký tự font 5×8 dot ma trận).
- **IC điều khiển gốc:** Hitachi HD44780 (hoặc tương đương KS0066U).
- **Điện áp hoạt động:** 5V DC (chân VDD cấp nguồn logic, chân A/K cấp đèn nền Backlight).
- **Giao tiếp:**
  - *Chế độ song song gốc (16 chân):* Cần 4 chân dữ liệu (D4–D7) + 2 chân điều khiển (RS, E).
  - *Chế độ I2C (Hàn module chuyển đổi PCF8574T/AT phía sau bo):* Thu gọn chỉ còn 2 dây tín hiệu I2C. Địa chỉ bus I2C mặc định thường là `0x27` (chip PCF8574T) hoặc `0x3F` (chip PCF8574AT).
- **Chân nối ESP32 (Qua module I2C ba lô):**
  - `GND` → GND chung
  - `VCC` → 5V (Chân VIN của ESP32 nếu cắm USB)
  - `SDA` → GPIO 21
  - `SCL` → GPIO 22
- **Thư viện khuyên dùng:** `LiquidCrystal_I2C` (by Frank de Brabander / marcoschwartz).
- ⚠️ **Lưu ý tương thích 3.3V:** Hầu hết module I2C PCF8574 có sẵn điện trở pull-up 4.7kΩ nối lên đường 5V (VCC). Dù ESP32 có khả năng chịu đựng nhất định, khuyến nghị nếu dùng lâu dài nên cấp nguồn 5V cho màn hình nhưng ngắt/gỡ bỏ trở pull-up 5V trên ba lô I2C và sử dụng trở kéo lên 3.3V, hoặc dùng IC chuyển mức logic (Logic Level Shifter).

---

### 1.2 Màn hình LED Ma Trận 8x8 (MAX7219 Dot Matrix)
- **Cấu tạo & Độ phân giải:** 64 điểm LED đơn xếp thành lưới 8 hàng × 8 cột.
- **IC điều khiển:** MAX7219 (Driver quét LED nối tiếp đa kênh, tích hợp thanh ghi dịch BCD code-B, mạch giải mã và RAM lưu trạng thái 8×8).
- **Điện áp hoạt động:** 5V DC.
- **Dòng tiêu thụ:** Lên tới 300mA – 1A khi bật sáng toàn bộ 64 điểm LED ở độ sáng cực đại.
- **Giao tiếp:** Nối tiếp chuẩn SPI 3 dây (DIN, CS/LOAD, CLK). Hỗ trợ ghép nối tiếp chuỗi (Daisy-chain) nhiều module 8x8 liên tiếp nhau.
- **Chân nối ESP32:**
  - `VCC` → 5V (VIN của ESP32 hoặc nguồn ngoài 5V riêng nếu ghép chuỗi)
  - `GND` → GND chung
  - `DIN` → GPIO 23 (VSPI MOSI)
  - `CS` → GPIO 5 (VSPI SS/CS)
  - `CLK` → GPIO 18 (VSPI SCK)
- **Thư viện khuyên dùng:** `LedControl` hoặc bộ thư viện đồ họa chuyên dụng `MD_MAX72xx` + `MD_Parola`.

---

### 1.3 Màn hình LED 7 Đoạn 4 Chữ Số (TM1637 Display)
- **Kích thước & Quy cách:** 4 chữ số 7 thanh kèm dấu 2 chấm đồng hồ (:), kích thước hiển thị 0.36 inch hoặc 0.56 inch.
- **IC điều khiển:** Titan Micro TM1637 (tích hợp mạch quét hiển thị LED và đọc bàn phím).
- **Điện áp hoạt động:** 3.3V – 5V DC (hoạt động trực tiếp hoàn hảo ở 3.3V của ESP32).
- **Dòng tiêu thụ:** ~30mA – 80mA (tùy thuộc mức chỉnh độ sáng qua phần mềm từ 0 đến 7).
- **Giao thức:** Giao thức truyền thông 2 dây độc quyền (tương tự I2C nhưng không dùng địa chỉ thiết bị; dữ liệu truyền đi kèm bit ACK phản hồi).
- **Chân nối ESP32:**
  - `VCC` → 3V3 (hoặc 5V)
  - `GND` → GND
  - `DIO` → GPIO 17 (hoặc bất kỳ GPIO ngõ ra nào)
  - `CLK` → GPIO 16 (hoặc bất kỳ GPIO ngõ ra nào)
- **Thư viện khuyên dùng:** `TM1637Display` (by Avishay Orpaz).

---

## 2. Cảm biến (Sensors)

### 2.1 Cảm biến Siêu âm Đo khoảng cách HC-SR04
- **Nguyên lý hoạt động:** Phát chùm 8 xung siêu âm tần số 40 kHz từ đầu phát (Transmitter - T), nhận sóng dội lại tại đầu thu (Receiver - R). Khoảng cách tính bằng: $d = \frac{t \times 0.0343}{2}$ (cm).
- **Dải đo:** 2 cm – 400 cm (độ phân giải 0.3 cm, góc quét hiệu dụng < 15°).
- **Điện áp hoạt động:** 5V DC (phiên bản HC-SR04 truyền thống bắt buộc dùng 5V; phiên bản HC-SR04P có thể chạy được 3.3V).
- **Dòng hoạt động:** ~15mA.
- **Sơ đồ chân nối ESP32:**
  - `VCC` → VIN (5V)
  - `GND` → GND chung
  - `Trig` → GPIO 5 (ESP32 xuất xung 3.3V kích Trig được bình thường)
  - `Echo` → **BẮT BUỘC QUA CẦU PHÂN ÁP** → GPIO 18 (Input)
- ⚠️ **CẢNH BÁO AN TOÀN QUAN TRỌNG (Echo Pin 5V):**
  - Chân `Echo` của HC-SR04 xuất xung phản hồi mức điện áp **5V**.
  - Các chân GPIO của ESP32 **không có khả năng chịu áp 5V (Not 5V Tolerant)**. Cắm trực tiếp chân Echo 5V vào GPIO sẽ gây nguy cơ cháy cổng ngõ vào vi điều khiển.
  - **Mạch phân áp bắt buộc:**
    $$\text{Echo (5V)} \longrightarrow [R_1 = 1\text{k}\Omega] \longrightarrow \text{GPIO 18} \longrightarrow [R_2 = 2\text{k}\Omega] \longrightarrow \text{GND}$$
    Điện áp đưa vào chân GPIO khi đó: $V_{\text{in}} = 5\text{V} \times \frac{2\text{k}\Omega}{1\text{k}\Omega + 2\text{k}\Omega} \approx 3.33\text{V}$ (hoàn toàn an toàn).

---

### 2.2 Cảm biến Âm thanh (Sound Sensor Module HW-484 / LM393)
- **Cấu tạo:** Microphone điện dung (Electret Microphone) + IC so sánh điện áp kép LM393 + biến trở vi chỉnh độ nhạy (Trimpot 10kΩ).
- **Điện áp hoạt động:** 3.3V – 5V DC (khuyên dùng 3.3V nối tiếp với ESP32).
- **Ngõ ra:**
  - `DO` (Digital Output): Mức logic `HIGH` (3.3V) khi môi trường yên tĩnh; kéo xuống `LOW` (0V) khi âm thanh vượt ngưỡng cài đặt trên biến trở.
  - `AO` (Analog Output): Điện áp biến thiên liên tục từ 0V đến VCC theo biên độ sóng âm.
- **Chân nối ESP32:**
  - `VCC` → 3V3
  - `GND` → GND
  - `DO` → GPIO 17 (Cấu hình `INPUT`)
  - `AO` → **GPIO 34 hoặc GPIO 35** (Kênh ADC1 của ESP32)
- ⚠️ **Lưu ý ADC:** Tuyệt đối không cắm chân `AO` vào các chân thuộc ADC2 (GPIO 4, 0, 2, 15, 13, 12, 14, 27, 25, 26) nếu ứng dụng có sử dụng Wi-Fi.

---

### 2.3 Module Cảm biến Dòng điện Hall ACS712
- **IC chính:** Allegro ACS712ELCTR (sử dụng hiệu ứng Hall đo từ trường sinh ra bởi dòng điện chạy qua dây dẫn đồng nội bộ cách ly điện tích cao 2.1kVRMS).
- **Các phân loại phiên bản:**
  - `ACS712-05B`: Dải đo ±5A (Độ nhạy $185\text{ mV/A}$).
  - `ACS712-20A`: Dải đo ±20A (Độ nhạy $100\text{ mV/A}$).
  - `ACS712-30A`: Dải đo ±30A (Độ nhạy $66\text{ mV/A}$).
- **Điện áp cấp IC:** 5V DC (chân VCC bắt buộc 5V để IC hoạt động đúng chuẩn).
- **Đặc tính điện áp ngõ ra (OUT):**
  - Khi dòng điện $I = 0\text{A}$: $V_{\text{OUT}} = \frac{V_{CC}}{2} \approx 2.5\text{V}$.
  - Khi có dòng điện chạy qua: Điện áp tăng lên (dòng thuận) hoặc giảm xuống (dòng nghịch) trong khoảng $0.5\text{V} – 4.5\text{V}$.
- **Sơ đồ đấu nối:**
  - *Cọc vít công suất (IP+, IP-):* Đấu nối tiếp vào đường dây pha của tải tiêu thụ.
  - *Chân tín hiệu:*
    - `VCC` → VIN (5V)
    - `GND` → GND chung
    - `OUT` → Qua cầu phân áp hạ áp → GPIO 32 / 33 (Kênh ADC1)
- ⚠️ **Cảnh báo điện áp ngõ ra vượt ngưỡng 3.3V:** Khi đo dòng điện cực đại, chân OUT có thể lên đến 4.5V. Cần mắc cầu phân áp tỉ lệ $\frac{2}{3}$ (ví dụ $R_1=1\text{k}\Omega, R_2=2\text{k}\Omega$) để đưa dải điện áp $0\text{V}–4.5\text{V}$ về khoảng an toàn $0\text{V}–3.0\text{V}$ cho ADC1 của ESP32.

---

## 3. Động cơ & Bộ điều khiển Truyền động (Motors & Actuators)

### 3.1 Động cơ Giảm tốc Vàng (TT Motor DC 3V–6V)
- **Loại động cơ:** Động cơ điện một chiều mini có chổi than (DC Brushed Motor) tích hợp hộp số giảm tốc bánh răng nhựa.
- **Tỉ số giảm tốc phổ biến:** 1:48 (Tốc độ quay trục ra ~130 RPM tại 3V, ~200 RPM tại 6V).
- **Điện áp hoạt động:** 3V – 6V DC (tối ưu nhất 4.5V – 6V).
- **Dòng tiêu thụ:**
  - Không tải: 60mA – 80mA.
  - Có tải: 200mA – 400mA.
  - Dòng kẹt cốt (Stall Current): Lên đến **800mA – 1.5A** tại 6V.
- ⚠️ **CẢNH BÁO NGUỒN CẤP ĐỘNG CƠ:**
  - Tuyệt đối **không** được cấp nguồn nuôi động cơ từ chân 3V3 hay VIN của ESP32. Dòng khởi động và xung cảm ứng ngược của động cơ sẽ gây sập nguồn, kích hoạt cơ chế bảo vệ sụt áp (Brownout Detector) làm ESP32 reset liên tục, hoặc thậm chí cháy chip ổn áp AMS1117.
  - **Bắt buộc sử dụng nguồn pin rời bên ngoài** (như 2 cell pin 18650 qua mạch giảm áp, hoặc hộp 4 pin AA) và **nối chung cực âm GND** với ESP32.

---

### 3.2 Động cơ Servo SG90 (Micro Servo 9g)
- **Góc quay:** 0° đến 180° (điều khiển vị trí góc bằng chuỗi xung PWM tần số 50Hz, chu kỳ 20ms: xung 0.5ms tương ứng 0°, 1.5ms tương ứng 90°, 2.5ms tương ứng 180°).
- **Lực kéo (Torque):** ~1.8 kg·cm (tại 4.8V).
- **Điện áp hoạt động:** 4.8V – 6.0V DC (tối ưu 5.0V DC).
- **Dòng tiêu thụ:** Chờ ~10mA; di chuyển bình thường ~100mA – 250mA; kẹt tải ~500mA – 800mA.
- **Sơ đồ dây nối:**
  - **Dây Đỏ (VCC):** Nối nguồn dương 5V ngoài.
  - **Dây Nâu / Đen (GND):** Nối mass chung (GND nguồn ngoài và GND ESP32).
  - **Dây Cam / Vàng (Signal):** Nối trực tiếp vào chân GPIO hỗ trợ PWM của ESP32 (ví dụ GPIO 18).
- **Thư viện khuyên dùng:** `ESP32Servo` (tự động điều phối kênh phần cứng LEDC của ESP32).

---

### 3.3 Động cơ Bước 28BYJ-48 & Mạch đệm ULN2003
- **Loại động cơ:** Động cơ bước đơn cực 4 pha 5 dây (Unipolar Stepper Motor), nam châm vĩnh cửu.
- **Tỉ số truyền hộp số:** 1:64.
- **Góc bước danh định:** $5.625^\circ / 64 \approx 0.08789^\circ$ (Cần 4096 bước ở chế độ kích nửa bước Half-step hoặc 2048 bước ở chế độ cả bước Full-step để quay đủ 1 vòng 360°).
- **Điện áp hoạt động:** 5V DC.
- **Dòng tiêu thụ:** ~200mA – 320mA khi kích hoạt các cuộn dây pha.
- **Mạch đệm công suất ULN2003:** Chứa mảng 7 transistor Darlington đệm dòng cực thu hở (Open-Collector) tích hợp diode triệt tiêu xung điện áp ngược.
- **Chân nối ESP32 (vào mạch ULN2003):**
  - `IN1` → GPIO 19
  - `IN2` → GPIO 18
  - `IN3` → GPIO 5
  - `IN4` → GPIO 17
  - `5V–12V` trên bo ULN2003 → Nối nguồn ngoài 5V DC.
  - `GND` trên bo ULN2003 → Nối GND nguồn ngoài và nối chung GND ESP32.
- **Thư viện khuyên dùng:** `Stepper` hoặc `AccelStepper`.

---

### 3.4 Mạch Điều Khiển Động Cơ Cầu H L298N (ZX-040)
- **Chip điều khiển:** STMicroelectronics L298N Dual Full-Bridge Driver.
- **Công suất tải:** Điều khiển độc lập 2 động cơ DC (đảo chiều + điều tốc băm xung PWM) hoặc 1 động cơ bước lưỡng cực (Bipolar Stepper).
- **Điện áp động lực (+12V):** 5V – 35V DC (Dòng định mức 2A mỗi kênh, dòng đỉnh 3A).
- **Điện áp điều khiển logic (+5V):** 5V DC.
- **Quy tắc Jumper 5V-EN:**
  - *Khi nguồn động cơ từ 7V – 12V:* Cắm jumper 5V-EN, bo mạch dùng IC 78M05 nội bộ để tự cấp nguồn logic 5V; chân `+5V` lúc này có thể dùng làm ngõ ra cấp nguồn cho bo ESP32 (vào chân VIN).
  - *Khi nguồn động cơ > 12V:* Rút jumper 5V-EN ra để tránh làm cháy IC 78M05, và cấp một nguồn 5V riêng biệt từ ngoài vào cọc `+5V`.
- **Chân điều khiển tín hiệu:**
  - `ENA` / `ENB`: Băm xung PWM điều tốc (nhận mức logic 3.3V từ ESP32).
  - `IN1`, `IN2`: Logic điều khiển chiều Motor A.
  - `IN3`, `IN4`: Logic điều khiển chiều Motor B.
- ⚠️ **Lưu ý mức logic:** Ngõ vào IN1–IN4 và ENA/ENB nhận mức logic tối thiểu $2.3\text{V}$ là mức HIGH, do đó tín hiệu 3.3V của ESP32 kích trực tiếp ổn định mà không cần nâng áp.

---

### 3.5 Module Relay 1 Kênh 5V (SRD-05VDC-SL-C)
- **Điện áp cuộn hút:** 5V DC (cấp từ VIN khi cắm USB hoặc nguồn 5V ngoài).
- **Dòng tiêu thụ cuộn hút:** ~70mA khi đóng rơ-le.
- **Tiếp điểm cơ khí:** Tiếp điểm SPDT Form C (chịu tải tối đa 10A 250V AC / 10A 30V DC).
- **Cấu hình chân:**
  - `VCC` → 5V (VIN)
  - `GND` → GND chung
  - `IN` → GPIO (kích Active LOW hoặc Active HIGH tùy thiết kế mạch)
  - `COM`, `NO`, `NC` → Mắc nối tiếp với tải điện 1 chiều (khuyến cáo < 24V DC trong thực hành học tập).

---

### 3.6 Loa Mini / Còi Bíp Gốm Áp Điện (Piezo Buzzer / Speaker)
- **Phân loại phần cứng:**
  - *Active Buzzer (Còi chủ động):* Có tích hợp sẵn mạch dao động âm tần bên trong. Chỉ cần kích mức logic `HIGH` (3.3V) là phát tiếng bíp đơn sắc tần số ~2.4 kHz.
  - *Passive Buzzer (Còi thụ động / Loa gốm):* Không có mạch tạo dao động bên trong. Cần đưa vào tín hiệu xung vuông PWM tần số biến thiên (từ 100Hz đến 5000Hz) để tạo các nốt nhạc.
- **Chân nối ESP32:**
  - Cực dương (+) → Chân GPIO PWM (ví dụ GPIO 25 hoặc GPIO 26 qua điện trở $100\Omega$).
  - Cực âm (-) → GND chung.
- **Hàm điều khiển trong ESP32 Core:** Dùng ngoại vi phần cứng LEDC (`ledcAttachChannel`, `ledcWriteTone` trên ESP-IDF / Arduino-ESP32 Core v3.x).

---

## 4. Giao diện Nhập liệu & Nhận dạng (Input & Identification)

### 4.1 Bàn Phím Ma Trận Nút Nhấn 4x4 (4x4 Matrix Keypad)
- **Kết cấu:** 16 phím bấm cơ học (hoặc màng dẻo) bố trí thành lưới 4 hàng (Row 1–4) × 4 cột (Column 1–4).
- **Nguyên lý quét ma trận:**
  - Vi điều khiển cấu hình 4 chân Hàng làm ngõ ra (Output), 4 chân Cột làm ngõ vào kéo lên (Input Pull-up).
  - Quét lần lượt từng hàng bằng cách kéo mức LOW, đọc giá trị trên 4 chân cột để phát hiện vị trí phím giao điểm đang được bấm.
- **Số chân kết nối:** Yêu cầu 8 chân GPIO trống của ESP32.
- **Gợi ý phân bổ chân trên ESP32 DevKit V1:**
  - Hàng: `R1` → GPIO 13, `R2` → GPIO 12, `R3` → GPIO 14, `R4` → GPIO 27.
  - Cột: `C1` → GPIO 26, `C2` → GPIO 25, `C3` → GPIO 33, `C4` → GPIO 32.
- **Thư viện khuyên dùng:** `Keypad` (by Mark Stanley, Alexander Brevig).

---

### 4.2 Module Đọc/Ghi Thẻ Từ RFID RC522 13.56MHz
- **IC truyền thông chính:** NXP MFRC522.
- **Tần số sóng mang:** 13.56 MHz (chuẩn giao tiếp không tiếp xúc ISO/IEC 14443A, hỗ trợ đọc/ghi thẻ Mifare Classic 1K, Mifare 4K, Mifare Ultralight).
- **Khoảng cách phát hiện:** < 50mm.
- **Giao diện chuẩn:** Bus SPI (Serial Peripheral Interface).
- ⚠️ **CẢNH BÁO NGUỒN CẤP BẮT BUỘC 3.3V:**
  - Chân `3.3V (VCC)` của module RC522 **bắt buộc phải cấp nguồn 3.3V DC** (lấy từ chân 3V3 của ESP32).
  - **Tuyệt đối không cấp nguồn 5V vào chân 3.3V của module RC522**, IC MFRC522 không hỗ trợ điện áp 5V và sẽ bị cháy ngay lập tức!
- **Sơ đồ kết nối với bus VSPI mặc định của ESP32:**
  - `3.3V` → 3V3
  - `RST` → GPIO 22 (hoặc GPIO tự do)
  - `GND` → GND
  - `MISO` → GPIO 19
  - `MOSI` → GPIO 23
  - `SCK` → GPIO 18
  - `SDA (SS)` → GPIO 21 (hoặc GPIO 5)
- **Thư viện khuyên dùng:** `MFRC522` (by GithubCommunity / Miguel Balboa).

---

### 4.3 Module Đồng Hồ Thời Gian Thực RTC (HW-111 / DS1307 / DS3231)
- **IC chính:**
  - *DS1307:* Dùng thạch anh dao động ngoài 32.768kHz (độ chính xác trung bình).
  - *DS3231:* Tích hợp thạch anh bù nhiệt độ TCXO (độ chính xác cực cao sai số < 1 phút/năm).
  - *EEPROM AT24C32:* Tích hợp sẵn 32 Kbit bộ nhớ không biến động lưu trữ dữ liệu người dùng.
- **Nguồn nuôi pin dự phòng:** Khe cắm pin cúc áo 3V (CR2032) giúp duy trì đếm giờ khi toàn bộ hệ thống bị cắt điện nguồn.
- **Điện áp hoạt động:** 3.3V – 5.5V DC (Khuyến khích nối vào chân 3V3 khi giao tiếp với ESP32).
- **Giao tiếp:** Chuẩn I2C (Địa chỉ mặc định của DS1307/DS3231 là `0x68`, của IC nhớ EEPROM là `0x57`).
- **Sơ đồ chân nối ESP32:**
  - `VCC` → 3V3
  - `GND` → GND
  - `SDA` → GPIO 21
  - `SCL` → GPIO 22
- **Thư viện khuyên dùng:** `RTClib` (by Adafruit).

---

## 5. Bo Mạch Vi Điều Khiển & Phụ Kiện Giao Tiếp (Microcontrollers & Cables)

### 5.1 Bo Mạch Vi Điều Khiển Arduino Nano (ATmega328P)
- **Kiến trúc:** Vi điều khiển 8-bit AVR RISC (Microchip ATmega328P), xung nhịp thạch anh 16 MHz.
- **Bộ nhớ:** 32 KB Flash (2 KB dùng cho bootloader), 2 KB SRAM, 1 KB EEPROM.
- **Điện áp hoạt động:** 5V DC (nguồn ngoài qua chân VIN từ 7V – 12V qua IC ổn áp AMS1117-5.0).
- **Ngõ vào/ra:** 14 chân Digital I/O (trong đó có 6 kênh PWM 8-bit), 8 kênh Analog Input 10-bit (A0–A7).
- ⚠️ **Lưu ý khi ghép nối với ESP32 (UART / SPI / I2C):**
  - Mức logic logic của Arduino Nano là **5V**. Mức logic của ESP32 là **3.3V**.
  - Khi truyền dữ liệu UART từ Arduino Nano sang ESP32: Chân TX của Arduino Nano xuất mức 5V; **bắt buộc phải qua cầu phân áp** ($1\text{k}\Omega / 2\text{k}\Omega$) để hạ về 3.3V trước khi cắm vào chân RX của ESP32.
  - Chiều ngược lại (TX của ESP32 3.3V nối sang RX của Arduino Nano) có thể nhận trực tiếp an toàn.

---

### 5.2 Bo Mạch Vi Điều Khiển STM32F4 "Black Pill" (STM32F401 / STM32F411)
- **Kiến trúc:** ARM 32-bit Cortex-M4 RISC tích hợp bộ tính toán số thực FPU phần cứng và hướng dẫn DSP.
- **Xung nhịp tối đa:** 84 MHz (bản STM32F401CCU6) hoặc 100 MHz (bản STM32F411CEU6).
- **Bộ nhớ:** 256 KB – 512 KB Flash, 64 KB – 128 KB SRAM.
- **Điện áp hoạt động:** 3.3V DC (Nguồn cấp qua cổng USB Type-C hoặc chân 5V qua IC ổn áp).
- **Đặc tính chân I/O:** Phần lớn các chân GPIO của STM32F4 được thiết kế chuẩn **5V-Tolerant (FT)** (chịu được mức tín hiệu 5V mà không bị hỏng, rất thuận tiện khi nối với ngoại vi 5V).
- **Giao diện lập trình & nạp code:**
  - Cổng USB Type-C hỗ trợ nạp trực tiếp qua chế độ DFU Bootloader (nhấn giữ nút BOOT0 và bấm nút NRST).
  - Cổng 4 chân SWD (3V3, SWCLK, SWDIO, GND) giao tiếp với mạch nạp ST-Link V2 để nạp và gỡ lỗi (debug) từng dòng lệnh.
- **Môi trường lập trình:** PlatformIO (khung `ststm32` với Arduino Framework hoặc STM32Cube).

---

### 5.3 Mạch Chuyển Đổi USB sang UART / TTL (HW-896 V1.2)
- **IC chuyển đổi:** Thường tích hợp chip CH340G, CP2102 hoặc FT232RL.
- **Chức năng:** Cầu nối trung gian chuyển đổi dữ liệu giao thức USB từ máy tính thành tín hiệu nối tiếp UART chuẩn TTL.
- **Cấu hình điện áp:** Có jumper ghim chọn mức logic ngõ ra giữa **3.3V** và **5V**.
- **Chân kết nối:**
  - `TXD`: Chân truyền dữ liệu (nối vào chân RX của vi điều khiển).
  - `RXD`: Chân nhận dữ liệu (nối vào chân TX của vi điều khiển).
  - `GND`: Chân mass chung (bắt buộc nối chung đất với bo mạch mục tiêu).
  - `5V / 3V3`: Cấp nguồn cho vi điều khiển nếu cần.

---

### 5.4 Cáp USB Type-A sang Type-B
- **Chuẩn kỹ thuật:** USB 2.0 (tốc độ truyền dữ liệu tối đa 480 Mbps).
- **Kiểu đầu cắm:**
  - *Type-A:* Đầu cắm dẹt truyền thống cắm vào cổng máy tính hoặc củ sạc.
  - *Type-B:* Đầu cắm hình vuông vát 2 góc chuyên dụng cho thiết bị công nghiệp, máy in và bo mạch Arduino Uno R3 / Mega 2560.
- **Sơ đồ 4 chân dây dẫn:**
  1. `VBUS` (Dây màu đỏ): Cấp nguồn dương +5V DC (chịu dòng tối đa 500mA theo chuẩn USB 2.0).
  2. `D-` (Dây màu trắng): Tín hiệu truyền dữ liệu vi sai âm.
  3. `D+` (Dây màu xanh lá): Tín hiệu truyền dữ liệu vi sai dương.
  4. `GND` (Dây màu đen): Chân nối đất / tiếp địa cực âm.

---

## 6. Bảng Tổng Hợp Tương Thích & Mức Điện Áp Với ESP32 DevKit V1

| Linh kiện / Thiết bị | Điện áp cấp nguồn ($V_{CC}$) | Mức logic tín hiệu | Tương thích trực tiếp ESP32 (3.3V)? | Biện pháp bảo vệ bắt buộc khi ghép nối |
|:---|:---:|:---:|:---:|:---|
| **LCD 1602 (kèm ba lô I2C)** | 5V DC | 5V $I^2C$ | ⚠️ Cảnh báo | Nên ngắt trở pull-up 5V trên ba lô I2C hoặc dùng mạch chuyển mức logic. |
| **LED Ma trận 8x8 (MAX7219)** | 5V DC | 3.3V / 5V SPI | ✅ Tương thích | Kích DIN, CS, CLK bằng 3.3V từ ESP32 bình thường. Nguồn cấp 5V lấy từ VIN. |
| **LED 7 Đoạn 4 Số (TM1637)** | 3.3V – 5V DC | 3.3V | ✅ Hoàn toàn tương thích | Cấp 3.3V trực tiếp từ ESP32, không cần trở đệm. |
| **Cảm biến Siêu âm HC-SR04** | 5V DC | 5V | ❌ Không tương thích chân Echo | **Bắt buộc phân áp** chân Echo từ 5V xuống ~3.3V trước khi vào GPIO ESP32. |
| **Cảm biến Âm thanh HW-484** | 3.3V – 5V DC | 3.3V | ✅ Hoàn toàn tương thích | Cấp nguồn 3.3V. Chân AO phải nối vào ADC1 (GPIO 32–39). |
| **Cảm biến Dòng ACS712** | 5V DC | 0.5V – 4.5V Analog | ⚠️ Nguy cơ quá áp | Cần cầu phân áp $\frac{2}{3}$ ngõ ra OUT trước khi cắm vào ADC1 ESP32. |
| **Động cơ Giảm tốc Vàng TT** | 3V – 6V DC | Động lực | ❌ Cấm nối trực tiếp | Dùng nguồn pin ngoài riêng biệt, qua mạch cầu H L298N/L9110S, nối chung GND. |
| **Động cơ Servo SG90** | 4.8V – 6V DC | 3.3V PWM | ⚠️ Cần nguồn ngoài | Dây tín hiệu PWM nối trực tiếp ESP32; nguồn VCC phải dùng nguồn 5V ngoài. |
| **Động cơ Bước 28BYJ-48** | 5V DC | 3.3V Logic | ⚠️ Cần nguồn ngoài | 4 chân IN1–IN4 nối trực tiếp GPIO qua mạch ULN2003; nguồn 5V cấp ngoài. |
| **Mạch Cầu H L298N** | 5V – 35V DC | 3.3V / 5V | ✅ Tương thích tín hiệu | IN1–IN4, ENA, ENB nhận xung 3.3V tốt. Nối chung GND giữa L298N và ESP32. |
| **Module Relay 1 Kênh 5V** | 5V DC | 3.3V / 5V | ✅ Tương thích tín hiệu | Cấp VCC cuộn hút 5V từ VIN. Chân kích IN nối trực tiếp GPIO. |
| **Còi Bíp / Loa Mini** | 3.3V – 5V DC | 3.3V PWM | ✅ Tương thích | Mắc thêm điện trở $100\Omega$ hạn dòng bảo vệ GPIO. |
| **Bàn phím Ma trận 4x4** | Thụ động | 3.3V | ✅ Hoàn toàn tương thích | Bật `INPUT_PULLUP` nội bộ trên các chân GPIO ngõ vào của ESP32. |
| **Module RFID RC522** | **3.3V DC** | 3.3V SPI | ⚠️ Cảnh báo nguồn | **Bắt buộc chỉ cấp 3.3V**, cấm cấp 5V gây cháy chip MFRC522. |
| **Module RTC (DS1307/3231)** | 3.3V – 5V DC | 3.3V $I^2C$ | ✅ Hoàn toàn tương thích | Cấp nguồn 3.3V từ chân 3V3 của ESP32. |
| **Arduino Nano (ATmega328P)** | 5V DC | 5V TTL | ⚠️ Cảnh báo chân TX | Phân áp chân TX Nano (5V) xuống 3.3V trước khi cắm vào chân RX ESP32. |
| **STM32F4 Black Pill** | 3.3V DC | 3.3V / 5V-FT | ✅ Hoàn toàn tương thích | Đều hoạt động ở mức logic 3.3V chuẩn. |

---

## 7. Nguyên Tắc An Toàn & Lưu Ý Kỹ Thuật Bắt Buộc

1. **Nguyên tắc "3.3V Logic Level":**
   - Bộ vi điều khiển ESP32 sử dụng mức điện áp bán dẫn $3.3\text{V}$. Không một chân GPIO nào của ESP32 có khả năng chịu quá áp $5\text{V}$.
   - Khi giao tiếp với bất kỳ cảm biến hay bo mạch hoạt động ở $5\text{V}$ (HC-SR04, Arduino Nano, IC đo dòng), luôn phải dùng **cầu phân áp điện trở** hoặc **IC Logic Level Converter (như TXS0108E / BSS138)** trên đường truyền tín hiệu từ thiết bị 5V vào ESP32.
2. **Nguyên tắc cách ly nguồn động lực cho tải cảm kháng (Inductive Loads):**
   - Động cơ DC, động cơ bước, động cơ servo và cuộn hút rơ-le tiêu thụ dòng khởi động và dòng quá tải rất lớn, đồng thời sinh ra xung điện áp cảm ứng âm nhọn (Back-EMF).
   - Tuyệt đối không cấp nguồn cho các thiết bị này từ chân 3V3 của ESP32. Luôn cấp bằng nguồn pin ngoài hoặc adapter độc lập và bắt buộc nối chung dây mass (GND) để thống nhất điện thế tham chiếu.
3. **Tuân thủ phân bổ chân GPIO trên ESP32 DevKit V1 (30 chân):**
   - **Chân Input-Only (GPIO 34, 35, 36/VP, 39/VN):** Không có điện trở kéo nội bộ (`INPUT_PULLUP` không hoạt động); tuyệt đối không cấu hình làm ngõ ra `OUTPUT`.
   - **Kênh ADC khi bật Wi-Fi:** Chỉ được sử dụng các kênh **ADC1 (GPIO 32, 33, 34, 35, 36, 39)** để đọc tín hiệu analog (cảm biến âm thanh, cảm biến dòng). Tất cả các chân thuộc **ADC2 bị vô hiệu hóa** hoàn toàn khi Wi-Fi được kích hoạt.
   - **Cấm sử dụng GPIO 6 – 11:** Đây là các chân kết nối trực tiếp với chip nhớ Flash SPI tích hợp bên trong module ESP32.
