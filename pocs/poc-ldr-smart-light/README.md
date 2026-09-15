# POC: LDR Smart Light Controller (Đèn Thông Minh Với Module Quang Trở 3 Chân)

Bản Proof of Concept (POC) thực nghiệm ứng dụng **Module Quang trở LDR 3 chân (LM393)** để tự động điều khiển đèn chiếu sáng thông minh (Dusk-to-Dawn) với thuật toán **Bộ lọc trễ thời gian chống chập chờn (Time-based Hysteresis / Stability Filter)** và hiệu ứng **Soft Fade-In / Fade-Out** qua bộ điều chế xung **LEDC PWM** trên **ESP32 DevKit V1 (30 chân)**.

---

## 1. Thông Số Kỹ Thuật & Giải Thuật

- **Đặc tính Module LDR 3 Chân trong Kit:**
  - Ký hiệu in trên bo mạch PCB từ trái sang phải: **`DO` - `GND` - `VCC`**.
  - **Không có ngõ ra Analog `AO`**: Module tích hợp sẵn quang trở LDR, IC so sánh **LM393** và biến trở vi chỉnh màu xanh (Trimpot) để trực tiếp so sánh và xuất mức logic số TTL ($0\text{V} / 3.3\text{V}$) tại chân `DO`.
  - Nối vào **GPIO 35 (ADC1_CH7)**: Chân Input-Only của ESP32 (ngõ ra LM393 đã tích hợp sẵn trở kéo pull-up $10\text{k}\Omega$ trên bo mạch, hoàn toàn tương thích và an toàn).
  - Cực tính logic:
    - Trời tối (hoặc che tay): `DO = HIGH` (LED tín hiệu trên module tắt).
    - Trời sáng: `DO = LOW` (LED tín hiệu trên module sáng).
- **Thuật toán Bộ lọc ổn định thời gian (Time-based Stability Filter / 1500ms):**
  - Khắc phục triệt để hiện tượng đèn nhấp nháy liên tục khi ánh sáng chập chờn mấp mé ngưỡng (chạng vạng, bóng mây, hoặc bóng người/xe lướt qua nhanh).
  - Tín hiệu `DO` phải giữ nguyên trạng thái liên tục ít nhất **$1500\text{ ms}$** thì firmware mới chính thức xác nhận sự kiện chuyển trạng thái giữa **NGÀY (DAWN)** và **ĐÊM (DUSK)**.
- **Hiệu ứng Soft PWM Fading:**
  - Sử dụng ngoại vi **LEDC PWM** tần số $5\text{ kHz}$, độ phân giải 12-bit ($0 - 4095$) trên **GPIO 18**.
  - Khi trời tối: Đèn sáng dần mượt mà (**Fade In** $0\% \rightarrow 100\%$) trong $1.0\text{s}$.
  - Khi trời sáng: Đèn mờ dần và tắt hẳn (**Fade Out** $100\% \rightarrow 0\%$) trong $1.0\text{s}$.
  - Dòng danh định qua LED $\approx 5.91\text{ mA}$ (mắc qua trở $220\Omega$, an toàn tuyệt đối $< 12\text{ mA}$).

---

## 2. Bản Đồ Nối Dây (Pinout Map)

| Chân ESP32 | Ký hiệu in trên Module LDR | Chân mô phỏng Wokwi | Chức năng kỹ thuật | Lưu ý an toàn |
|---|:---:|:---:|---|---|
| **3V3** | `VCC` (Chân 3 - Phải) | `ldr1:VCC` | Nguồn cấp 3.3V cho IC LM393 | Không cắm nhầm sang VIN 5V |
| **GND** | `GND` (Chân 2 - Giữa) | `ldr1:GND` | Nối đất chung | |
| **D35 (GPIO 35)** | `DO` (Chân 1 - Trái) | `ldr1:DO` | Ngõ vào số nhận tín hiệu logic LM393 | Chân Input-Only, đã có pull-up trên module |
| **D18 (GPIO 18)** | *(Rời)* | `r1:1` | Ngõ ra xung PWM LEDC (5 kHz, 12-bit) | Nối vào 1 đầu điện trở $220\Omega$ |
| *(Nối tiếp)* | *(Rời)* | `r1:2` $\rightarrow$ `led1:A` | Chân Anode (+) của LED Vàng | Nối vào đầu còn lại của trở $220\Omega$ |
| **GND** | *(Rời)* | `led1:C` | Chân Cathode (-) của LED Vàng | Chân ngắn / vát phẳng của LED |

---

## 3. Hướng Dẫn Cân Chỉnh Biến Trở (Trimpot) & Test Thực Tế

1. **Cấp nguồn & Quan sát LED trên Module:**
   - Cắm cáp USB vào ESP32.
   - Module LDR có 2 đèn LED nhỏ:
     - **Power LED:** Luôn sáng đỏ khi có nguồn cấp.
     - **Signal LED:** Sáng hoặc tắt tùy thuộc vào cường độ ánh sáng chiếu vào quang trở.
2. **Cân chỉnh ngưỡng kích hoạt:**
   - Đặt mạch trong điều kiện ánh sáng phòng bình thường lúc muốn đèn TẮT.
   - Dùng tua-vít 2 cạnh nhỏ xoay biến trở màu xanh trên module:
     - Xoay từ từ cho đến khi Signal LED vừa chớm chuyển trạng thái (ngay ranh giới sáng/tắt).
3. **Thực nghiệm kiểm chứng:**
   - **Test Trời Tối (Dusk):** Dùng lòng bàn tay hoặc ngón tay che kín bề mặt quang trở $\rightarrow$ Signal LED đổi trạng thái $\rightarrow$ Sau đúng $1.5\text{s}$, Serial Monitor báo `DUSK CONFIRMED!` và đèn LED vàng sáng dần lên $100\%$.
   - **Test Trời Sáng (Dawn):** Bỏ tay che ra để ánh sáng phòng rọi vào $\rightarrow$ Sau đúng $1.5\text{s}$, Serial Monitor báo `DAWN CONFIRMED!` và đèn LED vàng từ từ tắt hẳn về $0\%$.
   - **Test Chống Chập Chờn:** Vẫy tay nhanh qua mặt cảm biến $(< 0.5\text{s})$ $\rightarrow$ Đèn không bị phản ứng giật cục hay chập chờn.

---

## 4. Lệnh Thao Tác Dòng Lệnh (CLI-First)

### 4.1 Kiểm tra cú pháp sơ đồ Wokwi
```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc-ldr-smart-light/diagram.json", "utf8"))'
wokwi-cli lint pocs/poc-ldr-smart-light
```

### 4.2 Biên dịch Firmware
```bash
pio run -d pocs/poc-ldr-smart-light -e esp32dev
test -f pocs/poc-ldr-smart-light/.pio/build/esp32dev/firmware.bin && echo "BIN OK"
```

### 4.3 Nạp lên Board thật & Theo dõi Serial
```bash
# Liệt kê cổng serial
pio device list

# Nạp firmware lên ESP32
pio run -d pocs/poc-ldr-smart-light -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# Mở Serial Monitor (115200 baud)
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```

