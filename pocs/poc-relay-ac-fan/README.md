# POC: Relay AC Fan Controller

Điều khiển quạt máy AC 220V thông qua **Relay Module 2 kênh 5V (Optocoupler)** bằng ESP32 DevKit V1.

---

## Phần cứng Yêu cầu

| Linh kiện | Số lượng | Ghi chú |
|---|:---:|---|
| ESP32 DevKit V1 (30-pin) | 1 | Vi điều khiển trung tâm |
| Relay Module 2 kênh 5V (SONGLE SRD-05VDC-SL-C) | 1 | Optocoupler cách ly quang |
| Nút bấm 12×12mm | 1 | Toggle relay |
| LED Xanh lá | 1 | Hiển thị trạng thái relay |
| Điện trở 220Ω | 1 | Hạn dòng LED |
| Breadboard MB102 | 1 | Ráp mạch DC side |
| Cáp Jumper | ~10 | Kết nối |
| Dây điện 2 lõi (L + N) | ~1m | Nối ổ điện đầu vào |
| Phích cắm đôi (ổ điện cái) | 1 | Đầu ra cho quạt |
| Băng keo điện / ống gen nhiệt | Đủ dùng | Cách điện dây AC |

---

## Sơ Đồ Kết Nối

### DC Side (Low-Voltage — An toàn)

```
ESP32 DevKit V1
│
├── GPIO 26 ──────────────────────────► IN1 relay module
│   (Active LOW: LOW=ON, HIGH=OFF)
│
├── GPIO 27 ──[220Ω]──[LED Xanh]──GND   ← LED trạng thái
│   (HIGH = relay đang ON = LED sáng)
│
├── GPIO 14 ──[Button]──────────── GND   ← Nút toggle
│   (INPUT_PULLUP, nhấn = LOW)
│
├── VIN (5V) ──────────────────────────► VCC relay module
│                                    └──► JD-VCC relay module  (tháo jumper JD-VCC!)
└── GND ───────────────────────────────► GND relay module
```

> **Lưu ý JD-VCC jumper:** Tháo jumper trên board relay để cấp VCC và JD-VCC riêng biệt → cách ly quang hoàn toàn 100%.

### AC Side (High-Voltage — ⚠️ Làm khi ĐÃ NGẮT ĐIỆN)

```
[Ổ điện tường 220V]
│
├── L (Pha) ──────────────────► COM (relay kênh 1)
│                                     ↕ tiếp điểm NO
│                               NO ──────────────────► L' ──► [Quạt] ──► N'
└── N (Trung tính) ──────────────────────────────────────────────────► N'
```

**Nguyên lý:** Relay hoạt động như công tắc trên dây Pha (L). Khi relay ON → COM–NO thông mạch → quạt có điện.  
**Fail-Safe:** Relay OFF mặc định (NO hở mạch) → mất điều khiển = quạt tắt.

---

## GPIO Map

| GPIO | Chức năng | Loại | Ghi chú |
|---|---|---|---|
| **26** | Relay IN1 output | OUTPUT | Active LOW. `LOW` = relay ON |
| **27** | LED trạng thái | OUTPUT | `HIGH` = LED sáng = relay ON |
| **14** | Button toggle | INPUT_PULLUP | Nhấn = `LOW` → trigger toggle |

---

## Logic Điều Khiển

| Hành động | Kết quả |
|---|---|
| Nhấn button (< 3s) | Toggle relay (OFF→ON hoặc ON→OFF) |
| Giữ button ≥ 3 giây | **Force OFF** — Tắt relay bắt buộc (safety override) |
| Mất nguồn USB ESP32 | Relay mất điều khiển → tự OFF (Fail-Safe) |

**Relay logic (Active LOW):**
```
GPIO 26 = LOW  → Relay ON  → NO đóng → Quạt BẬT
GPIO 26 = HIGH → Relay OFF → NO hở   → Quạt TẮT  ← Trạng thái khởi động
```

---

## Quy Trình Kiểm Chứng (Theo Từng Giai Đoạn)

### 🟡 Giai đoạn 1 — Wokwi Simulation

```bash
# 1. Validate JSON syntax
node -e 'JSON.parse(require("fs").readFileSync("diagram.json", "utf8"))' && echo "JSON OK"

# 2. Build firmware
pio run -d pocs/poc-relay-ac-fan -e esp32dev

# 3. Verify artifacts
test -f pocs/poc-relay-ac-fan/.pio/build/esp32dev/firmware.bin && echo "BIN OK"
test -f pocs/poc-relay-ac-fan/.pio/build/esp32dev/firmware.elf && echo "ELF OK"

# 4. Wokwi lint
wokwi-cli lint pocs/poc-relay-ac-fan

# 5. Wokwi simulation test (nhấn phím 'r' để simulate button)
wokwi-cli --expect-text "[RELAY] ON" --timeout 12000 pocs/poc-relay-ac-fan
```

**Pass condition:** Serial in `[RELAY] ON` sau khi nhấn phím `r`.

---

### 🟠 Giai đoạn 2 — Board Thật, DC Only (Chưa có AC)

```bash
# Flash firmware
pio run -d pocs/poc-relay-ac-fan -e esp32dev -t upload --upload-port /dev/cu.usbserial-XXXX

# Theo dõi Serial
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
```

**Checklist kiểm tra:**
- [ ] LED indicator nhỏ trên relay module bật/tắt khi nhấn button
- [ ] Nghe tiếng "tách" relay đóng/mở
- [ ] LED xanh trên breadboard (GPIO 27) sáng khi relay ON
- [ ] Serial in `[RELAY] ON` / `[RELAY] OFF` đúng
- [ ] Nhấn giữ 3s → Serial in `[SAFETY] Force OFF triggered`

---

### 🔴 Giai đoạn 3 — Bóng Đèn 220V (Thử trước khi dùng quạt)

> [!CAUTION]
> **CHECKLIST AN TOÀN BẮT BUỘC — Hoàn thành 100% trước khi cắm điện:**
>
> - [ ] Đã pass hoàn toàn Giai đoạn 2
> - [ ] Dùng bóng đèn ≤ 60W để thử (không dùng quạt ngay)
> - [ ] Tất cả đầu nối AC được cách điện (băng keo điện + ống gen nhiệt)
> - [ ] **NGẮT PHÍCH ĐIỆN** trước khi nối/tháo bất kỳ dây nào
> - [ ] Không để dây trần tiếp xúc breadboard hoặc ESP32
> - [ ] Kiểm tra kỹ lần 2 trước khi cắm điện lần đầu
> - [ ] Không để tay gần vùng AC khi đang có điện

**Kịch bản kiểm tra:**
1. Cắm phích vào ổ điện → Đèn **TẮT** (relay OFF mặc định) ✅
2. Nhấn button → Đèn **BẬT** + LED xanh sáng ✅
3. Nhấn button lần 2 → Đèn **TẮT** + LED xanh tắt ✅
4. Rút USB ESP32 → Đèn **TẮT** (Fail-Safe) ✅

---

### 🟢 Giai đoạn 4 — Quạt Máy Thật

**Điều kiện tiên quyết:** Đã pass Giai đoạn 3 hoàn toàn.

**Lưu ý kỹ thuật quạt điện (tải cảm kháng):**
- Quạt máy là **Inductive Load** — có dòng khởi động cao (~3-6× dòng danh định)
- Relay SONGLE chịu được 10A/250VAC → đủ cho mọi quạt điện gia dụng (40W–120W ≈ 0.18–0.55A)
- Sau khi chạy thử: kiểm tra nhiệt độ relay sau 5 phút — relay không được nóng

---

## Cấu Trúc Dự Án

```
poc-relay-ac-fan/
├── platformio.ini       — Build config
├── wokwi.toml           — Wokwi runner
├── diagram.json         — Wokwi simulation diagram
├── src/
│   └── main.cpp         — Firmware ESP32
└── README.md            — Tài liệu này
```

---

## Wokwi Simulation — Giới Hạn Cần Biết

Wokwi **không có component relay** trong catalog và **không thể mô phỏng AC 220V**. Trong simulation:
- **LED đỏ** (GPIO 26) = đại diện cho tín hiệu IN1 vào relay (sáng = relay đang được kích)
  > Lưu ý: LED đỏ sáng khi HIGH, nhưng relay ON khi LOW (Active LOW) → trong sim, LED đỏ sẽ **tắt** khi relay ON
- **LED xanh** (GPIO 27) = trạng thái relay (sáng = relay ON)
- Nhấn phím `r` trên bàn phím để simulate button nhấn
