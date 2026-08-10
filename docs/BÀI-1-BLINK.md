# Bài 1: Làm quen ESP32 — Nháy LED (Blink)

> Dành cho người biết lập trình web (frontend) nhưng mới chạm vào điện tử / nhúng.
> Mỗi bước đều có **Bạn đang làm gì?** và **Tại sao?** — nếu thấy thuật ngữ nào khó,
> hãy bảo mình, mình giải thích thêm.

---

## Trước hết: ESP32 là gì, hiểu nhanh qua góc nhìn web

Bạn đã biết: trình duyệt chỉ hiểu HTML/CSS/JS. Muốn chạy JS, bạn cần
**Node.js** (một "môi trường chạy"). Muốn chạy React app, bạn cần
**npm + package.json** để quản lý công cụ và thư viện.

ESP32 cũng y hệt như vậy, chỉ là môi trường chạy khác:

| Khái niệm web bạn đã biết | Khái niệm tương đương trong ESP32 | Giải thích nhanh |
|---|---|---|
| Node.js (môi trường chạy JS) | **ESP32 chip** (phần cứng) | Con chip này "chạy code" |
| npm / package.json | **PlatformIO + platformio.ini** | Công cụ quản lý dự án, thư viện, build |
| Trình duyệt render HTML/CSS/JS | **Framework Arduino** (bộ thư viện) | Dịch code C++ của bạn thành thứ chip hiểu |
| `npm run build` | **Build** trong PlatformIO | Biên dịch code → file để nạp vào chip |
| `npm run dev` + xem browser | **Upload** + nhìn LED trên board | Nạp file vào chip thật, chip chạy luôn |
| `console.log()` xem log | **Serial Monitor** | Xem chip "in ra" chữ gì |
| Màn hình hiển thị web | **LED tích hợp** trên board | Thứ để bạn *thấy* code chạy |

**Điểm khác biệt quan trọng nhất:** code web chạy trong trình duyệt trên máy bạn.
Code ESP32 **chạy trong con chip** — máy tính của bạn chỉ là nơi viết và build code,
rồi "gửi" sang chip qua cáp USB. Không cần mạng, không cần server; chip chạy độc lập.

---

## Tổng quan: mình sẽ làm gì trong bài này

Chương trình **Blink** là "Hello World" của thế giới nhúng:
bật LED lên 1 giây, tắt 1 giây, lặp lại mãi. Chỉ vài dòng code, không cần linh kiện
ngoài nào (dùng LED sẵn có trên board).

Hành trình gồm 5 bước:

| Bước | Tên | Kiểu giống web |
|---|---|---|
| 1 | Tạo thư mục dự án + file cấu hình | `npm init` + `package.json` |
| 2 | Viết code Blink | Viết `main.js` |
| 3 | Build (biên dịch) | `npm run build` |
| 4 | (Có board thì) Upload + xem LED | `npm run dev` + xem màn hình |
| 5 | (Có board thì) Serial Monitor | `console.log` |

Hiện bạn chưa có board nên chúng ta làm trọn vẹn bước 1–3 (code + build chạy thành
công). Bước 4–5 ghi chú sẵn để khi bạn mua board là làm được ngay.

---

## Bước 1 — Tạo dự án: file `platformio.ini`

### Bạn đang làm gì
Tạo một file văn bản tên `platformio.ini` — đây là **"package.json của dự án
ESP32"**: khai báo mình dùng board gì, framework gì, tốc độ gì.

### Cách làm
1. Trong VS Code: **File → Open Folder** → chọn thư mục `esp32-learning`
   (thư mục chứa dự án, nơi bạn đang có Claude Code chạy).
2. Trên khung Explorer (bên trái), bấm icon **New File**, đặt tên:
   ```
   platformio.ini
   ```
   *(lưu ý: để ở thư mục gốc, không nằm trong thư mục con nào)*
3. Dán nội dung sau vào file:
   ```ini
   [env:esp32dev]
   platform = espressif32
   board = esp32dev
   framework = arduino
   monitor_speed = 115200
   ```

### Tại sao
- **`[env:esp32dev]`** — giống tên môi trường trong `package.json`
  (`dev`/`prod`). Ở đây mình khai báo "môi trường build" tên `esp32dev`.
- **`platform = espressif32`** — giống `dependencies`: báo PlatformIO tải
  bộ công cụ của hãng Espressif (nhà sản xuất ESP32) về máy.
- **`board = esp32dev`** — chọn đúng model board. `esp32dev` là board ESP32
  DevKit phổ biến nhất, hay được bán kèm trong kit học tập. (Board của bạn
  in dòng chữ "ESP32-WROOM-32" — tên board khớp.)
- **`framework = arduino`** — chọn "bộ thư viện" để viết code. Arduino nổi
  tiếng dễ học, phù hợp người mới. (Có framework khác tên ESP-IDF mạnh hơn
  nhưng khó hơn — sau này mình quen rồi tìm hiểu cũng được.)
- **`monitor_speed = 115200`** — "tốc độ nói chuyện" giữa máy tính và chip
  qua cổng USB. Đơn giản chỉ cần để giống nhau ở 2 đầu; 115200 là chuẩn của
  ESP32, cứ để nguyên.

### Xong chưa?
Bạn thấy file `platformio.ini` xuất hiện trong Explorer là bước 1 xong.

---

## Bước 2 — Viết code: file `src/main.cpp`

### Bạn đang làm gì
Tạo file code chính của chương trình. Tên file có đuôi `.cpp` vì Arduino
dùng ngôn ngữ C++ (họ hàng của JavaScript nhưng kiểu dữ liệu rõ ràng hơn).

### Cách làm
1. Tạo thư mục `src` (trong Explorer: New Folder → `src`).
   **Tại sao:** PlatformIO quy ước code nằm trong thư mục `src` — như web
   quy ước code nằm trong `src/` vậy.
2. Tạo file `src/main.cpp` và dán:
   ```cpp
   // Chương trình đầu tiên: nháy LED trên board

   #include <Arduino.h>   // nạp các hàm có sẵn của framework Arduino

   void setup() {
     pinMode(2, OUTPUT);  // "cấu hình" chân LED thành ngõ ra
   }

   void loop() {
     digitalWrite(2, HIGH);  // bật LED
     delay(1000);                      // chờ 1 giây
     digitalWrite(2, LOW);   // tắt LED
     delay(1000);                      // chờ 1 giây
   }
   ```

### Tại sao — hiểu code theo kiểu web
Mọi chương trình Arduino đều có 2 hàm bắt buộc, giống như React có
`useEffect` vậy — framework tự gọi:

- **`setup()`** — chạy **một lần duy nhất** khi chip bật nguồn.
  *Giống:* code khởi tạo app khi trang load (đọc config, nối socket...).
- **`loop()`** — chạy **lặp đi lặp lại mãi mãi** sau khi `setup` xong.
  *Giống:* vòng lặp sự kiện (event loop) của JS — nhưng ở đây là chúng ta
  viết thẳng ra, chip tự lặp.

Đọc code theo thứ tự:

| Dòng code | Giải thích kiểu web |
|---|---|
| `#include <Arduino.h>` | Như `import React from 'react'` — mượn các hàm có sẵn |
| `pinMode(LED_BUILTIN, OUTPUT)` | Báo chip: chân này dùng để **xuất tín hiệu** (như đặt `<button onclick>` — "chân này sẽ được điều khiển") |
| `digitalWrite(..., HIGH)` | **Bật** điện ở chân đó → LED sáng (như `element.classList.add('on')`) |
| `digitalWrite(..., LOW)` | **Tắt** điện → LED tối (như `classList.remove('on')`) |
| `delay(1000)` | Dừng 1 giây (1000 mili-giây). *Khác với web:* `setTimeout` không chặn, `delay` **chặn** hẳn — chip không làm gì khác trong lúc chờ. Với chương trình này thì không sao, chỉ có một việc duy nhất. |
| `LED_BUILTIN` | Chữ viết sẵn của framework, tự biết LED trên board mình nằm ở chân nào. *Giống:* dùng biến môi trường `process.env.PORT` thay vì tự đặt cứng số cổng. |

> **Tại sao gọi là "digital"?** Điện ở chân này chỉ có 2 trạng thái: có
> (HIGH = 1) hoặc không (LOW = 0) — giống boolean `true/false`. Vậy nên gọi
> là *digital* (số). (Loại khác là *analog* — tín hiệu mượt như gradient —
> sau này mình sẽ gặp khi đọc cảm biến.)

### Xong chưa?
Thấy file `src/main.cpp` với nội dung trên trong Explorer là xong bước 2.

---

## Bước 3 — Build (biên dịch code)

### Bạn đang làm gì
Biên dịch code C++ thành file mà chip ESP32 hiểu được — như `npm run build`
băm JS thành bundle để trình duyệt chạy.

### Cách làm
1. Chờ PlatformIO nhận diện project: mở thư mục `esp32-learning` trong VS
   Code, khi nào thấy **thanh trạng thái màu xanh** ở cuối cửa sổ (giống
   thanh status của bạn khi mở workspace) hiện tên board `esp32dev` và các
   icon là PlatformIO đã sẵn sàng.
2. Bấm icon **✓ (Build)** trên thanh trạng thái đó.
3. Theo dõi terminal của VS Code (mục "Terminal" của PlatformIO).

### Kết quả bạn sẽ thấy
- Lần đầu: PlatformIO tự tải framework espressif32 (~300MB) — **vài phút**,
  đây là chuyện bình thường (như lần đầu `npm install`).
- Cuối cùng: dòng **`SUCCESS`** hiện ra, kèm đường dẫn file kết quả
  `.pio/build/esp32dev/firmware.bin`.
- Nếu có lỗi: dòng **`FAILED`** và mô tả lỗi — đừng cuống, mang lỗi đó
  cho mình xem, mình giúp sửa.

### Tại sao
- Build là "bước kiểm tra cổng": nếu code đúng, nó sẽ ra file firmware
  (giống bundle JS). Chỉ khi nào build `SUCCESS` thì mới đáng để upload.
- Tất cả file tạm nằm trong thư mục `.pio/` — **đừng sửa gì trong đó**.
  (Giống `node_modules`: chỉ là nơi chứa thứ công cụ tạo ra.)

---

## Bước 4 — Upload (khi bạn có board thật)

### Bạn đang làm gì
Nạp file `firmware.bin` đã build vào chip qua cáp USB — giống triển khai app.

### Cách làm
1. Cắm board vào máy bằng cáp USB (nên dùng cáp **data**, không phải cáp
   sạc).
2. Bấm icon **→ (Upload)** trên thanh trạng thái PlatformIO.
3. Trong vài giây, LED trên board bắt đầu **nháy: sáng 1 giây, tắt 1 giây**.
   Đây là khoảnh khắc "code chạy trên phần cứng thật"!

### Tại sao
- Máy tính gửi file qua cổng USB bằng **giao thức riêng của chip** (gọi là
  UART). Nó như "scp" nhưng cho firmware.
- Mỗi lần bật nguồn, chip chạy lại từ đầu: `setup()` 1 lần rồi `loop()` mãi.
  Chương trình này **chạy mãi mãi** đến khi cắt nguồn — giống app server
  luôn chạy, không có nút stop.

### Nếu bị lỗi
- **"Could not open port"** → macOS chưa nhận cổng. Hầu hết board giá rẻ
  dùng chip chuyển USB→serial tên CH340 hoặc CP210x; cần cài driver của
  hãng đó. Cắm board vào rồi cho mình xem dòng lỗi, mình chỉ cài đúng cái
  cần thiết.

---

## Bước 5 — Serial Monitor (khi có board)

### Bạn đang làm gì
Mở "cửa sổ console của chip": chip có thể in chữ ra cửa sổ này, như
`console.log` ở web.

### Cách làm
- Bấm icon **🔌 (Serial Monitor)** trên thanh trạng thái PlatformIO.
- Đảm bảo chọn đúng cổng board (góc phải, chọn tên có chứa "USB" / "CH340"
  / "CP210x").

### Tại sao
- Đây là công cụ debug quan trọng nhất của nhúng: muốn biết chip đang nghĩ
  gì thì in ra.
- Muốn thử ngay: thêm dòng `Serial.begin(115200);` vào đầu `setup()`, và
  `Serial.println("Hello ESP32!");` vào `loop()` — rồi build, upload lại,
  xem Serial Monitor. Đây chính là nội dung Bài 2 của chúng ta.

---

## Tóm tắt "đường đi" của code

```
Bạn viết main.cpp (C++)
   ↓ PlatformIO build   (như npm run build)
firmware.bin  ← file mà chip hiểu
   ↓ upload qua USB     (như deploy)
Chip ESP32 chạy loop() mãi mãi → LED nháy
```

## Nếu mình bị mắc ở đâu đó

Đừng ngại — mình chính là "người gỡ lỗi" của bạn:
- **Mỗi bước mình sẽ dừng và hỏi bạn thấy gì**, xong hết mới sang bước sau.
- Lỗi build, lỗi cổng, lỗi gì cũng được — **copy nguyên dòng lỗi gửi mình**.
- Thuật ngữ nào chưa rõ, bảo mình giải thích lại bằng ngôn ngữ web.

> Chúc mừng! Đây là bước đầu tiên của cả một hành trình — từ nháy một bóng
> LED đến những dự án IoT thật (cảm biến, WiFi, điều khiển qua web...).
> Bạn đã quen hệ sinh thái "code → build → nạp → xem kết quả" rồi đấy.
