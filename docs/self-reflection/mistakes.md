# Mistakes register (sai lầm tự gây ra)

Sai lầm do người thực hiện, không phải do công cụ. Ghi ngày vào tiêu đề; mục
con cố định: `desc`, `root-cause`, `pot-sol`, `selected-choice`.

---

## 1. Chạy script `poc.sh` của repo gốc cho POC5 (2026-08-23)

### desc

Chạy `./scripts/poc.sh 5 build` từ repo `esp32-learning` (gốc) để build POC5.
Script báo `Unknown POC: 5` hoặc cho kết quả không nhất quán (lúc "thành công"
lúc không), gây hiểu nhầm build sai trong khi thực chất build đúng code khác.

### root-cause

POC5 nằm trong repo **riêng** `esp32-learning-poc5-cloud-device`; script gốc
chỉ biết POC 1–4. Khi cwd là repo gốc, `./scripts/poc.sh` trỏ về script của
repo gốc (không có POC5) → sai lệnh.

### pot-sol

- (a) Luôn gọi script bằng **đường tuyệt đối** của repo POC tương ứng:
  `/Users/toannguyen/Documents/esp32-learning-poc5-cloud-device/scripts/poc.sh 5 build`.
- (b) Thêm POC5 vào script gốc (không nên — repo con là repo độc lập).
- (c) Luôn `cd` đúng repo POC trước khi chạy, kiểm tra bằng `pwd`.

### selected-choice

Chọn (a) + (c): dùng đường tuyệt đối của script repo con kèm kiểm tra cwd.
Đã áp dụng và build thành công (RAM 14.5%, Flash 73.2%).

---

## 2. Đọc sai kết quả `openssl verify` vì thiếu file untrusted (2026-08-23)

### desc

Khi kiểm tra TLS chain của ngrok, chạy `openssl verify` nhưng dùng file
chứng chỉ ghép **sai định dạng**, dẫn đến lỗi "Could not find untrusted
certificates" và suýt kết luận sai rằng CA nhúng trong firmware không hợp lệ.

### root-cause

File PEM ghép các chứng chỉ mà không có **đánh dấu phân tách**
(`-----BEGIN/END CERTIFICATE-----`) chuẩn → `openssl` đọc được 0 chứng chỉ
untrusted, báo lỗi hình thức chứ không phải lỗi thật.

### pot-sol

- (a) Dựng lại file untrusted bằng `cat` từng cert riêng (mỗi cert đã có
  BEGIN/END chuẩn) thay vì tự nối chuỗi.
- (b) Dùng `openssl verify -untrusted a.pem -untrusted b.pem` tách biệt.
- (c) Đối chiếu SHA-256 của CA nhúng với bản tải chính thức
  (letsencrypt.org) để loại trừ nghi ngờ CA sai.

### selected-choice

Chọn (a) + (c). Kết quả: CA nhúng trong `tls_ca.h` là **ISRG Root X1 thật**
(SHA-256 khớp bản tải chính thức), chain ngrok verify `OK`. TLS trust không
phải nguyên nhân lỗi WSS.

---

## 3. Nghi ngờ sai: CA nhúng không đúng như comment (2026-08-23)

### desc

Tự đưa ra báo động "CA nhúng trong `tls_ca.h` không phải ISRG Root X1 như
comment" — sai. Sau khi kiểm tra lại, CA là đúng.

### root-cause

So sánh SHA-256 của cert với một giá trị ghi nhớ sẵn, nhưng giá trị đó thực
chất là **SHA-1 fingerprint** của ISRG X1 (thông thường, mọi nơi đều dẫn
SHA-1), nên so không khớp → kết luận vội.

### pot-sol

- (a) Luôn tải lại CA gốc từ nguồn chính thức và so cả SHA-1 lẫn SHA-256
  trong cùng một bước.
- (b) Dùng `openssl x509 -noout -fingerprint -sha1/-sha256` thay vì giá trị
  nhớ.

### selected-choice

Chọn (b). Xác nhận CA nhúng hợp lệ; rút bài học: không so hash với giá trị
nhớ mà phải so với nguồn tải lại.

---

## 4. Để `WOKWI_PRECONFIG_SERVER_HOST` chứa URL đầy đủ thay vì host (2026-08-23)

### desc

Đã đặt `WOKWI_PRECONFIG_SERVER_HOST = "https://sombrous-...ngrok-free.dev"`
(full URL) vào `secrets.h`. Firmware `beginSslWithCA` chỉ nhận **host-only**, nên DNS sẽ
thử giải `"https://..."` → WSS không bao giờ bắt đầu.

### root-cause

Sai kỳ vọng về tham số `serverHost` của `beginSslWithCA` (host-only, không
scheme, không path). Việc này bắt nguồn từ cách hiểu nhầm khi fill placeholder
mà tôi đề xuất ban đầu ("placeholder ngrok host").

### pot-sol

- (a) Chỉnh trị về host-only `sombrous-homomorphous-zavier.ngrok-free.dev`.
- (b) Thêm validation ở firmware: nếu `serverHost` chứa `://` thì log lỗi rõ.

### selected-choice

Chọn (a) (đã thực hiện, rebuild) + (b) là cải tiến tương lai (chưa làm,
ghi nhận ở đây để không quên).

---

## 5. Giả định "Wokwi không mô phỏng TLS" không kiểm chứng (2026-08-23)

### desc

Đưa ra giả thuyết "Wokwi không mô phỏng TLS phía ESP32 nên
`beginSslWithCA` fail" trước khi có bằng chứng. Giả thuyết này **sai/hết
lực**: nhiều dự án Wokwi công khai chạy `WiFiClientSecure` (HTTPS, MQTT TLS
port 8883) qua `Wokwi-GUEST` tài khoản thường.

### root-cause

Dựa vào suy luận chung (Wokwi là simulator, "chắc TLS không được") thay vì
tra cứu thực tế trước khi khẳng định. Thiếu bước kiểm tra bằng chứng (web
search / dự án mẫu) trước khi gọi tên nguyên nhân.

### pot-sol

- (a) Luôn tra cứu bằng chứng (Tavily/web) trước khi tuyên bố một giới hạn
  công cụ.
- (b) Dựng **probe nhỏ** (TCP probe, TLS probe như đã làm) để phân tầng
  lỗi thay vì đoán.

### selected-choice

Chọn (a) + (b). Đã bổ sung `TCP_PROBE_*` và `TLS_PROBE_*` trong
`cloud_client.cpp`, tách được tầng TCP (OK) vs TLS (fail) một cách có bằng
chứng.
