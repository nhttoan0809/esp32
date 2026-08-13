# Trạng thái POC 2 — BLE GATT server

Cập nhật: 2026-08-13.

## Đã hoàn thành

- Firmware quảng bá tên `ESP32-POC2-BLE`, service UUID cố định và
  characteristic READ trả `ready`.
- Callback ghi connect/disconnect và tự quảng bá lại sau 500 ms.
- Build `esp32dev` pass, không có compiler warning; ELF/BIN đã tạo.
- UUID chỉ là identifier, không phải credential. POC chưa bật pairing, bonding,
  passkey hoặc encryption.

## Đang dở dang / chưa xác minh

- BLE runtime là `N/A — unsupported` trên Wokwi; POC cố ý không có
  `diagram.json`/`wokwi.toml`.
- Máy kiểm chứng chưa có ESP32 qua USB, nên chưa quan sát scan, connect, đọc
  `ready`, disconnect, advertising restart và reconnect trên radio thật.
- Bluetooth Incoming Port của macOS không phải BLE GATT và không thay thế được
  phép kiểm tra bằng BLE client.

## Bước tiếp theo

1. Cắm ESP32 classic thật, upload firmware và mở monitor 115200.
2. Dùng BLE scanner hoặc Chrome Web Bluetooth trên máy tính làm GATT client.
3. Scan/connect/read/disconnect/reconnect và lưu Serial evidence.
4. Chỉ bổ sung passkey/bonding nếu yêu cầu sản phẩm cần authentication.

## Lệnh kiểm chứng

```bash
./scripts/poc.sh 2 build       # compile firmware BLE
./scripts/poc.sh 2 artifacts   # xác nhận ELF/BIN
./scripts/poc.sh 2 lint        # trả N/A có chủ đích vì không có diagram
./scripts/poc.sh 2 verify      # build + artifact, lint N/A
pio device list               # tìm cổng ESP32 thật
```
