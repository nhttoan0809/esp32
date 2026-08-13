# Trạng thái POC 3 — Web LED và Private Gateway

Cập nhật: 2026-08-13.

## Đã hoàn thành

- ESP32 phục vụ frontend và API idempotent từ flash, điều khiển LED GPIO2.
- `[[net.forward]]` triển khai Private Gateway `localhost:8180 -> target:80`.
- Đã quan sát runtime thật trong Wokwi VS Code 3.6.0: trạng thái
  `false -> true -> false`, LED sáng/tắt, JSON 404 và Serial request log.
- Serial dùng CRLF, không còn output lệch bậc thang trong Wokwi Terminal.
- Build pass; Wokwi CLI 0.26.1 lint pass với một `info` về board part, không có
  warning/error.
- Cơ chế bundled gateway, flow request và chẩn đoán đã được ghi trong
  `docs/PRIVATE-GATEWAY.md`.

## Đang dở dang / chưa xác minh

- Chưa chạy lại cùng test bằng Wokwi CLI vì môi trường chưa có
  `WOKWI_CLI_TOKEN`; runtime đã xác minh bằng extension.
- Restart nóng có thể cấp IP `.3` trong khi target forward cũ bị reset; tài liệu
  đã ghi cách reload window/start phiên sạch.
- Server vẫn là POC local, chưa có TLS, authentication hoặc authorization.

## Bước tiếp theo

1. Export token cá nhân và chạy CLI với marker:

   ```bash
   ./scripts/poc.sh 3 simulate --expect-text HTTP_SERVER_STARTED
   ```

2. Trong lúc CLI chạy, gọi API tại `127.0.0.1:8180` và xác nhận lại LED.
3. Nếu nâng thành sản phẩm, thiết kế security và không expose listener ra LAN.

## Lệnh kiểm chứng

```bash
./scripts/poc.sh 3 build       # compile firmware web server
./scripts/poc.sh 3 artifacts   # xác nhận ELF/BIN
./scripts/poc.sh 3 lint        # lint diagram
./scripts/poc.sh 3 verify      # JSON + build + artifact + lint
./scripts/poc.sh 3 serial --lines 10  # đọc RFC2217 port 4000
```
