# Build và kiểm thử các POC từ terminal

Workflow này giúp chạy bốn project PlatformIO độc lập mà không cần đổi folder
workspace hoặc chọn lại config trong VS Code.

## Công nghệ và phiên bản

| Công cụ | Phiên bản đã cài/khóa | Mục đích |
|---|---:|---|
| PlatformIO Core | 6.1.19 | resolve dependency, compile và tạo ELF/BIN |
| Espressif32 platform | 7.0.1 | board/toolchain package cho `esp32dev` |
| Arduino-ESP32 | 2.0.17 | framework của firmware |
| Wokwi CLI | 0.26.1 | lint diagram và chạy simulator từ terminal |
| Node.js | 22.16.0 đã kiểm tra | parse `diagram.json` trước build |
| Python + pyserial | pyserial 3.5 | đọc UART từ RFC2217 khi simulator VS Code chạy |

Wokwi CLI và Wokwi for VS Code đều đọc `wokwi.toml` và `diagram.json`, nhưng
license/token khác nhau. Extension dùng license đã kích hoạt trong VS Code;
CLI cần biến môi trường `WOKWI_CLI_TOKEN` cho cloud simulation.

## Cài đặt

```bash
./scripts/setup-tools.sh
```

Script thực hiện:

1. cài Wokwi CLI 0.26.1 bằng installer chính thức vào `$HOME/.wokwi` và tạo
   symlink `$HOME/bin/wokwi-cli`;
2. tạo `$HOME/bin/pio` và `$HOME/bin/platformio` trỏ tới CLI của PlatformIO
   IDE tại `$HOME/.platformio/penv/bin`;
3. tạo `.venv` trong repository và cài `pyserial==3.5`.

Mở terminal mới sau khi cài. Kiểm tra:

```bash
wokwi-cli --short-version
pio --version
.venv/bin/python -c 'import serial; print(serial.VERSION)'
```

Để chạy simulator bằng CLI, tạo token tại
<https://wokwi.com/dashboard/ci>, rồi chỉ export vào shell hiện tại:

```bash
export WOKWI_CLI_TOKEN='...'
```

Không commit token, không ghi token vào `wokwi.toml` hoặc file Markdown.

## Runner chung

Cú pháp:

```bash
./scripts/poc.sh <1|2|3|4> <action> [Wokwi CLI options]
```

| Lệnh | Mục đích |
|---|---|
| `./scripts/poc.sh 1 build` | compile POC1 đúng environment `esp32dev` |
| `./scripts/poc.sh 2 artifacts` | xác nhận ELF và BIN đã được tạo |
| `./scripts/poc.sh 3 lint` | lint diagram bằng registry hiện hành của Wokwi CLI |
| `./scripts/poc.sh 4 verify` | parse JSON, build, kiểm tra artifact và lint |
| `./scripts/poc.sh 1 simulate` | build rồi chạy Wokwi CLI, in Serial ra terminal |
| `./scripts/poc.sh 3 serial --lines 10` | đọc 10 dòng UART từ RFC2217 của extension |

Ví dụ chờ marker runtime và chạy lâu hơn mặc định:

```bash
WOKWI_TIMEOUT_MS=60000 \
  ./scripts/poc.sh 1 simulate --expect-text HTTP_RESPONSE
```

Wokwi CLI tự thoát sau timeout; mặc định CLI dùng exit code 42 khi timeout.
`--expect-text` làm phép kiểm tra có điều kiện thay vì chỉ quan sát log.

## Khác biệt theo POC

| POC | Build | Wokwi lint/simulate | Runtime đặc thù |
|---|---|---|---|
| 1 — Wi-Fi HTTP | có | có | cần Public Gateway để gọi API Internet |
| 2 — BLE | có | không hỗ trợ | cần ESP32 thật và BLE GATT client |
| 3 — Web LED | có | có | Private Gateway forward `8180 -> target:80` |
| 4 — Provisioning | có | có một phần | Private Gateway `8184 -> target:80`; SoftAP join/E2E cần board thật |

POC2 trả `N/A` khi lint vì Wokwi không mô phỏng Bluetooth. Đây không phải lỗi
cấu hình. `simulate` cho POC2 chủ động dừng với exit code 3.

## Kết quả lint hiện hành

Wokwi CLI 0.26.1 lint thành công POC1, POC3 và POC4 với một `info` chung:

```text
[unsupported-part] Part "esp" uses undocumented type
"board-esp32-devkit-c-v4".
```

Đây là `info`, không phải warning/error. Part identifier khớp diagram baseline,
tài liệu Wokwi ESP32 và schema của extension Wokwi 3.6.0 đã cài, nên được giữ
nguyên. Không đổi identifier chỉ để triệt tiêu thông báo registry của CLI.

## Nguồn chính thức

- [Cài Wokwi CLI](https://docs.wokwi.com/wokwi-ci/cli-installation)
- [Wokwi CLI usage, options và lint](https://docs.wokwi.com/wokwi-ci/cli-usage)
- [Wokwi CLI releases](https://github.com/wokwi/wokwi-cli/releases)
- [PlatformIO Core CLI](https://docs.platformio.org/en/latest/core/index.html)
- [Wokwi project config](https://docs.wokwi.com/vscode/project-config)
