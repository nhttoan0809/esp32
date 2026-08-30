# Trạng thái POC 5 — Cloud WebSocket device

Cập nhật: 2026-08-27.

## Offline desired-state + auto-sync (2026-08-27)

- Thay đổi hợp đồng server (phía device/firmware **không đổi**):
  - `PUT /state` khi device offline: không còn 503 `device_offline`; trả 200
    với `synced: false`, `command_id: null`,
    `warning: "device_offline_queued"`. Trạng thái mong muốn được lưu
    (mới nhất ghi đè — chỉ giữ 1 giá trị, không phải queue nhiều lệnh).
  - `GET /devices/{id}` thêm trường `pending_on` (null khi không có gì
    pending).
  - Khi device (tái) kết nối và `hello.reported.on` khác `pending_on`,
    server tự đẩy `set_state` (fire-and-forget; ACK do read-loop nhận và
    xóa queue). Nếu `hello.reported.on` đã khớp → không đẩy lại, chỉ xóa
    queue. ACK của lệnh live bị timeout/mất cũng giữ `pending_on` để tự
    hồi ở lần reconnect kế tiếp.
  - Dashboard hiển thị dòng cảnh báo "chưa đồng bộ / queued".
- 12 server tests pass (10 test hợp đồng cũ + 2 test mới: queued-while-offline
  và auto-reconcile-on-reconnect, trong đó có nhánh no-op khi device đã ở
  đúng trạng thái).
- Cần restart uvicorn để áp dụng (ngrok không cần chạy lại).
- Nghiệm thu thật (board thật + LED GPIO23) vẫn chờ gate cũ ở mục bên dưới;
  test này xác minh bằng fake device qua ngrok + TestClient.

## Đã triển khai và xác minh

- Worktree `esp32-learning-poc5-cloud-device`, branch
  `codex/poc5-cloud-device`.
- Firmware provisioning, Wi-Fi retry, NTP, TLS/WSS, `hello/ready`, heartbeat,
  GPIO command/ACK, NVS pending commit/fallback và setup/reset button.
- FastAPI health, Swagger REST, dashboard, authenticated device WebSocket và
  direct-command registry; không có `desired_state` hoặc offline queue.
- 10 server tests pass, gồm ACK đúng, offline, disconnect đang chờ ACK, timeout,
  token sai, connection replacement và ACK không khớp.
- Uvicorn local đã được chạy thực tế: health/OpenAPI trả 200; fake device nhận
  `set_state`, ACK, REST trả 200 và snapshot cập nhật `on=true`.
- PlatformIO build pass; ELF/BIN tồn tại. Wokwi CLI 0.26.1 lint không có
  error/warning, còn một `info` về board part xem bên dưới.

## Xác minh phía server/ngrok (2026-08-23)

- Chạy thật WSS end-to-end **từ máy host** qua ngrok
  (`sombrous-homomorphous-zavier.ngrok-free.dev`): nhận `ready`, REST
  `GET /api/devices/esp32-poc5` trả `online=true` khi socket mở. Phía
  FastAPI + ngrok + token + TLS chain (openssl verify với ISRG Root X1)
  đều đúng.
- Lỗi 401 `invalid_dashboard_api_key` trên Swagger đã xử lý: shell không tự
  nạp `.env`, nên uvicorn trước đó chạy với key placeholder; chạy lại đúng
  key thì API hoạt động.

## Wokwi: firmware preconfig và chẩn đoán WSS (2026-08-23)

- Wokwi free không có Private IoT Gateway, nên SoftAP portal không điều khiển
  được từ trình duyệt. Đã thêm **đường preconfig**: khi NVS trống và
  `WOKWI_PRECONFIG_ENABLED = true`, firmware bỏ qua portal, nối STA
  `Wokwi-GUEST` (pass rỗng) + host ngrok trong `secrets.h`. Cố tình **không
  commit NVS** (Wokwi reset NVS mỗi lần chạy). Flash board thật phải đặt
  flag về `false`.
- Firmware thêm 3 lớp chẩn đoán trong `cloud_client.cpp`:
  - `TCP_PROBE_OK` / `TCP_PROBE_FAILED code=N` — probe TCP thường trước TLS;
  - `TLS_PROBE_OK elapsed_ms` / `TLS_PROBE_FAILED code elapsed_ms` — bắt tay
    TLS thật (cùng CA nhúng, ngân sách `TLS_PROBE_TIMEOUT_MS = 20000`), đo
    thời gian;
  - log lý do đóng của thư viện WebSockets (`WSS_DISCONNECTED reason=...`,
    `WSS_ERROR reason=...`).

### Kết quả chạy Wokwi CLI headless (token CI, `--serial-log-file`)

```
WIFI_CONNECTED ip=10.13.37.2
TIME_SYNCED epoch=...
TCP_PROBE_OK host=...ngrok-free.dev port=443
TLS_PROBE_FAILED code=0 elapsed_ms=34148
WSS_CONNECTING ... → WSS_DISCONNECTED reason="TCP connection cleanup"
```

Kết luận kỹ thuật:

- TCP thường qua Wokwi Public Gateway tới ngrok `:443` **được** (DNS +
  routing + gateway đều sống).
- Bắt tay TLS **không hoàn tất** dù ngân sách 20s (gấp ~4 lần 5s — thời
  hạn `WEBSOCKETS_TCP_TIMEOUT` của thư viện WebSockets). Loại giả thuyết
  "TLS chậm hơn 5s"; đây là giới hạn của TLS outbound qua gateway Wokwi
  (khớp bug Wokwi #721 "HTTPS / TLS requests not completing").
- Watcher `lsof :8000` trong cả 2 lần chạy headless: **không** peer nào
  chạm tới server — WSS chưa từng đi đến được FastAPI.

### Quyết định nghiệm thu (2026-08-23)

Không nâng Wokwi (Private Gateway trả phí) và chưa có board ESP32 thật, nên
**bước WSS-TLS không được xác nhận trong Wokwi free**; đèn server (GPIO22)
chưa sáng được trong simulator vì đúng tầng TLS đó. POC chưa được tuyên bố
nghiệm thu end-to-end. Các đường thay thế khi cần:

1. Flash board thật + Wi-Fi thật → WSS/ngrok hoạt động không qua gateway
   Wokwi (khuyến nghị để nghiệm thu).
2. Wokwi Private Gateway (bản trả phí) → TLS full trong simulator.
3. Chế độ WS thường (không TLS) cho Wokwi-only — làm thay đổi bản chất POC
   (mất TLS), chỉ dùng nếu chấp nhận.

## Chưa thể xác minh trong môi trường hiện tại

- Chưa quan sát `WSS_AUTHENTICATED`, LED GPIO22 sáng, dashboard → ESP32 →
  GPIO19/23 trong simulator.
- Chưa có board ESP32 thật để join SoftAP, quan sát GPIO, power-cycle/NVS,
  Wi-Fi loss, reset button và recovery.

## Wokwi lint info còn lại

CLI báo `unsupported-part` mức **info** cho
`board-esp32-devkit-c-v4`. Identifier này vẫn được giữ vì tài liệu diagram
Wokwi chính thức và schema của Wokwi for VS Code 3.6.0 chấp nhận đúng tên đó;
không có warning hoặc error.

## Lệnh kiểm chứng đã chạy

```bash
node -e 'JSON.parse(require("fs").readFileSync("diagram.json", "utf8"))'
pio run -e esp32dev
test -f .pio/build/esp32dev/firmware.elf
test -f .pio/build/esp32dev/firmware.bin
wokwi-cli lint
cd server && .venv/bin/python -m pytest -q
# Wokwi headless (token CI):
wokwi-cli --timeout 60000 --serial-log-file poc5-serial.log .
```

Kết quả final verify: RAM 47,548/327,680 byte (14.5%), flash
959,965/1,310,720 byte (73.2%); server test `10 passed`.

## Gate tiếp theo

1. Flash board thật, provision SoftAP bằng điện thoại, chạy dashboard qua
   ngrok và hoàn tất test matrix trong README (đèn GPIO22 sáng khi
   `WSS_AUTHENTICATED`).
2. Hoặc nâng Wokwi Private Gateway nếu bắt buộc phải chứng minh trong
   simulator.
