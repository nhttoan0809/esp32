# Trạng thái POC 4 — SoftAP Wi-Fi provisioning

Cập nhật: 2026-08-13.

## Đã hoàn thành

- Firmware AP+STA state machine, provisioning form/API, status/reset endpoint,
  upstream HTTP probe và NVS persistence đã triển khai.
- Credential cũ được giữ cho tới khi credential mới nhận IP và probe HTTP 200;
  response/status không trả password.
- Wokwi diagram, RFC2217 port 4004 và Private Gateway
  `localhost:8184 -> target:80` đã cấu hình.
- Build pass; Wokwi CLI 0.26.1 lint pass với một `info` về board part, không có
  warning/error.
- README đã phân biệt partial Wokwi simulation và runtime gate trên board thật.

## Đang dở dang / chưa xác minh

- Chưa quan sát runtime Serial, form submit, STA transition, upstream probe,
  NVS reboot hoặc reset trên firmware cuối.
- Wokwi không cho Wi-Fi adapter thật join RF SoftAP của ESP32 mô phỏng; flow
  Wokwi chỉ kiểm chứng một phần qua Private Gateway.
- Chưa có ESP32 thật để nghiệm thu end-to-end và failure/recovery cases.
- Wokwi CLI simulation cần `WOKWI_CLI_TOKEN`, hiện chưa được cung cấp.

## Bước tiếp theo

1. Chạy partial simulation bằng CLI:

   ```bash
   ./scripts/poc.sh 4 simulate --expect-text HTTP_SERVER_STARTED
   ```

2. Qua `localhost:8184`, submit `Wokwi-GUEST` password rỗng và quan sát status.
3. Trên ESP32 thật, join SoftAP từ client riêng; thử credential sai/đúng, reboot
   NVS, reset và xác nhận fallback giữ cấu hình cũ.

## Lệnh kiểm chứng

```bash
./scripts/poc.sh 4 build       # compile provisioning firmware
./scripts/poc.sh 4 artifacts   # xác nhận ELF/BIN
./scripts/poc.sh 4 lint        # lint diagram
./scripts/poc.sh 4 verify      # JSON + build + artifact + lint
./scripts/poc.sh 4 serial --lines 20  # đọc RFC2217 port 4004
```
