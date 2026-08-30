# Nạp code POC 5 lên board thật — từ cắm cable đến flash thành công

File này ghi nhận quy trình trên Mac (USB-C): cắm cable → nhận port serial →
cài driver (nếu cần) → đưa board vào chế độ nạp → flash firmware POC5 → xem
Serial. Đi theo đúng lệnh trong [README POC5](../../pocs/poc5-cloud-device/README.md)
mục "Provision trên board thật".

> Danh mục linh kiện trong kit nằm tại [KIT-COMPONENTS.md](KIT-COMPONENTS.md).
> File này chỉ giả sử bạn đã: clone repo, cài PlatformIO (kiểm tra
> `pio --version`; nếu `pio` không có trong `PATH`, dùng
> `${HOME}/.platformio/penv/bin/pio`), và có `pocs/poc5-cloud-device/` từ Git.
> Sơ đồ nối mạch POC5 trên breadboard nằm ở mục "Nối mạch POC5 trên breadboard"
> cuối file.

## Tóm tắt luồng

```text
cắm cable → xác định chip USB-serial → cài driver (nếu CP210x/CH34x)
→ pio device list ra /dev/cu.* → (giữ BOOT, nhấn RST)
→ secrets.h → pio run -d pocs/poc5-cloud-device -e esp32dev -t upload
→ pio device monitor -b 115200 → thấy board mở SoftAP ESP32-SETUP-*
```

## 1. Cắm cable

Cắm cable trong kit: đầu USB-C (hoặc USB-A) vào **board**, đầu còn lại vào
port **USB-C của Mac**.

- Cáp phải là **cáp dữ liệu**. Cáp sạc-only (chỉ có 4 dây ±5V/±VBUS) không
  tạo ra port serial. Dấu hiệu: cắm rồi mà `pio device list` (bước 3) không ra
  thêm port nào, hoặc Mac không có bất kỳ âm thanh nhận thiết bị mới nào.
- Khi cắm, board được cấp nguồn qua USB (đây là lúc "nạp code + chạy thử",
  không cần adapter riêng — xem mục 7).

## 2. Xác định chip USB-serial và cài driver

ESP32 chỉ có chân GPIO, không có USB-serial nội bộ trên bản classic; một chip
con riêng trên board (CP210x, CH34x hoặc FTDI) chuyển USB ↔ serial. Nhìn chữ
in trên chip nhỏ cạnh cổng USB:

| Chip (chữ trên IC) | Driver trên macOS | Nguồn tải |
|---|---|---|
| `CP2102` / `CP210x` (Silicon Labs) | Có driver, cần cài | Trang developer của Silicon Labs (khóa "CP210x Universal BCC VCP Drivers") — <https://www.silabs.com/developers/uart> (bản cho Apple Silicon) |
| `CH340` / `CH340G` / `CH341` (WCH) | Có driver, cần cài | Trang WCH (kèm wiki Devantech) — <https://www.wch.cn/downloads/CH341SER_EXE.html> |
| `FT232` / `FT232H` (FTDI) | **Không cần cài** | macOS hỗ trợ sẵn (FTDI) |

> ⚠️ Trạng thái xác minh (2026-08-26): hai link vendor trên chưa được kiểm
> tra trực tiếp do quota Tavily CLI bị chặn. Khi dùng, mở link và chọn đúng
> phiên bản **macOS** (bản macOS mới hỗ trợ Apple Silicon); nếu link 404,
> tìm bằng từ khóa `CP210x VCP driver macOS` / `CH341 driver macOS` tại trang
> chính thức của vendor (silabs.com, wch.cn).

Sau khi cài driver: **khởi động lại Mac**, rút-cắm cable một lần, rồi sang
bước 3.

## 3. Xác nhận Mac thấy port

```bash
pio device list
# hoặc nếu pio không có trong PATH:
~/.platformio/penv/bin/pio device list
```

Mong thấy một dòng tương tự (tên cụ thể tùy driver):

```text
/dev/cu.usbserial-011200XXXX  Serial
# CH34x/FTDI: /dev/cu.usbmodemXXXX01
```

Không thấy port → mục 9 (bảng chẩn đoán), các dòng "không ra port".

## 4. Đưa board vào chế độ nạp (bootloader)

ESP32 chỉ nhận flash khi vào chế độ bootloader (download mode):

1. Giữ nút **BOOT** (một số board in là `IO`/`BOOT`; có board chỉ có nút
   `FLASH` và `RESET`).
2. Giữ BOOT, nhấn nhẹ nút **RST** (hoặc `EN`) một phát rồi thả.
3. Thả nút BOOT.

Với board không có nút vật lý: dùng cable để **giữ hai chân G0 và EN chung
mass (GND)** rồi mới cấp nguồn — nhưng kit này dùng loại có nút, không cần.

Lỗi "A fatal error occurred: (A system error has occurred)" khi upload thường
là bước này bị bỏ: chưa vào bootloader mà đã chạy upload.

## 5. Chuẩn bị secrets trước khi build

Từ repository root:

```bash
cp pocs/poc5-cloud-device/include/secrets.example.h \
   pocs/poc5-cloud-device/include/secrets.h
```

Trong `secrets.h` (file thật, đã gitignore):

- `DEVICE_ID` giữ `esp32-poc5`;
- `DEVICE_TOKEN` thay bằng token riêng và **đúng** với key tương ứng trong
  `POC5_DEVICE_TOKENS_JSON` khi chạy server (xem README POC5 mục
  "Đồng bộ secret firmware và server");
- `WOKWI_PRECONFIG_ENABLED` **phải là `false`** (default trong example đã
  false). Nếu để `true`, board thật NVS rỗng sẽ cố nối mạng giả `Wokwi-GUEST`
  thay vì mở portal.

## 6. Build và flash

```bash
# build (lần đầu sẽ tải WebSockets + ArduinoJson + toolchain)
./scripts/poc.sh 5 build

# flash (thay PORT bằng port ở bước 3)
pio run -d pocs/poc5-cloud-device -e esp32dev \
  -t upload --upload-port /dev/cu.usbserial-XXXX
```

- Nếu `pio` không có trong `PATH`, thay bằng `~/.platformio/penv/bin/pio`.
- Nếu upload lần đầu thất bại, lặp lại bước 4 (BOOT+RST) rồi chạy lại —
  bootloader mode không giữ được lâu.
- Flash xong board tự chạy firmware mới ngay.

## 7. Nguồn điện giữ board vận hành

| Tình huống | Nguồn | Ghi chú |
|---|---|---|
| Nạp code + chạy thử | Cắm Mac (USB-C) | Port USB-C của Mac cấp 5V (1.5–3A); ESP32 Wi-Fi đỉnh ~350mA ≈ 1.75W, trung bình nhỏ hơn nhiều → dư sức. Đây là cách chuẩn. |
| Chạy độc lập (demo, không cắm Mac) | Adapter USB **5V ≥ 1A** hoặc power bank 5V | Chỉ **cấp nguồn** — không có kênh serial nên *không* flash được; muốn flash thì cắm lại Mac. |
| Tránh | Cáp "sạc-only" | Không có kênh dữ liệu → không ra port, không nạp được. |

Không dùng nguồn khác (kể cả rail 5V/3V3 của breadboard hay adapter 9–12V)
để cấp board ESP32.

## 8. Xem Serial sau flash

```bash
pio device monitor -p /dev/cu.usbserial-XXXX -b 115200
# thoát: Ctrl + ]
```

Board chưa có config (NVS rỗng) sẽ in các marker provisioning và mở SoftAP.
Từ đây sang phía server/điện thoại, làm tiếp theo README POC5:

```text
1. Mac: chạy FastAPI (uvicorn, port 8000, 1 worker) → ngrok http 8000
2. Điện thoại: join "ESP32-SETUP-..." (mật khẩu "configure-me")
3. Mở http://192.168.4.1 → nhập Wi-Fi nhà + host ngrok + port 443 + path
   /ws/devices
4. Chờ marker WIFI_CONNECTED → TIME_SYNCED → WSS_AUTHENTICATED →
   CONFIG_COMMITTED (LED GPIO22 sáng)
5. Mở dashboard public → bật/tắt → LED GPIO23 đổi, Serial có
   COMMAND_RECEIVED / DEVICE_STATE_APPLIED / COMMAND_ACK_SENT
```

## 9. Chẩn đoán theo triệu chứng

| Triệu chứng | Kiểm tra trước |
|---|---|
| `pio device list` không ra port mới | Cáp có phải dữ liệu không? Cắm lại port khác? Driver đúng chip đã cài? Reboot sau khi cài driver? |
| Ra port nhưng upload báo "A fatal error occurred (system error)" | Board chưa vào bootloader — làm lại bước 4 (giữ BOOT, nhấn RST). |
| Upload được nhưng monitor không thấy chữ | Baud phải là **115200**; monitor và upload dùng cùng port; thử reset board (nút RST) rồi đọc lại. |
| Board mở AP nhưng điện thoại không thấy | Kiểm tra AP name `ESP32-SETUP-*` ở bước 8; bật cả Wi-Fi 2.4GHz; di chuyển lại gần. |
| Portal vào được nhưng `WIFI_CONNECTED` không ra | Sai SSID/mật khẩu Wi-Fi nhà; AP này chỉ là cổng cấu hình nên điện thoại báo "No Internet" là bình thường. |
| LED GPIO22 không sáng dù Wi-Fi OK | Host ngrok sai/dạng không đúng (chỉ nhận host, không nhận scheme); TLS chain ngrok đã đổi → xem lại trust anchor; token không khớp `POC5_DEVICE_TOKENS_JSON`. |
| Muốn reset config để provision lại | Giữ nút setup ≥ 5 giây (factory reset theo `app_config.h`), hoặc xóa NVS bằng `pio run -t erase` rồi flash lại. |

## 10. Trạng thái xác minh

- Quy trình viết cho **Mac + kit ESP32 Basic Starter + POC5 code tại
  `pocs/poc5-cloud-device/`**, kiểm tra chéo với `pocs/poc5-cloud-device/README.md`,
  `scripts/poc.sh` và `include/app_config.h` (2026-08-26).
- Board đã về (2026-08-27), là ESP32 DevKit V1 30 chân. Việc đối chiếu GPIO 18/19/21/22/23/25
  đã hoàn tất với layout chuẩn + phần tử Wokwi `board-esp32-devkit-v1`; xác nhận lại hàng phải khi nối dây.

## Nối mạch POC5 trên breadboard

POC5 chỉ cần: 4 LED status (vàng/xanh dương/xanh lá/trắng) + 1 LED
`Real_Device` (đỏ), 1 nút setup, và điện trở hạn dòng. Nguồn lấy từ GPIO của
ESP32, **không** bật rail 5V/3V3 của breadboard.

### Cắm trực tiếp lên board hay dùng breadboard? Cần hàn không?

- **Cắm trực tiếp** (que LED vào chân cái của board): vật lý là được, nhưng
  **không nên** ở giai đoạn thử nghiệm:
  - LED phải nối series điện trở 220Ω trước khi vào GPIO; điện trở cũng có 2
    que, nên không có breadboard thì buộc phải xoắn que (lỏng) hoặc hàn.
  - ~12 que cắm thẳng lên board rất lung lay, khó sửa khi thử nhiều phiên.
- **Không cần hàn.** Hàn chỉ dùng khi mạch đã ổn định và muốn cố định lâu
  dài (gắn hộp, để nguyên). Thử nghiệm: breadboard + cáp nhảy, tháo lắp bằng
  tay, không hàn.
- **Dùng breadboard** (MB102 trong kit): các lỗ cùng một hàng ngang thông
  nhau, nên "nối dây" chỉ bằng cách cắm. Cắm board ESP32 lên 2 hàng đầu của
  breadboard (2×15 chân), đặt linh kiện ở các hàng phía dưới.

> Kit chỉ có LED đỏ/vàng/xanh lá + 1 RGB, **không** có LED xanh dương.
> Firmware không quan tâm màu (chỉ bật/tắt GPIO) nên GPIO19 dùng LED xanh lá
> (hoặc kênh xanh của LED RGB) là an toàn.

| GPIO | Vai trò POC5 | LED (màu gợi ý) |
|---:|---|---|
| 18 | SoftAP + HTTP portal đang chạy | vàng |
| 19 | Có station đang join SoftAP | xanh dương* |
| 21 | Wi-Fi nhà đã có IP | xanh lá |
| 22 | WSS với server đã `ready` | trắng |
| 23 | `Real_Device` (lệnh bật/tắt) | đỏ |
| 25 | Nút setup (ngắn = mở portal, giữ ≥5s = reset) | — |

Cách nối mỗi LED (nối series):

```text
GPIO ──[điện trở 220Ω]──► (đầu + LED, chân dài) ──► LED ──► GND
```

- Dùng chính điện trở 220Ω trong kit (GPIO 3.3V, LED thường ~2V → ~6mA).
- LED có cực: chân dài = cực (+), chân ngắn = cực (−). Cắm ngược sẽ không sáng
  (không hỏng, chỉ cần đảo lại).
- Nút setup (GPIO25): một chân → GPIO25, chân còn lại → GND. Code đã dùng
  `INPUT_PULLUP` (xem `app_config.h` / `device_controller.cpp`) nên **không**
  cần thêm điện trở.

> Lưu ý pin map: POC5 khóa theo số GPIO; phần tử Wokwi là `board-esp32-devkit-v1` (30 chân),
> cùng form board thật ESP32 DevKit V1 — nối dây theo số in trên chân.
