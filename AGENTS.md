# AGENTS.md

## Phạm vi

Các chỉ dẫn này áp dụng cho toàn bộ repository. Mục tiêu là giữ cấu hình
PlatformIO + ESP32 + Wokwi có thể build và mô phỏng lặp lại được, đồng thời
không suy đoán tên board, part, chân kết nối, đường dẫn firmware hoặc vị trí
Serial Monitor.

## Nguyên tắc bắt buộc

1. Trước khi sửa cấu hình board, framework hoặc extension, phải xác định phiên
   bản đang dùng và đọc tài liệu chính thức tương ứng trong cùng lượt làm việc.
2. Chỉ dùng nguồn sơ cấp theo thứ tự ưu tiên:
   - tài liệu của nhà sản xuất chip/board;
   - tài liệu chính thức của framework;
   - tài liệu chính thức của PlatformIO/Wokwi;
   - Marketplace, changelog và schema đi kèm đúng phiên bản extension đã cài.
3. Không lấy blog, video, gist, Stack Overflow hoặc snippet do AI tạo làm nguồn
   quyết định cấu hình. Chỉ dùng chúng để tìm từ khóa, sau đó xác minh lại bằng
   nguồn chính thức.
4. Không tự đoán identifier. Ví dụ, Wokwi dùng `wokwi-led`, không phải `led`.
5. Không báo thành công chỉ vì code build được. Phải quan sát được output hoặc
   trạng thái phần cứng mô phỏng theo mục "Cổng kiểm chứng" bên dưới.
6. Khi tài liệu web và hành vi extension khác nhau, ưu tiên hành vi/schema/
   changelog của đúng phiên bản extension đang cài, đồng thời ghi rõ khác biệt.

## Khảo sát trước khi thay đổi

Đọc tối thiểu các file sau nếu chúng tồn tại:

```text
platformio.ini
wokwi.toml
diagram.json
src/main.cpp
docs/WOKWI-SETUP.md
```

Xác định cấu hình thực tế bằng các lệnh đọc-only phù hợp:

```bash
code --list-extensions --show-versions | rg -i 'wokwi|platformio'
pio --version
pio run --list-targets
pio pkg list
```

Nếu `pio` không có trong `PATH`, kiểm tra CLI do PlatformIO IDE cài tại
`${HOME}/.platformio/penv/bin/pio`.

Với Wokwi, kiểm tra đúng package đang cài thay vì dựa vào ảnh chụp của phiên bản
cũ. Các file hữu ích thường gồm:

```text
package.json
changelog.md
schemas/diagram.schema.json
```

Tìm package trong thư mục extension của editor đang dùng, ví dụ VS Code,
VS Code Insiders hoặc Cursor. Không hard-code phiên bản extension vào script.

## Chọn tài liệu theo lớp cấu hình

### 1. PlatformIO và board target

`platformio.ini` là nguồn sự thật cho environment, platform, board và framework.

- PlatformIO Espressif32:
  <https://docs.platformio.org/en/latest/platforms/espressif32.html>
- ESP32 Dev Module (`board = esp32dev`):
  <https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html>
- Cấu hình PlatformIO project:
  <https://docs.platformio.org/en/latest/projectconf/index.html>
- PlatformIO IDE for VS Code và Serial Monitor:
  <https://docs.platformio.org/en/latest/integration/ide/vscode.html>
- `pio device monitor`:
  <https://docs.platformio.org/en/stable/core/userguide/device/cmd_monitor.html>

Khi đổi `board`, phải mở trang board chính thức tương ứng trong PlatformIO
Boards catalog. Không suy ra chip, flash size, upload protocol hoặc pin map chỉ
từ tên thương mại in trên board.

Tên environment trong `[env:NAME]` phải khớp với thư mục build mà Wokwi đọc:

```text
.pio/build/NAME/firmware.elf
.pio/build/NAME/firmware.bin
```

Nếu đổi tên environment hoặc board, phải cập nhật `wokwi.toml` trong cùng thay
đổi và build lại để xác nhận artifact thực sự tồn tại.

### 2. Framework và chip

Chọn tài liệu theo giá trị `framework` trong `platformio.ini`:

| Framework | Nguồn chính thức bắt buộc |
|---|---|
| Arduino trên ESP32 | <https://docs.espressif.com/projects/arduino-esp32/en/latest/getting_started.html> |
| Arduino GPIO | <https://docs.espressif.com/projects/arduino-esp32/en/latest/api/gpio.html> |
| Arduino Serial/UART | <https://docs.espressif.com/projects/arduino-esp32/en/latest/api/serial.html> |
| ESP-IDF | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/index.html> |
| ESP-IDF trong PlatformIO | <https://docs.platformio.org/en/latest/frameworks/espidf.html> |
| MicroPython trên Wokwi VS Code | <https://docs.wokwi.com/vscode/vscode-micropython> |

Với chip khác ESP32 classic, phải chọn đúng biến thể tài liệu (ESP32-C3, S2,
S3, C6...). Không mặc định rằng UART, USB CDC, LED tích hợp hoặc GPIO2 hoạt
động giống ESP32 classic.

Đặc biệt với ESP32-C3/S3 và các chip có USB Serial/JTAG, đọc phần USB CDC trong:

<https://docs.wokwi.com/guides/esp32>

Không vừa cấu hình `serialInterface = "USB_SERIAL_JTAG"` vừa giữ các dây
`$serialMonitor` nếu tài liệu của chip yêu cầu bỏ chúng.

Nếu repository đổi nền tảng, định tuyến nguồn theo nhà sản xuất thay vì tái sử
dụng tài liệu ESP32:

| Board/chip | Nguồn phần cứng và SDK ưu tiên |
|---|---|
| ESP32 family | Espressif Docs: <https://docs.espressif.com/> |
| Arduino AVR/SAMD/Renesas | Arduino Hardware và Language Reference: <https://docs.arduino.cc/> |
| RP2040/Raspberry Pi Pico | Raspberry Pi Documentation: <https://www.raspberrypi.com/documentation/microcontrollers/> |
| STM32 | STMicroelectronics STM32 portal: <https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html> và datasheet/reference manual của đúng MCU |

Nếu framework đổi sang Zephyr, dùng <https://docs.zephyrproject.org/latest/>;
nếu dùng Raspberry Pi Pico SDK, dùng tài liệu SDK tại Raspberry Pi. Sau đó mới
dùng trang integration tương ứng của PlatformIO hoặc Wokwi để nối build system
với simulator.

### 3. Wokwi for VS Code

- Bắt đầu với Wokwi for VS Code:
  <https://docs.wokwi.com/vscode/getting-started>
- Cấu hình `wokwi.toml` và RFC2217:
  <https://docs.wokwi.com/vscode/project-config>
- Di chuyển project vào VS Code:
  <https://docs.wokwi.com/vscode/migrating>
- Trang Marketplace chính thức:
  <https://marketplace.visualstudio.com/items?itemName=Wokwi.wokwi-vscode>

Phải build firmware trước khi start simulator. Sau khi sửa code, build lại và
xác nhận timestamp/hash của artifact thay đổi nếu thay đổi đó phải ảnh hưởng
firmware.

Vị trí Serial UI phụ thuộc phiên bản extension. Wokwi for VS Code 3.6 dùng
terminal tích hợp của VS Code (thường có tên `Wokwi Terminal`) thay vì panel
Serial nằm dưới sơ đồ. Khi làm việc với phiên bản khác, đọc changelog của chính
package đã cài trước khi hướng dẫn người dùng tìm UI.

Định tuyến tài liệu theo extension đang thực sự thực hiện tác vụ:

| Extension | Nguồn ưu tiên |
|---|---|
| Wokwi Simulator | Wokwi Docs, trang Marketplace chính thức, changelog/schema của package đã cài |
| PlatformIO IDE | <https://docs.platformio.org/en/latest/integration/ide/vscode.html> và PlatformIO Core docs |
| ESP-IDF for VS Code | <https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/> |
| Arduino IDE/CLI integration | <https://docs.arduino.cc/arduino-cli/> và trang Marketplace do Arduino phát hành |

Không dùng tài liệu của một extension để suy ra hành vi UI hoặc cổng serial của
extension khác. PlatformIO Monitor, Wokwi Terminal và ESP-IDF Monitor là ba
consumer serial khác nhau dù cùng hiển thị trong panel Terminal của VS Code.

### 4. `diagram.json` và linh kiện

- Định dạng sơ đồ và danh sách MCU:
  <https://docs.wokwi.com/diagram-format>
- Phần cứng được hỗ trợ:
  <https://docs.wokwi.com/getting-started/supported-hardware>
- LED part chính thức:
  <https://docs.wokwi.com/parts/wokwi-led>
- Serial Monitor và các pin ảo:
  <https://docs.wokwi.com/guides/serial-monitor>
- Wokwi CLI và `lint`:
  <https://docs.wokwi.com/wokwi-ci/cli-usage>

Mọi `type` trong `parts` phải lấy nguyên văn từ Wokwi Docs, Wokwi Elements hoặc
schema của extension. Không rút gọn prefix `wokwi-`.

Mọi chân trong `connections` phải được đối chiếu với cả part reference và code.
TX nối sang RX, RX nối sang TX. Không mặc định một board có LED tích hợp; nếu
code điều khiển LED rời thì `diagram.json` phải có LED rời và dây tương ứng.

`Serial.println()` gửi dữ liệu qua Serial/UART. Nó không làm chữ xuất hiện trên
OLED/LCD. Muốn có màn hình vật lý trong sơ đồ phải thêm đúng display part, wiring,
library và code render theo tài liệu của display đó.

## Baseline đã xác minh của repository này

Target hiện tại:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
```

Wokwi đọc firmware từ:

```toml
[wokwi]
version = 1
elf = ".pio/build/esp32dev/firmware.elf"
firmware = ".pio/build/esp32dev/firmware.bin"
rfc2217ServerPort = 4000
```

Sơ đồ dùng ESP32 DevKitC V4, LED part `wokwi-led`, GPIO2 và UART0. Các kết nối
Serial bắt buộc đối với baseline đã xác minh này là:

```json
[ "esp:TX", "$serialMonitor:RX", "", [] ],
[ "esp:RX", "$serialMonitor:TX", "", [] ]
```

`src/main.cpp` dùng `Serial.begin(115200)`, nên baud rate kiểm tra phải là
115200. Dòng `Hello ESP32!` xuất hiện sau một chu kỳ HIGH/LOW, khoảng 2 giây.

Không dùng task `PlatformIO: Serial Monitor` mặc định để quan sát ESP32 ảo. Nếu
task báo `/dev/cu.*`, `/dev/tty*` hoặc `COM*`, đó là cổng thiết bị của hệ điều
hành. Dùng `Wokwi Terminal`, hoặc RFC2217 tại `localhost:4000` khi cần kiểm tra
tự động.

## Cổng kiểm chứng bắt buộc

Thực hiện theo thứ tự và không bỏ qua lỗi:

1. Xác thực cú pháp JSON:

   ```bash
   node -e 'JSON.parse(require("fs").readFileSync("diagram.json", "utf8"))'
   ```

2. Build đúng environment:

   ```bash
   pio run -e esp32dev
   ```

3. Xác nhận artifact:

   ```bash
   test -f .pio/build/esp32dev/firmware.elf
   test -f .pio/build/esp32dev/firmware.bin
   ```

4. Lint sơ đồ bằng Wokwi CLI nếu CLI có sẵn:

   ```bash
   wokwi-cli lint
   ```

   Phân biệt rõ `error`, `warning` và `info`. Đối chiếu warning với tài liệu
   Wokwi hiện hành trước khi kết luận part không được hỗ trợ.

5. Start simulator và giữ tab simulator hiển thị. Wokwi có thể pause khi tab bị
   ẩn, vì vậy không dùng trạng thái paused để kết luận firmware hỏng.

6. Phải quan sát được Serial thực tế. Có thể dùng `Wokwi Terminal`, hoặc PySerial
   qua RFC2217:

   ```python
   import serial

   port = serial.serial_for_url(
       "rfc2217://localhost:4000",
       baudrate=115200,
       timeout=5,
   )
   print(port.readline().decode(errors="replace"))
   port.close()
   ```

7. Với LED/GPIO, phải quan sát LED đổi trạng thái hoặc thu tín hiệu bằng Wokwi
   Logic Analyzer. Không chỉ dựa vào việc source code có `digitalWrite()`.

8. Sau kiểm thử, gỡ linh kiện, helper, port hoặc artifact tạm không thuộc thiết
   kế cuối. Chạy lại build và lint trên trạng thái cuối cùng.

## Chẩn đoán theo triệu chứng

| Triệu chứng | Kiểm tra trước |
|---|---|
| Board hiện nhưng linh kiện mất | `parts[].type`; tra Wokwi part reference/schema |
| LED hiện nhưng không nháy | Pin code so với wiring, polarity A/C, simulator có đang pause không |
| Không có Serial | `Serial.begin`, baud, TX/RX wiring, vị trí terminal theo phiên bản extension |
| PlatformIO monitor mở Bluetooth/USB | Đang mở cổng thiết bị thật, không phải Wokwi virtual serial |
| Sửa code nhưng hành vi không đổi | Save, build lại, kiểm tra đúng environment và artifact path |
| `serialMonitor.display` không đổi UI | Kiểm tra changelog/source của extension; UI web và VS Code có thể khác |
| Mong chữ xuất hiện trên sơ đồ | Xác định cần UART terminal hay cần thêm OLED/LCD thật |
| Simulator có board nhưng không chạy | Kiểm tra firmware tồn tại, tab có bị pause, license và log extension |

## Yêu cầu khi báo cáo kết quả

Báo cáo phải nêu:

- board, framework và phiên bản extension đã kiểm tra;
- file đã thay đổi;
- nguồn chính thức đã dùng;
- lệnh build/lint/test đã chạy;
- output thực tế đã quan sát;
- warning còn lại và lý do chấp nhận hoặc cách xử lý.

Nếu chưa quan sát được output hoặc trạng thái phần cứng, phải nói rõ chưa xác
minh thành công và tiếp tục chẩn đoán; không suy diễn từ build success.
