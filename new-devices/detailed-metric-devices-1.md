# TỔNG HỢP CHI TIẾT THÔNG SỐ CÁC THIẾT BỊ

---

## 1. Module Relay 1 Kênh (5V)

* **Tên sản phẩm:** Module Relay 1 Kênh (Rơ-le 5V)
* **Tên tiếng Anh + mã:** 1-Channel 5V Relay Module (SRD-05VDC-SL-C)
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *1-Channel:* 1 kênh (điều khiển độc lập 1 thiết bị / tải đóng ngắt)
  * *5V:* Điện áp kích hoạt cuộn hút ($5\text{V DC}$)
  * *Relay:* Rơ-le (công tắc đóng ngắt bằng điện từ)
  * *Module:* Bo mạch chức năng tích hợp sẵn linh kiện điều khiển phụ trợ (transistor kích, opto cách ly, diode dập xung ngược)
  * *SRD-05VDC-SL-C:* Mã quy chuẩn kỹ thuật của rơ-le Songle
* **Giải thích lý do có tên và mã đó:**
  * **"1-Channel 5V Relay Module":** Đặt tên theo số lượng kênh đóng ngắt (1 kênh), mức điện áp kích hoạt cuộn hút ($5\text{V DC}$) và hình thức bo mạch chức năng hoàn chỉnh.
  * **"SRD-05VDC-SL-C":** Quy ước kỹ thuật của hãng sản xuất Songle:
    * **SRD:** Mã định danh dòng rơ-le gắn bo mạch (PCB relay) của hãng Songle.
    * **05VDC:** Điện áp danh định của cuộn hút là $5\text{V DC}$.
    * **SL:** Cấu trúc vỏ kín khí (Sealed type) chống bụi bẩn và oxy hóa tiếp điểm.
    * **C:** Tiếp điểm chuyển đổi dạng Form C (1 chân COM, 1 chân thường đóng NC, 1 chân thường mở NO - SPDT).
* **Danh sách các chân cắm (6 chân chia làm 2 cụm):**
  * **Đầu vào (Tín hiệu điều khiển & Nguồn - 3 chân):**
    1. **VCC:** Chân cấp nguồn dương (+5V DC) nuôi cuộn hút và mạch kích.
    2. **GND:** Chân tiếp địa (Cực âm, $0\text{V}$).
    3. **IN (hoặc S):** Chân nhận tín hiệu kích điều khiển từ vi điều khiển (thường hỗ trợ kích mức thấp Active LOW hoặc mức cao Active HIGH).
  * **Đầu ra (Tiếp điểm tải điện áp cao - 3 cọc vít):**
    4. **NO (Normally Open):** Tiếp điểm thường mở (chỉ đóng tiếp xúc với COM khi rơ-le được kích hoạt).
    5. **COM (Common):** Chân chung, thường nối với một đầu dây nguồn của tải (ví dụ dây pha $220\text{V AC}$).
    6. **NC (Normally Closed):** Tiếp điểm thường đóng (bình thường luôn nối thông với COM, khi rơ-le kích hoạt sẽ ngắt ra).
* **Tính ứng dụng:** Bật/tắt an toàn các thiết bị điện áp cao, dòng tải lớn (bóng đèn 220V, quạt gió, máy bơm nước gia đình) thông qua tín hiệu logic điện áp thấp của vi điều khiển (ESP32, Arduino).

---

## 2. Màn Hình LED Ma Trận 8x8

* **Tên sản phẩm:** Màn hình LED ma trận 8x8 (Module LED Matrix 8x8)
* **Tên tiếng Anh + mã:** 8x8 LED Matrix Module (tích hợp IC MAX7219)
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *8x8:* Kích thước lưới gồm 8 hàng và 8 cột (tổng cộng 64 điểm LED đơn)
  * *LED:* Light Emitting Diode (Diode phát quang)
  * *Matrix:* Ma trận (mô hình mạng lưới giao điểm giữa các hàng và cột)
  * *Module:* Bo mạch chức năng (thường tích hợp IC MAX7219 để quét LED qua giao tiếp nối tiếp)
* **Giải thích lý do có tên và mã đó:**
  * **"8x8 LED Matrix":** Đặt tên trực tiếp theo cấu trúc ma trận hiển thị gồm 64 bóng LED được bố trí đối xứng trên lưới 8 hàng dọc và 8 hàng ngang.
  * **"MAX7219" (nếu có bo module):** Là mã IC driver quét hiển thị ma trận nối tiếp đa kênh do Maxim Integrated thiết kế, giúp giải phóng chân GPIO cho vi điều khiển chỉ với 3 đường SPI.
* **Danh sách các chân cắm (5 chân giao tiếp chuẩn):**
  1. **VCC:** Chân cấp nguồn dương (+5V DC).
  2. **GND:** Chân tiếp địa (Cực âm, $0\text{V}$).
  3. **DIN (Data In):** Chân nhận dữ liệu nối tiếp từ vi điều khiển (chuẩn SPI).
  4. **CS (Chip Select / Load):** Chân chốt chọn chip, lưu dữ liệu nạp vào thanh ghi để hiển thị.
  5. **CLK (Clock):** Chân cấp xung nhịp đồng hồ đồng bộ hóa dữ liệu truyền.
  *(Khối LED trần thô gồm 16 chân vật lý: 8 chân Hàng (Row 1–8) và 8 chân Cột (Column 1–8) theo nguyên lý quét Anode/Cathode chung).*
* **Tính ứng dụng:** Hiển thị ký tự, con số, biểu đồ sóng âm, biểu tượng cảm xúc (pixel art), chữ chạy cuộn quảng cáo, màn hình hiển thị game cổ điển (Snake, Tetris).

---

## 3. Bàn Phím Ma Trận Nút Nhấn 4x4

* **Tên sản phẩm:** Bàn phím ma trận nút nhấn 4x4 (Keypad 4x4)
* **Tên tiếng Anh + mã:** 4x4 Matrix Button Keypad Module
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *4x4:* Kích thước ma trận gồm 4 hàng (Row) và 4 cột (Column), tạo thành 16 phím bấm
  * *Matrix:* Ma trận (cách đấu nối lưới dây để tiết kiệm số chân GPIO)
  * *Button / Key:* Phím bấm / Nút bấm cơ học
  * *Keypad:* Bàn phím số thu nhỏ
  * *Module:* Khối bàn phím tích hợp (dạng màng dẻo Membrane hoặc phím cơ trên mạch PCB)
* **Giải thích lý do có tên và mã đó:**
  * **"4x4 Matrix Button Keypad":** Tên gọi mô tả chính xác kết cấu kỹ thuật: 16 nút nhấn được nối theo lưới ma trận 4 hàng x 4 cột. Nhờ vậy chỉ cần 8 chân I/O để quét 16 nút thay vì cần 16 chân riêng biệt.
  * **"4x4":** Là quy cách hình học gồm 4 hàng x 4 cột tạo nên 16 phím chức năng (0–9, *, #, A, B, C, D).
* **Danh sách các chân cắm (8 chân giao tiếp):**
  1. **R1 (Row 1):** Đường quét hàng 1 (kết nối hàng phím 1, 2, 3, A).
  2. **R2 (Row 2):** Đường quét hàng 2 (kết nối hàng phím 4, 5, 6, B).
  3. **R3 (Row 3):** Đường quét hàng 3 (kết nối hàng phím 7, 8, 9, C).
  4. **R4 (Row 4):** Đường quét hàng 4 (kết nối hàng phím *, 0, #, D).
  5. **C1 (Column 1):** Đường đọc cột 1 (kết nối cột phím 1, 4, 7, *).
  6. **C2 (Column 2):** Đường đọc cột 2 (kết nối cột phím 2, 5, 8, 0).
  7. **C3 (Column 3):** Đường đọc cột 3 (kết nối cột phím 3, 6, 9, #).
  8. **C4 (Column 4):** Đường đọc cột 4 (kết nối cột phím A, B, C, D).
* **Tính ứng dụng:** Nhập mật khẩu mở khóa điện tử (Smart Lock), nhập tham số/dữ liệu trong các tủ điều khiển công nghiệp, máy tính mini tính toán, giao diện chọn menu tương tác.

---

## 4. Mạch Chuyển Đổi USB - UART (Micro USB to TTL)

* **Tên sản phẩm:** Mạch nạp & chuyển đổi USB sang UART / TTL (Mạch USB to TTL)
* **Tên tiếng Anh + mã:** Micro USB to TTL Serial Module (HW-896 V1.2)
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *Micro USB:* Cổng kết nối vật lý chuẩn Micro-B
  * *To:* Sang / Thành
  * *TTL:* Transistor-Transistor Logic (Mức điện áp logic số $0\text{V}/3.3\text{V}$ hoặc $0\text{V}/5\text{V}$)
  * *Serial:* Giao thức truyền thông nối tiếp bất đồng bộ (UART)
  * *Module:* Bo mạch chức năng
  * *HW-896 V1.2:* Mã nhận diện phần cứng (Hardware 896, Version 1.2)
* **Giải thích lý do có tên và mã đó:**
  * **"Micro USB to TTL Serial":** Đặt tên trực tiếp theo tính năng cầu nối chuyển đổi tín hiệu bus dữ liệu USB của máy tính thành giao tiếp nối tiếp chuẩn TTL (UART RX/TX) tương thích với vi điều khiển.
  * **"HW-896 V1.2":** Mã bo mạch do nhà sản xuất phần cứng quy chuẩn (HW viết tắt của Hardware, 896 là mã định danh bo, V1.2 là phiên bản phát hành phần cứng thứ 1.2).
* **Danh sách các chân cắm (5 – 6 chân):**
  1. **5V (hoặc VCC):** Chân cấp nguồn dương 5V DC lấy từ cổng USB máy tính.
  2. **3V3:** Chân cấp nguồn dương 3.3V DC (từ chip ổn áp tích hợp trên bo mạch).
  3. **TXD (Transmit Data):** Chân truyền tín hiệu dữ liệu UART (nối với chân RXD của vi điều khiển).
  4. **RXD (Receive Data):** Chân nhận tín hiệu dữ liệu UART (nối với chân TXD của vi điều khiển).
  5. **GND (Ground):** Chân tiếp địa mass chung (bắt buộc nối chung mass giữa máy tính và bo vi điều khiển).
  6. **DTR / RST (tùy phiên bản):** Tín hiệu Data Terminal Ready dùng để tự động Reset vi điều khiển khi nạp code.
* **Tính ứng dụng:** Nạp chương trình firmware cho các bo vi điều khiển không tích hợp sẵn chip nạp (Arduino Pro Mini, ESP8266 ESP-01, STM32), giao tiếp truyền nhận dữ liệu và debug qua Serial Monitor trên máy tính.

---

## 5. Cảm Biến Âm Thanh

* **Tên sản phẩm:** Module cảm biến âm thanh (Sound Sensor Module)
* **Tên tiếng Anh + mã:** Sound Sensor Module (HW-484 V0.2 / IC so sánh LM393)
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *Sound:* Âm thanh / Tiếng động
  * *Sensor:* Cảm biến (chuyển đổi rung động sóng âm thành tín hiệu điện)
  * *Module:* Bo mạch chức năng tích hợp sẵn microphone electret, chiết áp tinh chỉnh độ nhạy và IC so sánh LM393
  * *HW-484 V0.2:* Mã nhận diện bo mạch của nhà sản xuất (Hardware 484 phiên bản 0.2)
* **Giải thích lý do có tên và mã đó:**
  * **"Sound Sensor":** Đặt tên theo khả năng nhận diện cường độ sóng âm thanh dao động trong không khí thông qua đầu thu micro điện dung tích hợp.
  * **"HW-484 V0.2":** Mã định danh bo mạch do đơn vị thiết kế/chế tạo quy ước (Hardware 484 version 0.2).
* **Danh sách các chân cắm (4 chân):**
  1. **VCC (+):** Chân cấp nguồn dương (hoạt động từ +3.3V đến +5V DC).
  2. **GND (-):** Chân nối đất / cực âm ($0\text{V}$).
  3. **DO (Digital Output):** Ngõ ra tín hiệu số (mức HIGH/LOW), tự động đổi trạng thái khi cường độ âm thanh vượt quá ngưỡng cài đặt trên biến trở.
  4. **AO (Analog Output):** Ngõ ra tín hiệu tương tự, điện áp biến thiên liên tục tỉ lệ thuận theo biên độ sóng âm thu được tại micro.
* **Tính ứng dụng:** Bật/tắt đèn chiếu sáng thông minh bằng tiếng vỗ tay (Clap Switch), phát hiện tiếng gõ cửa, cảnh báo tiếng ồn vượt ngưỡng, cảm biến an ninh phát hiện xâm nhập.

---

## 6. Cảm Biến Siêu Âm Đo Khoảng Cách HC-SR04

* **Tên sản phẩm:** Cảm biến siêu âm đo khoảng cách HC-SR04
* **Tên tiếng Anh + mã:** Ultrasonic Distance Sensor (HC-SR04)
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *Ultrasonic:* Siêu âm (sóng âm thanh có tần số cao hơn 20kHz, ở đây là sóng 40kHz vượt ngưỡng tai người nghe)
  * *Distance:* Khoảng cách
  * *Sensor:* Cảm biến
  * *HC-SR04:* Mã tiêu chuẩn sản phẩm (HC: Dòng module cảm biến ngoại vi / SR: Sound Ranging - Đo tầm bằng âm thanh)
* **Giải thích lý do có tên và mã đó:**
  * **"Ultrasonic Distance Sensor":** Đặt tên theo cơ chế phát chùm sóng siêu âm $40\text{ kHz}$ và đo thời gian sóng phản xạ dội ngược lại ($t$) để suy ra khoảng cách ($d = \frac{v \times t}{2}$).
  * **"HC-SR04":** Mã hiệu sản phẩm tiêu chuẩn công nghiệp:
    * **HC:** Mã quy định dòng thiết bị cảm biến ngoại vi.
    * **SR:** **S**ound **R**anging (hệ thống đo cự ly định vị bằng sóng âm thanh).
    * **04:** Số hiệu thế hệ thiết kế phiên bản 4.
* **Danh sách các chân cắm (4 chân):**
  1. **VCC:** Chân cấp nguồn hoạt động (+5V DC).
  2. **Trig (Trigger):** Chân kích phát sóng siêu âm (nhận xung mức HIGH có độ rộng tối thiểu $10\mu\text{s}$ từ vi điều khiển để phát ra chuỗi 8 xung $40\text{ kHz}$).
  3. **Echo:** Chân ngõ ra phản hồi (giữ mức HIGH trong khoảng thời gian bằng đúng thời gian từ lúc sóng phát đi đến khi nhận được sóng dội về).
  4. **GND:** Chân tiếp địa (Cực âm nguồn, $0\text{V}$).
* **Tính ứng dụng:** Đo khoảng cách chính xác từ 2cm đến 400cm, phát hiện vật cản cho robot tránh chướng ngại vật, xe tự hành, đo mực nước không tiếp xúc trong bể chứa, hỗ trợ đỗ xe thông minh.

---

## 7. Module Cảm Biến Dòng Điện ACS712

* **Tên sản phẩm:** Module cảm biến dòng điện ACS712
* **Tên tiếng Anh + mã:** Current Sensor Module (Allegro ACS712)
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *Current:* Dòng điện (cường độ dòng điện chạy qua dây dẫn)
  * *Sensor:* Cảm biến
  * *Module:* Bo mạch hoàn chỉnh tích hợp IC cảm biến, tụ lọc nhiễu và cọc đấu dây vít (Terminal block)
  * *ACS712:* Mã chip cảm biến IC đo dòng hiệu ứng Hall của hãng Allegro MicroSystems
* **Giải thích lý do có tên và mã đó:**
  * **"Current Sensor":** Đặt tên theo nhiệm vụ đo lường cường độ dòng điện (hỗ trợ cả dòng một chiều DC lẫn xoay chiều AC).
  * **"ACS712":** Mã định danh chip tích hợp của nhà sản xuất Allegro MicroSystems:
    * **ACS:** **A**llegro **C**urrent **S**ensor (Dòng cảm biến đo dòng của hãng Allegro).
    * **712:** Dòng sản phẩm IC đo dòng cách ly điện thế cao với trở kháng dây đồng dẫn dòng cực thấp ($1.2\text{ m}\Omega$).
* **Danh sách các chân cắm (5 chân chia làm 2 cụm):**
  * **Cụm cọc vít đo dòng tải công suất cao (2 chân):**
    1. **IP+ (Cọc vít 1):** Cực dương đưa dòng điện tải vào (đấu nối tiếp trên đường dây dẫn tải).
    2. **IP- (Cọc vít 2):** Cực đưa dòng điện ra tới phụ tải cần cấp điện.
  * **Cụm chân tín hiệu kết nối vi điều khiển (3 chân):**
    3. **VCC:** Chân cấp nguồn dương hoạt động (+5V DC).
    4. **OUT (VOUT):** Chân xuất điện áp Analog tỉ lệ tuyến tính với dòng điện đi qua (khi không có dòng $I=0\text{A}$, điện áp ra là $\frac{V_{CC}}{2} \approx 2.5\text{V}$).
    5. **GND:** Chân nối đất ($0\text{V}$).
* **Tính ứng dụng:** Giám sát điện năng tiêu thụ, bảo vệ chống quá dòng và ngắt mạch, theo dõi tải hoạt động của động cơ, hệ thống quản lý năng lượng pin mặt trời (Solar Inverter).

---

## 8. Mạch Điều Khiển Động Cơ L298N

* **Tên sản phẩm:** Mạch điều khiển động cơ cầu H L298N (Driver L298N / ZX-040)
* **Tên tiếng Anh + mã:** Dual H-Bridge Motor Driver Module (L298N / ZX-040)
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *Dual:* Kép (gồm 2 mạch cầu H độc lập bên trong cùng 1 khối)
  * *H-Bridge:* Cầu H (cấu trúc mạch 4 transistor đóng mở tạo thành hình chữ H giúp đảo chiều dòng điện cấp cho động cơ)
  * *Motor Driver:* Mạch công suất khuếch đại dòng lái động cơ
  * *L298N / ZX-040:* Mã IC công suất L298N (STMicroelectronics) và mã thiết kế bo thương mại ZX-040
* **Giải thích lý do có tên và mã đó:**
  * **"Dual H-Bridge Motor Driver":** Đặt tên theo cấu trúc mạch điện: trang bị 2 bộ cầu H công suất cao có khả năng chịu dòng đỉnh đến 2A mỗi kênh, hỗ trợ băm xung PWM điều khiển tốc độ và đảo chiều quay.
  * **"L298N":** Mã linh kiện bán dẫn IC điều khiển của hãng STMicroelectronics (chữ N biểu thị dạng chân cắm Multiwatt15 đứng).
  * **"ZX-040":** Mã sản phẩm bo mạch tích hợp sẵn tản nhiệt nhôm do các xưởng sản xuất phần cứng quy chuẩn.
* **Danh sách các chân cắm:**
  * **Cụm cọc vít nguồn và cấp cho động cơ (Power & Motor Terminals):**
    1. **+12V (VCC Power):** Cấp nguồn động lực cho động cơ (nhận từ 5V đến 35V DC).
    2. **GND (Power Ground):** Chân mass chung nguồn (phải nối chung mass với vi điều khiển).
    3. **+5V:** Ngõ ra nguồn ổn áp 5V (từ IC hạ áp 78M05 tích hợp trên bo, dùng cấp cho vi điều khiển nếu có jumper 5V-EN) hoặc cấp nguồn 5V vào mạch logic nếu nguồn động cơ > 12V.
    4. **OUT1 & OUT2:** Cọc vít nối với động cơ DC thứ nhất (Motor A) hoặc pha động cơ bước.
    5. **OUT3 & OUT4:** Cọc vít nối với động cơ DC thứ hai (Motor B) hoặc pha động cơ bước.
  * **Cụm chân điều khiển tín hiệu logic (Control Pins Header):**
    6. **ENA:** Kích hoạt/Băm xung PWM điều tốc Motor A (rút jumper để cấp xung PWM).
    7. **IN1 & IN2:** Tín hiệu logic điều khiển chiều quay Motor A.
    8. **IN3 & IN4:** Tín hiệu logic điều khiển chiều quay Motor B.
    9. **ENB:** Kích hoạt/Băm xung PWM điều tốc Motor B (rút jumper để cấp xung PWM).
* **Tính ứng dụng:** Điều khiển chiều quay và tốc độ (PWM) cho 2 động cơ DC (tiến, lùi, phanh, băm xung PWM đổi tốc độ) cho robot xe 2 bánh / 4 bánh, hoặc lái 1 động cơ bước 2 pha 4 dây (như NEMA 17).

---

## 9. Module Thời Gian Thực RTC

* **Tên sản phẩm:** Module đồng hồ thời gian thực RTC (Real-Time Clock HW-111)
* **Tên tiếng Anh + mã:** I2C Real-Time Clock Module (HW-111 / DS1307 / DS3231)
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *Real-Time:* Thời gian thực (giờ, phút, giây, ngày, tháng, năm thực tế của đồng hồ thế giới)
  * *Clock:* Đồng hồ đếm thời gian
  * *I2C:* Giao thức truyền thông nối tiếp 2 dây (Inter-Integrated Circuit)
  * *HW-111 / DS1307 / DS3231:* Mã bo mạch (HW-111) và mã IC RTC chuyên dụng (DS1307 hoặc DS3231 độ chính xác cao bù nhiệt của hãng Maxim Integrated/Dallas)
* **Giải thích lý do có tên và mã đó:**
  * **"Real-Time Clock (RTC)":** Đặt tên theo nhiệm vụ lưu giữ và đếm thời gian liên tục, duy trì hoạt động ngay cả khi vi điều khiển mất điện nhờ viên pin cúc áo (CR2032).
  * **"HW-111":** Mã phần cứng bo mạch chế tạo tích hợp pin dự phòng và IC nhớ EEPROM (24C32).
  * **"DS1307 / DS3231":** Mã quy ước của hãng Dallas Semiconductor / Maxim Integrated: "DS" viết tắt của Dallas Semiconductor, con số đại diện cho thế hệ IC đồng hồ thời gian thực.
* **Danh sách các chân cắm (giao tiếp I2C):**
  1. **VCC:** Chân cấp nguồn dương (+5V DC hoặc +3.3V tùy phiên bản IC).
  2. **GND:** Chân tiếp địa mass ($0\text{V}$).
  3. **SCL (Serial Clock):** Chân xung nhịp đồng bộ của giao thức $I^2C$.
  4. **SDA (Serial Data):** Chân truyền/nhận dữ liệu của giao thức $I^2C$.
  5. **DS:** Chân kết nối cảm biến nhiệt độ Dallas 1-Wire (DS18B20 tích hợp tùy chọn trên một số bo mạch).
  6. **SQ / SQW (Square Wave Output):** Chân xuất xung vuông có thể lập trình tần số (1Hz, 4kHz, 8kHz, 32kHz) hoặc chân ngắt báo động (Alarm Interrupt).
* **Tính ứng dụng:** Duy trì thời gian chính xác cho đồng hồ điện tử để bàn, hệ thống hẹn giờ tưới cây tự động, máy chấm công, ghi nhật ký dữ liệu cảm biến (Data Logger) kèm mốc thời gian (timestamp).

---

## 10. Module Đọc Thẻ Từ RFID RC522

* **Tên sản phẩm:** Module đọc/ghi thẻ từ RFID RC522 13.56MHz
* **Tên tiếng Anh + mã:** RFID Reader Module 13.56MHz (RFID-RC522 / HW-126)
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *RFID:* Radio Frequency Identification (Nhận dạng đối tượng bằng sóng vô tuyến không dây)
  * *Reader:* Thiết bị đọc dữ liệu từ xa
  * *13.56MHz:* Tần số sóng mang vô tuyến tần số cao (HF - High Frequency) theo chuẩn thẻ Mifare
  * *RC522 / HW-126:* Mã IC truyền thông NXP MFRC522 và mã bo mạch sản xuất HW-126
* **Giải thích lý do có tên và mã đó:**
  * **"RFID Reader":** Đặt tên theo công nghệ nhận dạng không tiếp xúc qua sóng vô tuyến điện tử RFID tần số $13.56\text{ MHz}$.
  * **"RC522":** Mã định danh chip vi mạch giao tiếp không tiếp xúc MFRC522 do hãng NXP Semiconductors phát minh.
  * **"HW-126":** Mã bố trí mạch in (PCB Layout) do nhà sản xuất module linh kiện phần cứng đánh số thứ tự.
* **Danh sách các chân cắm (8 chân giao tiếp SPI):**
  1. **3.3V (VCC):** Chân cấp nguồn dương **+3.3V DC** (Lưu ý: Không được cấp 5V vì sẽ làm hỏng chip RC522).
  2. **RST (Reset):** Chân thiết lập lại phần cứng hoặc đưa chip vào chế độ tiết kiệm năng lượng (Power-down).
  3. **GND:** Chân nối đất ($0\text{V}$).
  4. **IRQ (Interrupt Request):** Chân ngắt cứng, phát tín hiệu khi có thẻ từ đi vào vùng từ trường anten.
  5. **MISO (Master In Slave Out):** Chân truyền dữ liệu từ RC522 về vi điều khiển (giao thức SPI).
  6. **MOSI (Master Out Slave In):** Chân truyền dữ liệu từ vi điều khiển sang RC522 (giao thức SPI).
  7. **SCK (Serial Clock):** Chân xung nhịp đồng bộ của bus SPI.
  8. **SDA / SS (Slave Select):** Chân chọn chip giao tiếp (CS) trên bus SPI.
* **Tính ứng dụng:** Hệ thống khóa cửa thông minh (Smart Lock), thẻ xe chung cư, máy chấm công nhân viên, quản lý kho hàng và vé xe buýt điện tử.

---

## 11. Bo Mạch Vi Điều Khiển Arduino Nano

* **Tên sản phẩm:** Bo mạch vi điều khiển Arduino Nano
* **Tên tiếng Anh + mã:** Arduino Nano Development Board (ATmega328P)
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *Arduino:* Tên riêng thương hiệu mã nguồn mở nổi tiếng (lấy cảm hứng từ quán bar Bar di Re Arduino tại Ý)
  * *Nano:* Tiếp đầu ngữ chỉ kích thước siêu nhỏ gọn ($10^{-9}$, ở đây mang nghĩa compact / tí hon)
  * *Development Board:* Bo mạch phát triển hoàn chỉnh gồm chip xử lý, mạch nạp, thạch anh và mạch ổn áp
  * *ATmega328P:* Mã định danh vi điều khiển 8-bit AVR của Atmel (nay thuộc Microchip)
* **Giải thích lý do có tên và mã đó:**
  * **"Arduino Nano":** Tên thương mại đại diện cho phiên bản bo mạch Arduino thu nhỏ tối đa diện tích, thiết kế chân cắm DIP cắm trực tiếp được lên testboard (breadboard).
  * **"ATmega328P":** Mã kỹ thuật của chip vi điều khiển:
    * **AT:** Viết tắt hãng Atmel.
    * **mega:** Dòng chip AVR ATmega có bộ nhớ lớn và nhiều tính năng ngoại vi.
    * **32:** Dung lượng bộ nhớ Flash là $32\text{ KB}$.
    * **8:** Độ dài từ dữ liệu 8-bit.
    * **P:** PicoPower (công nghệ tiết kiệm năng lượng nâng cao của Atmel).
* **Danh sách các chân cắm (30 chân chia làm 2 hàng):**
  1. **D0 – D13 (14 chân Digital I/O):** Các chân xuất/nhập kỹ thuật số (trong đó D3, D5, D6, D9, D10, D11 hỗ trợ băm xung PWM 8-bit; D0 là RX, D1 là TX).
  2. **A0 – A7 (8 chân Analog Input):** Các kênh đọc tín hiệu tương tự ADC 10-bit (nhiều hơn Arduino Uno 2 chân là A6, A7). Chân A4 (SDA) và A5 (SCL) dùng cho giao tiếp I2C.
  3. **VIN:** Chân nhận nguồn ngoài cấp cho bo (từ 7V đến 12V DC qua IC ổn áp).
  4. **5V:** Ngõ ra nguồn ổn áp 5V (hoặc cấp nguồn 5V trực tiếp).
  5. **3V3:** Ngõ ra nguồn điện 3.3V DC (dòng tối đa 50mA từ chip nạp USB).
  6. **GND (2 chân):** Chân tiếp địa nguồn ($0\text{V}$).
  7. **RST (Reset - 2 chân):** Kéo xuống mass để khởi động lại vi điều khiển.
  8. **AREF:** Chân cấp điện áp tham chiếu cho bộ chuyển đổi Analog-to-Digital.
* **Tính ứng dụng:** Bộ não xử lý trung tâm cho các mô hình tự chế, robot mini, máy đo đạc xách tay, thiết bị IoT nhỏ gọn yêu cầu cắm trên breadboard.

---

## 12. Màn Hình LCD 1602

* **Tên sản phẩm:** Màn hình tinh thể lỏng LCD 1602 (LCD 16x2)
* **Tên tiếng Anh + mã:** 16x2 Character LCD Display (HD44780 Driver)
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *16x2:* Hiển thị được 16 ký tự trên mỗi dòng và có 2 dòng hiển thị (tổng 32 ô ký tự 5x8 pixel)
  * *Character:* Ký tự chữ cái và số dạng ma trận điểm
  * *LCD:* Liquid Crystal Display (Màn hình tinh thể lỏng)
  * *1602:* Mã số viết tắt tiêu chuẩn biểu thị quy cách hiển thị 16 ký tự x 02 dòng
* **Giải thích lý do có tên và mã đó:**
  * **"Character LCD":** Màn hình tinh thể lỏng chuyên dụng để hiển thị các ký tự văn bản mã ASCII thay vì hiển thị hình ảnh đồ họa tự do.
  * **"1602":** Là mã chuẩn hóa công nghiệp biểu thị quy cách 16 ký tự mỗi dòng và 02 dòng văn bản (16 cột x 2 hàng).
  * **"HD44780":** Mã vi điều khiển điều khiển màn hình chuẩn do hãng Hitachi thiết kế và sản xuất.
* **Danh sách các chân cắm (16 chân chuẩn):**
  1. **VSS (Chân 1):** Chân nối đất ($0\text{V}$).
  2. **VDD (Chân 2):** Chân cấp nguồn dương (+5V DC).
  3. **V0 (Chân 3):** Chân điều chỉnh độ tương phản màn hình (thường nối qua biến trở $10\text{k}\Omega$).
  4. **RS (Chân 4 - Register Select):** Chọn thanh ghi dữ liệu (Data register) hoặc thanh ghi lệnh (Command register).
  5. **RW (Chân 5 - Read/Write):** Chọn chế độ đọc (HIGH) hoặc ghi dữ liệu (LOW) vào LCD.
  6. **E (Chân 6 - Enable):** Chân kích hoạt chốt tín hiệu dữ liệu.
  7. **D0 – D7 (Chân 7 đến 14 - Data Bus):** 8 đường bus dữ liệu song song (thường chỉ dùng 4 chân D4–D7 trong chế độ 4-bit).
  8. **A / BLA (Chân 15 - Anode):** Cực dương đèn nền màn hình (Backlight Anode +5V qua trở hạn dòng).
  9. **K / BLK (Chân 16 - Cathode):** Cực âm đèn nền màn hình (Backlight Cathode $0\text{V}$).
  *(Thường được hàn ghép thêm module I2C PCF8574 phía sau để rút gọn 16 chân xuống chỉ còn 4 chân: VCC, GND, SDA, SCL).*
* **Tính ứng dụng:** Hiển thị thông số môi trường (nhiệt độ, độ ẩm), đồng hồ thời gian, trạng thái kết nối Wi-Fi, tin nhắn thông báo trong các máy trạm và dự án điện tử.

---

## 13. Bo Mạch Vi Điều Khiển STM32 "Black Pill"

* **Tên sản phẩm:** Bo mạch vi điều khiển STM32F4 "Black Pill" (STM32 ARM Cortex-M4)
* **Tên tiếng Anh + mã:** STM32 ARM Cortex-M4 Development Board (STM32F401CCU6 / STM32F411CEU6 "Black Pill")
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *STM32:* Bo vi điều khiển 32-bit của tập đoàn STMicroelectronics
  * *ARM Cortex-M4:* Kiến trúc nhân vi xử lý 32-bit hiệu năng cao tích hợp bộ xử lý số thực FPU (Floating Point Unit)
  * *Development Board:* Bo mạch phát triển hệ thống nhúng
  * *Black Pill:* "Viên thuốc đen" (biệt danh do cộng đồng đặt dựa theo hình dáng bo mạch thuôn dài màu đen)
* **Giải thích lý do có tên và mã đó:**
  * **"STM32":** Tên thương hiệu viết tắt từ **ST**Microelectronics + **32**-bit Architecture.
  * **"Black Pill":** Biệt danh để phân biệt với thế hệ trước là "Blue Pill" (STM32F103 mạch màu xanh dương). "Black Pill" có PCB màu đen, sử dụng chip thế hệ F4 mạnh mẽ hơn nhiều lần và dùng cổng USB Type-C hiện đại.
  * **"F401 / F411":** Dòng chip STM32F4 hiệu năng cơ bản (Foundation Line) chạy xung nhịp lên đến 84MHz – 100MHz.
* **Danh sách các chân cắm (khoảng 40 chân hai hàng + cổng SWD debug):**
  1. **PA0 – PA15 (Port A):** Cụm chân I/O đa năng Port A (hỗ trợ ADC, PWM, UART, SPI, I2C, USB OTG).
  2. **PB0 – PB15 (Port B):** Cụm chân I/O đa năng Port B.
  3. **PC13 – PC15 (Port C):** Các chân I/O điều khiển LED trên bo (PC13 nối LED tích hợp) và chân nối thạch anh 32.768kHz RTC.
  4. **5V / 3V3 / GND:** Các chân cấp nguồn vào 5V, xuất nguồn áp 3.3V và chân nối đất.
  5. **NRST:** Chân phần cứng Reset chip.
  6. **Cổng nạp SWD (4 chân):** 3V3, SWCLK (Serial Wire Clock), SWDIO (Serial Wire Data In/Out), GND dùng để nạp code và debug tốc độ cao qua mạch ST-Link.
  7. **Cổng USB Type-C:** Dùng cấp nguồn và giao tiếp trực tiếp qua tính năng USB Native (DFU bootloader, HID, Virtual COM).
* **Tính ứng dụng:** Xử lý tín hiệu số (DSP), giải thuật toán học phức tạp, điều khiển thiết bị bay không người lái (Drone Flight Controller), bàn phím cơ tùy biến (QMK/VIA), xử lý âm thanh kỹ thuật số.

---

## 14. Màn Hình LED 7 Đoạn 4 Số TM1637

* **Tên sản phẩm:** Màn hình LED 7 đoạn 4 chữ số (Module TM1637)
* **Tên tiếng Anh + mã:** 4-Digit 7-Segment LED Display Module (TM1637)
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *4-Digit:* 4 chữ số hiển thị
  * *7-Segment:* 7 đoạn LED (mỗi chữ số được tạo thành từ 7 thanh LED con gán nhãn a, b, c, d, e, f, g kèm dấu hai chấm đồng hồ)
  * *LED Display:* Màn hình hiển thị bằng diode phát quang
  * *TM1637:* Mã chip IC điều khiển quét LED và đọc phím tích hợp trên bo mạch của hãng Titan Micro Electronics
* **Giải thích lý do có tên và mã đó:**
  * **"4-Digit 7-Segment":** Tên gọi định danh chính xác đặc điểm vật lý gồm cụm 4 bóng đèn LED 7 thanh ghép liền kề nhau có thể hiển thị số từ 0000 đến 9999 hoặc đồng hồ đếm giờ.
  * **"TM1637":** Mã IC driver chuyên dụng của hãng Titan Micro Electronics. Nhờ có chip này, toàn bộ 4 chữ số 7 thanh (vốn cần ít nhất 12 chân để quét) được thu gọn lại điều khiển chỉ bằng 2 chân tín hiệu.
* **Danh sách các chân cắm (4 chân giao tiếp):**
  1. **VCC:** Chân cấp nguồn dương (+3.3V đến +5V DC).
  2. **GND:** Chân nối đất ($0\text{V}$).
  3. **DIO (Data Input/Output):** Chân truyền và nhận dữ liệu nối tiếp điều khiển hiển thị.
  4. **CLK (Clock):** Chân cấp xung nhịp đồng bộ truyền dữ liệu (giao thức tương tự $I^2C$ không có địa chỉ).
* **Tính ứng dụng:** Hiển thị đồng hồ thời gian số (HH:MM với dấu hai chấm nhấp nháy), máy đếm ngược thời gian (Timer), máy đếm sản phẩm trong dây chuyền, hiển thị điểm số trò chơi, đo điểm số và điện áp.

---

## 15. Loa Mini / Còi Bíp Gốm Áp Điện

* **Tên sản phẩm:** Loa mini / Còi bíp gốm áp điện (Piezo Buzzer / Mini Speaker)
* **Tên tiếng Anh + mã:** Mini Speaker / Piezo Buzzer Module (Passive / Active Buzzer)
* **Phân tích nghĩa từng từ trong tên tiếng Anh:**
  * *Mini:* Siêu nhỏ / Kích thước thu gọn
  * *Speaker:* Loa phát âm thanh đa âm tần
  * *Piezo:* Piezoelectric (Hiện tượng áp điện - màng gốm thạch anh biến đổi dao động điện áp thành dao động cơ học tạo sóng âm)
  * *Buzzer:* Chuông còi phát âm thanh báo hiệu
  * *Module:* Bo mạch chức năng (thường có sẵn transistor đệm dòng kích âm thanh)
* **Giải thích lý do có tên và mã đó:**
  * **"Piezo Buzzer / Speaker":** Đặt tên theo nguyên lý tạo âm thanh: sử dụng màng gốm áp điện (Piezoelectric ceramic) hoặc cuộn dây điện từ rung động khi có dòng điện biến thiên chạy qua để phát ra tiếng bíp hoặc nốt nhạc.
  * **Phân biệt Active Buzzer vs Passive Buzzer:**
    * *Active Buzzer (Còi chủ động):* Đã tích hợp sẵn mạch dao động bên trong, chỉ cần cấp nguồn DC là tự động phát tiếng bíp đơn tần.
    * *Passive Buzzer (Còi bị động / Loa mini):* Không có mạch dao động, cần vi điều khiển băm xung PWM ở các tần số khác nhau để phát ra các nốt nhạc trầm bổng (Do, Re, Mi...).
* **Danh sách các chân cắm (2 chân trên còi rời / 3 chân trên module):**
  1. **VCC (hoặc Chân +):** Chân cấp nguồn dương (+3.3V đến +5V DC) hoặc nhận xung PWM từ vi điều khiển.
  2. **GND (hoặc Chân -):** Chân tiếp địa mass ($0\text{V}$).
  3. **I/O (hoặc S / Signal - nếu là module 3 chân):** Chân nhận tín hiệu kích hoặc xung tần số âm thanh từ vi điều khiển (chân VCC và GND cấp nguồn riêng).
* **Tính ứng dụng:** Phát âm thanh cảnh báo (báo động cháy, trộm cắp, rò rỉ khí gas), âm thanh xác nhận thao tác bấm nút, phát nhạc đơn điệu (8-bit chiptune) cho các dự án game mini.
