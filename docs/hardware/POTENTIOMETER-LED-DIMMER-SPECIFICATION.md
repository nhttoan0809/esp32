# Tài Liệu Thiết Kế Kỹ Thuật: Ứng Dụng Biến Trở Điều Chỉnh Độ Sáng LED (Potentiometer LED Dimmer)

Tài liệu này ghi nhận chi tiết toàn bộ cơ sở lý thuyết, công thức tính toán vật lý, thông số thiết kế mạch và giải thuật lập trình cho ứng dụng điều khiển độ sáng bóng đèn LED thông qua biến trở xoay. Dự án sử dụng bo mạch **ESP32 DevKit V1 (30 chân)** kết hợp các linh kiện cố định trong bộ kit thí nghiệm theo chuẩn [`docs/hardware/KIT-COMPONENTS-REFERENCE.md`](KIT-COMPONENTS-REFERENCE.md).

---

## 1. Danh Mục Linh Kiện & Thông Số Kỹ Thuật Cố Định

| Linh kiện | Thông số kỹ thuật danh định | Nguồn gốc trong Kit | Ghi chú & Vai trò |
|---|---|---|---|
| **Vi điều khiển** | ESP32 DevKit V1 (30-pin, WROOM-32) | Bo mạch chính | Điện áp logic 3.3V, ADC1 12-bit (0–4095), LEDC PWM |
| **Bóng đèn LED** | LED đơn 5mm màu Đỏ (Red) | Mục 3.3 trong Kit | $V_F \approx 2.0\text{V}$, dòng định mức an toàn $\le 12\text{ mA}$ |
| **Điện trở thuần** | Điện trở cố định $220\Omega \pm 5\%$ (1/4W) | Mục 4 trong Kit (10 con) | Hạn dòng bảo vệ LED và chân GPIO |
| **Biến trở xoay** | Chiết áp xoay 3 chân $10\text{k}\Omega$ | Mục 4 trong Kit (1 con) | Bộ phân áp lấy mẫu góc xoay $0\text{V} \rightarrow 3.3\text{V}$ |
| **Breadboard** | MB-102 (830 lỗ cắm) | Mục 4 trong Kit | Bo cắm mạch thử nghiệm không cần hàn |
| **Dây nối** | Cáp Jumper Đực - Đực | Mục 4 trong Kit | Dây dẫn liên kết các chân linh kiện |

---

## 2. Tính Toán Thông Số Mạch Điện Chi Tiết

```text
       ESP32 DevKit V1 (30-Pin)
      ┌────────────────────────┐
      │                        │
      │   3V3 ───┐             │
      │          │ (3.3V)      │
      │   GND ───┼──────────┐  │
      │          │          │  │
      │  GPIO34 ─┼──────┐   │  │
      │          │      │   │  │
      │  GPIO18 ─┼───┐  │   │  │
      └──────────┼───┼──┼───┼──┘
                 │   │  │   │
      ┌──────────┘   │  │   │
  (1) │              │  │   │
 ┌────┴────┐         │  │   │
 │ 10kΩ    ├──(2)────┘  │   │  (Wiper -> GPIO 34 ADC1)
 │ Chiết áp│            │   │
 └────┬────┘            │   │
  (3) │                 │   │
      └─────────────────┼───┘  (GND)
                        │
                        │
                  ┌─────┴─────┐
                  │ R = 220Ω  │
                  └─────┬─────┘
                        │ Anode (+)
                     ───┴───
                     \     /  LED Đỏ (VF ≈ 2.0V)
                      \   /
                     ───┬───
                        │ Cathode (-)
                        ▼
                       GND
```

### 2.1 Cường độ dòng điện qua LED ($I_{LED}$) & Chọn điện trở thuần ($R_{limit}$)

Mạch ngõ ra điều khiển LED gồm: Chân **GPIO 18** của ESP32 $\rightarrow$ Điện trở thuần $R_{limit}$ $\rightarrow$ Cực Anode LED $\rightarrow$ Cực Cathode LED $\rightarrow$ **GND**.

Khi GPIO 18 xuất mức logic `HIGH`:
- Điện áp ngõ ra của chân: $V_{OH} = 3.3\text{V}$.
- Điện áp phân cực thuận của LED đỏ: $V_F \approx 2.0\text{V}$.
- Điện áp rơi trên điện trở thuần:
  $$V_R = V_{OH} - V_F = 3.3\text{V} - 2.0\text{V} = 1.3\text{V}$$

Áp dụng định luật Ohm:
$$I_{LED} = \frac{V_R}{R_{limit}} = \frac{V_{OH} - V_F}{R_{limit}}$$

#### So sánh các giá trị điện trở có sẵn trong Kit:
1. **Trường hợp chọn $R_{limit} = 220\Omega$:**
   $$I_{LED} = \frac{1.3\text{V}}{220\Omega} \approx 0.00591\text{ A} = \mathbf{5.91\text{ mA}} \approx \mathbf{6\text{ mA}}$$
   - **Đánh giá mức an toàn:** ESP32 Technical Reference quy định dòng khuyến nghị tối đa cho mỗi chân GPIO là $12\text{ mA}$ (tuyệt đối không vượt quá $40\text{ mA}$). Dòng $5.91\text{ mA}$ chỉ bằng $\approx 50\%$ ngưỡng khuyến nghị $\Rightarrow$ **Rất an toàn cho vi điều khiển**.
   - **Độ sáng:** Dòng $6\text{ mA}$ đủ để LED 5mm đạt độ sáng phát quang rõ ràng, sắc nét mà không bị chói lóa hay quá nhiệt.
   - **Công suất tiêu tán trên điện trở $220\Omega$:**
     $$P_R = I_{LED}^2 \cdot R = (0.00591)^2 \cdot 220 \approx 7.68\text{ mW} = 0.00768\text{ W}$$
     Điện trở trong kit có công suất định mức $0.25\text{ W} = 250\text{ mW}$. Do đó $P_R \ll 250\text{ mW}$ (chỉ chiếm $3\%$ công suất danh định), điện trở hoàn toàn mát, không sinh nhiệt.
   - **Công suất tiêu tán trên LED:**
     $$P_{LED} = V_F \cdot I_{LED} = 2.0\text{V} \cdot 0.00591\text{ A} \approx 11.82\text{ mW}$$
     Nằm hoàn toàn trong ngưỡng chịu đựng của LED 5mm ($70 - 100\text{ mW}$).

2. **Trường hợp thử nghiệm các điện trở khác trong Kit:**
   - Với điện trở $1\text{k}\Omega$:
     $$I_{LED} = \frac{1.3\text{V}}{1000\Omega} = 1.3\text{ mA}$$
     Dòng điện $1.3\text{ mA}$ quá thấp, hiệu suất phát quang của LED rất yếu, mắt thường chỉ nhìn thấy một chấm mờ trong phòng tối.
   - Với điện trở $10\text{k}\Omega$:
     $$I_{LED} = \frac{1.3\text{V}}{10000\Omega} = 0.13\text{ mA} = 130\mu\text{A}$$
     Dòng điện không đủ để kích hoạt vùng phát quang của chất bán dẫn, đèn coi như tắt.

> 🎯 **Kết luận 1:** Sử dụng điện trở thuần **$R = 220\Omega$**. Cường độ dòng điện qua LED khi sáng nhất đạt **$I_{LED\_max} \approx 5.91\text{ mA}$**.

---

### 2.2 Biến trở nguồn điện vào bao nhiêu? ($V_{in\_pot}$)

Biến trở $10\text{k}\Omega$ có 3 chân được nối thành mạch cầu phân áp (Voltage Divider):
- Chân 1 (chân bìa): Nối vào nguồn **3V3** (3.3V DC của ESP32).
- Chân 3 (chân bìa đối diện): Nối vào **GND** (0V).
- Chân 2 (chân giữa / con chạy Wiper): Nối vào chân đọc Analog **GPIO 34** (ADC1).

#### Phân tích điện áp cấp vào biến trở:
- **Nguồn cấp bắt buộc: $3.3\text{V}$ (Chân 3V3 của ESP32).**
- ⚠️ **Cảnh báo an toàn (Tuyệt đối không cấp 5V / VIN vào biến trở):**
  1. Các chân GPIO và khối ADC của ESP32 được chế tạo trên tiến trình CMOS điện áp thấp, dải điện áp ngõ vào chịu đựng tối đa chỉ là:
     $$V_{IN\_max} = V_{DD} + 0.3\text{V} = 3.3\text{V} + 0.3\text{V} = 3.6\text{V}$$
  2. Nếu vô tình nối biến trở vào nguồn 5V (VIN), khi xoay núm về phía cực đại, chân giữa sẽ đưa thẳng điện áp $5.0\text{V}$ vào GPIO 34. Điều này sẽ làm đánh thủng diode bảo vệ ESD nội bộ, gây cháy vĩnh viễn kênh ADC và làm hỏng chip ESP32 ngay lập tức.
  3. Khi cấp đúng nguồn 3.3V, dải điện áp ngõ ra tại con chạy biến trở biến thiên chuẩn xác từ **$0.0\text{V}$ đến $3.3\text{V}$**, hoàn toàn tương thích $100\%$ với dải đo thang đo của bộ chuyển đổi ADC ESP32.

#### Tính toán dòng rò và công suất tiêu thụ của biến trở 10kΩ:
- Điện trở toàn phần giữa 2 chân bìa là không đổi ($R_{pot} = 10\text{k}\Omega = 10,000\Omega$).
- Dòng điện tĩnh chạy qua biến trở từ 3V3 xuống GND:
  $$I_{pot} = \frac{V_{in\_pot}}{R_{pot}} = \frac{3.3\text{V}}{10000\Omega} = 0.00033\text{ A} = \mathbf{0.33\text{ mA}} = \mathbf{330\mu\text{A}}$$
- Công suất nhiệt tiêu tán trên biến trở:
  $$P_{pot} = \frac{(V_{in\_pot})^2}{R_{pot}} = \frac{3.3^2}{10000} = \frac{10.89}{10000} \approx \mathbf{1.089\text{ mW}}$$
  Mức công suất $\approx 1.09\text{ mW}$ là cực kỳ nhỏ (biến trở thông thường chịu được $100 - 250\text{ mW}$). Biến trở hoàn toàn mát, dòng rò $0.33\text{ mA}$ không làm ảnh hưởng đến nguồn cấp hệ thống.
- **Trở kháng vào của ADC:** Chân GPIO 34 ở chế độ analog có trở kháng vào cực lớn ($> 10\text{ M}\Omega$), do đó dòng điện trích qua chân con chạy (Wiper) vào ADC là không đáng kể ($< 0.1\mu\text{A}$), không gây méo điện áp phân áp.

> 🎯 **Kết luận 2:** Nguồn điện cấp vào biến trở là **$3.3\text{V}$ (3V3)**. Dòng điện tiêu thụ tĩnh là **$0.33\text{ mA}$**, công suất tiêu tán **$1.09\text{ mW}$**.

---

### 2.3 Range từ Tối thiểu đến Tối đa của Biến Trở để Bóng Đèn từ Tắt đến Sáng Nhất

Để trả lời chuẩn xác câu hỏi này, ta cần phân tích trên hai mô hình: **Hệ thống điều khiển số ESP32 (Kiến trúc chuẩn của POC)** và **Mạch thuần phần cứng (Phân tích vật lý đối chiếu)**.

#### A. Trong kiến trúc hệ thống nhúng ESP32 (POC Chuẩn)
Trong mô hình này, biến trở đóng vai trò cảm biến vị trí góc xoay (Sensor), ESP32 đọc giá trị qua bộ ADC1 và xuất xung điều chế độ rộng PWM (LEDC) điều khiển công suất LED.

1. **Dải điện trở của biến trở giữa chân Wiper và GND ($R_{W-GND}$):**
   - **Tối thiểu:** $R_{W-GND} = \mathbf{0\Omega}$ (Núm xoay vặn hết cỡ về phía GND, góc quay 0%).
   - **Tối đa:** $R_{W-GND} = \mathbf{10\text{k}\Omega}$ (Núm xoay vặn hết cỡ về phía 3V3, góc quay 100%).

2. **Dải điện áp đưa vào chân ADC (GPIO 34):**
   $$V_{ADC} = 3.3\text{V} \times \frac{R_{W-GND}}{10\text{k}\Omega}$$
   - **Tối thiểu:** $V_{ADC\_min} = \mathbf{0.00\text{V}}$ $\Rightarrow$ Tương ứng trạng thái **TẮT HOÀN TOÀN**.
   - **Tối đa:** $V_{ADC\_max} = \mathbf{3.30\text{V}}$ $\Rightarrow$ Tương ứng trạng thái **SÁNG NHẤT**.

3. **Dải số đọc ADC 12-bit của ESP32 ($0 \rightarrow 4095$):**
   - Do đặc tính vật lý phi tuyến (Non-linearity) của bộ chuyển đổi SAR ADC trên ESP32:
     - Dải điện áp cận dưới $< 0.05\text{V}$ thường cho giá trị ADC dao động $0 - 35$ kèm nhiễu sàn (noise floor).
     - Dải điện áp cận trên $> 3.25\text{V}$ bắt đầu bão hòa ở mức $4050 - 4095$.
   - **Thuật toán xử lý vùng chết (Dead-zone & Hysteresis Filtering) trong firmware:**
     - Khi $ADC \le 35$ (tương ứng $R_{W-GND} \le 85\Omega$, góc xoay $< 1\%$): Firmware gán cứng `Duty Cycle = 0%`. Dòng trung bình qua LED $I_{avg} = 0\text{ mA} \Rightarrow$ **Bóng đèn tắt đen tuyệt đối**, loại bỏ hoàn toàn hiện tượng LED nhấp nháy do nhiễu sàn ADC.
     - Khi $ADC \ge 4050$ (tương ứng $R_{W-GND} \ge 9.88\text{k}\Omega$, góc xoay $> 99\%$): Firmware gán cứng `Duty Cycle = 100%`. Dòng trung bình qua LED $I_{avg} = 5.91\text{ mA} \Rightarrow$ **Bóng đèn sáng cực đại ổn định**.
     - Khi $35 < ADC < 4050$: Ánh xạ tuyến tính mượt mà dải giá trị sang Duty Cycle từ $0$ đến $4095$.

4. **Dải điều chế xung LEDC PWM (Tần số 5 kHz, Độ phân giải 12-bit):**
   $$\text{Duty Cycle (\%)} = \frac{\text{LEDC\_Duty}}{4095} \times 100\%$$
   - **Tại mức tối thiểu:** `LEDC_Duty = 0` $\rightarrow$ Tỷ lệ đóng điện 0% $\rightarrow$ Dòng qua LED $I_{LED} = 0\text{ mA}$ $\rightarrow$ **Đèn TẮT**.
   - **Tại mức 50% hành trình:** `LEDC_Duty = 2048` $\rightarrow$ Tỷ lệ đóng điện 50% $\rightarrow$ Dòng trung bình $I_{LED} \approx 2.95\text{ mA}$ $\rightarrow$ **Đèn sáng trung bình**.
   - **Tại mức tối đa:** `LEDC_Duty = 4095` $\rightarrow$ Tỷ lệ đóng điện 100% $\rightarrow$ Dòng trung bình $I_{LED} \approx 5.91\text{ mA}$ $\rightarrow$ **Đèn SÁNG NHẤT**.

---

#### B. So sánh đối chiếu với Mạch thuần phần cứng (Không qua vi điều khiển)
Để hiểu rõ tại sao cần dùng ESP32, hãy xem xét nếu chỉ dùng biến trở và nguồn 3.3V cấp trực tiếp cho LED:

##### Cách 1: Mắc biến trở nối tiếp như con trở điều chỉnh dòng (Rheostat Mode)
Mạch gồm: $3.3\text{V} \rightarrow R_{limit} (220\Omega) \rightarrow R_{pot} (0 - 10\text{k}\Omega) \rightarrow \text{LED} \rightarrow \text{GND}$.
- Khi vặn biến trở về $0\Omega$: Dòng qua LED là $I = (3.3 - 2.0) / 220 = 5.91\text{ mA}$ (Sáng nhất).
- Khi vặn biến trở kịch kim $10\text{k}\Omega$:
  $$I_{min} = \frac{3.3\text{V} - 1.8\text{V}}{10000\Omega + 220\Omega} \approx \frac{1.5\text{V}}{10220\Omega} \approx 0.000147\text{ A} = \mathbf{147\mu\text{A}}$$
  - ❌ **Hạn chế nghiêm trọng:** Dòng điện $147\mu\text{A}$ vẫn đủ lớn để kích thích chất bán dẫn của LED phát quang mờ trong bóng tối. Muốn đèn tắt hẳn ($I < 1\mu\text{A}$), tổng điện trở mạch phải đạt trên $1.5\text{ M}\Omega$! Vì vậy, **nếu mắc nối tiếp, biến trở $10\text{k}\Omega$ trong kit KHÔNG THỂ làm bóng đèn tắt hẳn**.

##### Cách 2: Mắc biến trở dạng cầu phân áp thuần cấp cho LED
Nối chân 1 vào 3.3V, chân 3 vào GND, chân giữa nối qua điện trở 220Ω vào LED.
- Điện áp rơi thuận của LED đỏ cần ít nhất $V_{th} \approx 1.6\text{V}$ để bắt đầu có dòng rò rỉ dẫn điện.
- Khi $V_{Wiper} < 1.6\text{V}$: LED tắt hoàn toàn. Điều này tương ứng với:
  $$R_{W-GND} < 10\text{k}\Omega \times \frac{1.6\text{V}}{3.3\text{V}} \approx \mathbf{4.85\text{k}\Omega}$$
  - ❌ **Hạn chế:** Khoảng từ **$0\Omega$ đến $4.85\text{k}\Omega$ (gần $50\%$ vòng quay núm vặn)** là "vùng chết hoàn toàn" — đèn không sáng một chút nào. Chỉ từ $4.85\text{k}\Omega$ đến $10\text{k}\Omega$ đèn mới bắt đầu sáng và tăng dần.

> 🎯 **Kết luận 3:**
> - Trong **mạch thuần phần cứng**, dải để đèn tắt là $0\Omega \rightarrow 4.85\text{k}\Omega$, dải để đèn chuyển từ tắt sang sáng nhất là $4.85\text{k}\Omega \rightarrow 10\text{k}\Omega$ (lãng phí 50% góc xoay).
> - Trong **hệ thống nhúng ESP32**, firmware giải quyết trọn vẹn nhược điểm này: Range sử dụng trải dài toàn bộ **$0\Omega \rightarrow 10\text{k}\Omega$ ($0\% \rightarrow 100\%$ góc xoay núm vặn)**, tương ứng điện áp $0\text{V} \rightarrow 3.3\text{V}$, PWM Duty Cycle $0\% \rightarrow 100\%$, đưa đèn chuyển động từ **TẮT TUYỆT ĐỐI $\rightarrow$ SÁNG NHẤT TUYỆT ĐỐI**.

---

## 3. Sơ Đồ Nối Dây Chi Tiết Trên Breadboard MB102

| Chân ESP32 DevKit V1 | Linh kiện đích | Chân linh kiện | Màu dây quy ước | Mục đích kỹ thuật |
|---|---|---|:---:|---|
| **3V3** (Chân 16 bên phải) | Biến trở $10\text{k}\Omega$ | Chân 1 (Bìa trái) | Đỏ (Red) | Cấp điện áp chuẩn 3.3V cho cầu phân áp |
| **GND** (Chân 17 bên phải) | Biến trở $10\text{k}\Omega$ | Chân 3 (Bìa phải) | Đen (Black) | Chân mass chuẩn (0V) |
| **D34 / GPIO34** (Chân 4 bên trái) | Biến trở $10\text{k}\Omega$ | Chân 2 (Chân giữa) | Vàng (Yellow) | Tín hiệu điện áp analog đưa vào kênh ADC1 |
| **D18 / GPIO18** (Chân 24 bên phải) | Điện trở thuần $220\Omega$ | Đầu 1 của trở | Xanh (Blue) | Ngõ ra xung PWM LEDC tần số 5 kHz |
| *(Nối tiếp trên breadboard)* | Điện trở $220\Omega$ (Đầu 2) | LED Đỏ | Chân Anode (Chân dài) | Mắc nối tiếp hạn dòng bảo vệ LED |
| **GND** (Chân 14 bên trái) | LED Đỏ | Chân Cathode (Chân ngắn/vát mép) | Đen (Black) | Hồi tiếp dòng về cực âm nguồn |

---

## 4. Kiến Trúc Phần Mềm & Giải Thuật Xử Lý

```text
  ┌─────────────────────────┐
  │ Biến trở xoay 10kΩ      │ (V_out: 0V - 3.3V)
  └────────────┬────────────┘
               │
               ▼
  ┌─────────────────────────┐
  │ ADC1 (GPIO 34, 12-bit)  │ (Raw Value: 0 - 4095)
  └────────────┬────────────┘
               │
               ▼
  ┌─────────────────────────┐
  │ Bộ lọc số EMA           │ EMA = α * Raw + (1 - α) * Prev
  │ (Exponential Filter)    │ (Triệt tiêu hiện tượng rung nháy Jitter)
  └────────────┬────────────┘
               │
               ▼
  ┌─────────────────────────┐
  │ Dead-zone & Range Map   │ Nếu Raw <= 35   -> Duty = 0    (Tắt hẳn)
  │ (Hysteresis Window)     │ Nếu Raw >= 4050 -> Duty = 4095 (Sáng nhất)
  └────────────┬────────────┘
               │
               ▼
  ┌─────────────────────────┐
  │ LEDC PWM Hardware Engine│ 5 kHz @ 12-bit resolution
  │ (GPIO 18 -> R 220Ω)     │ (Mắt người nhìn mượt mà, không chớp nháy)
  └────────────┬────────────┘
               │
               ▼
  ┌─────────────────────────┐
  │ Serial Monitor Logger   │ In biểu đồ thanh ASCII trực quan:
  │ (115200 baud)           │ [████████░░] 80% | ADC: 3276 | 2.64V | 4.73mA
  └─────────────────────────┘
```

### 4.1 Bộ lọc số trung bình trượt mũ (Exponential Moving Average - EMA)
Do tính chất của điện trở than trong biến trở và nhiễu nhiệt cao tần của bộ ADC vi điều khiển, giá trị đọc thô `analogRead(34)` thường bị dao động ngẫu nhiên $\pm 10 - 20$ LSB.
Để thanh độ sáng LED không bị chớp giật li ti, firmware sử dụng bộ lọc EMA:
$$\text{EMA}_t = \alpha \cdot \text{Sample}_t + (1 - \alpha) \cdot \text{EMA}_{t-1}$$
Với $\alpha = 0.25$, phản hồi núm vặn tức thì trong vòng $< 10\text{ms}$ nhưng loại bỏ hoàn toàn nhiễu giật rung.

### 4.2 Cấu hình phần cứng LEDC PWM
- **Kênh:** Channel 0.
- **Tần số (Frequency):** $5000\text{ Hz}$ ($5\text{ kHz}$). Tần số này cao hơn rất nhiều so với tần số nhận biết của mắt người ($60 - 100\text{ Hz}$) và tần số thu hình của camera điện thoại, đảm bảo không có hiện tượng gợn sóng hay nhấp nháy.
- **Độ phân giải (Resolution):** 12-bit ($0 \rightarrow 4095$). Đồng bộ độ phân giải $1:1$ trực tiếp với giá trị ADC 12-bit mà không cần chia nhỏ dải đo làm mất độ mịn.

---

## 5. Tiêu Chuẩn Kiểm Chứng & Nghiệm Thu

1. **Kiểm tra biên dịch:** `pio run -d pocs/poc-potentiometer-dimmer -e esp32dev` phải hoàn tất không có cảnh báo/lỗi (exit code 0).
2. **Kiểm tra sơ đồ mạch Wokwi:** `wokwi-cli lint pocs/poc-potentiometer-dimmer` hợp lệ.
3. **Kiểm tra hành vi thực nghiệm:**
   - Khi vặn núm xoay về mức 0%: LED phải tắt hoàn toàn (dòng điện đo được bằng đồng hồ vạn năng hoặc mô phỏng $0.00\text{ mA}$).
   - Khi vặn núm xoay lên mức 100%: LED đạt độ sáng cực đại ổn định (dòng điện qua LED xấp xỉ $5.9\text{ mA}$).
   - Xoay đều núm vặn: độ sáng LED tăng giảm liên tục, mượt mà, không bị giật cục hoặc chớp tắt giữa chừng.
