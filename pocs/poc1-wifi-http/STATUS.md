# Trạng thái POC 1 — Wi-Fi HTTP client

Cập nhật: 2026-08-13.

## Đã hoàn thành

- Firmware kết nối `Wokwi-GUEST` hoặc credential trong `secrets.h`.
- Wi-Fi timeout/reconnect và HTTP retry chạy theo `millis()`.
- HTTP GET có timeout và log status, content type, JSON body.
- Wokwi diagram, firmware paths và RFC2217 port 4001 đã cấu hình.
- Serial đã chuẩn hóa CRLF để Wokwi Terminal không hiển thị lệch bậc thang.
- PlatformIO build và Wokwi CLI 0.26.1 lint đã pass. Lint còn một `info` cho
  `board-esp32-devkit-c-v4`, không có warning/error.

## Đang dở dang / chưa xác minh

- Runtime trước bản sửa CRLF đã quan sát Wi-Fi connect và HTTP 200 thật; output
  cuối sau khi chuẩn hóa CRLF chưa được chạy lại bằng Wokwi CLI.
- Chưa chủ động gây mất Wi-Fi, DNS/TCP timeout và khôi phục để nghiệm thu toàn
  bộ retry state machine.
- Wokwi CLI simulation cần `WOKWI_CLI_TOKEN`; token không có trong môi trường
  và không được lưu trong repository.

## Bước tiếp theo

1. Export token cá nhân và chạy:

   ```bash
   ./scripts/poc.sh 1 simulate --expect-text HTTP_RESPONSE
   ```

2. Xác nhận mỗi marker nằm trên một dòng và JSON giữ đúng định dạng.
3. Chạy fault injection cho mất mạng/endpoint timeout, rồi phục hồi cấu hình
   cuối và chạy `./scripts/poc.sh 1 verify`.

## Lệnh kiểm chứng

```bash
./scripts/poc.sh 1 build       # compile firmware
./scripts/poc.sh 1 artifacts   # xác nhận ELF/BIN
./scripts/poc.sh 1 lint        # lint diagram bằng Wokwi CLI
./scripts/poc.sh 1 verify      # JSON + build + artifact + lint
./scripts/poc.sh 1 serial --lines 10  # đọc RFC2217 khi extension đang chạy
```
