# POC5 — Đổi phần tử Wokwi sang board 30 chân ESP32 DevKit V1

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** POC5 hiện mô phỏng bằng phần tử Wokwi 40 chân (`board-esp32-devkit-c-v4`) trong khi board thật trong kit là **ESP32 DevKit V1, 30 chân**. Đổi diagram sang phần tử 30 chân (`board-esp32-devkit-v1`) để sơ đồ khớp 1-1 với board thật, xác minh bằng `wokwi-cli lint`, và cập nhật các doc phần cứng cho khớp thực tế.

**Bối cảnh / Vấn đề:**

- Board thật (kiểm tra vật lý, 2026-08-27): ghi nhãn **ESP32 DEVKITV1**, 30 chân (2×15).
  Hàng trái (từ trên xuống): `EN, VP, VN, D34, D35, D32, D33, D25, D26, D27, D14, D12, D13, GND, VIN`.
  (Ghi chú: bản ghi của người dùng có "D12" xuất hiện 2 lần — đây là lỗi gõ thừa,
  KHÔNG phải board có 16 chân. Thực tế board khớp 1-1 layout 15 chân của phần tử
  Wokwi 30 chân (đã đối chiếu source element): EN, VP, VN, D34, D35, D32, D33,
  D25, D26, D27, D14, D12, D13, GND, VIN — và **hàng trái không có 3V3, chỉ có VIN**.)
- Wokwi có phần tử đúng 30 chân: `board-esp32-devkit-v1`
  (source: [esp32-devkit-v1-element.ts](https://github.com/wokwi/wokwi-elements/blob/master/src/esp32-devkit-v1-element.ts)
  trong repo chính thức `wokwi/wokwi-elements`). Hàng trái của element trùng khớp từng vị
  trí với board thật đã đọc.
- Firmware POC5 khóa theo **số GPIO** (18/19/21/22/23/25), không theo hình board →
  không cần sửa code firmware, chỉ đổi phần tử + tên pin trong `diagram.json`.
- Pin trên phần tử 30 chân có naming khác DevKitC v4: `18`→`D18`, `25`→`D25`,
  `TX`→`TX0`, `RX`→`RX0` (đã kiểm chứng danh sách pin hợp lệ bằng lint, xem Task 2).

**Đã xác minh trước khi viết plan (2026-08-30, `wokwi-cli v0.26.1`):**

- Bản sao diagram POC5 đã đổi sang `board-esp32-devkit-v1` + pin `D*` lint ở
  `/tmp/poc5-lint-test`: **1 info `unsupported-part`, không có error/warning** —
  cùng mức với baseline `board-esp32-devkit-c-v4` hiện tại (STATUS.md POC5 ghi nhận
  baseline cũng chỉ có 1 info này).
- Test âm: nối `esp:D99` → lint báo **error `invalid-pin`** và in danh sách pin hợp lệ:
  `3V3, D12, D13, D14, D15, D18, D19, D2, D21, D22, D23, D25, D26, D27, D32, D33,
  D34, D35, D4, D5, EN, GND.1, GND.2, RX0, RX2, TX0, TX2, VIN, VN, VP`.
  → lint thật sự validate pin, và 6 GPIO POC5 + `GND.1`, `TX0`, `RX0` đều nằm trong danh sách.

**Tech Stack:** `wokwi-cli` v0.26.1 (tại `/Users/toannguyen/bin/wokwi-cli`), cần
`WOKWI_CLI_TOKEN` (do người dùng cung cấp trong phiên 2026-08-30 — **không được
hard-code hay commit token vào repo**, AGENTS.md "Token Wokwi").

## Global Constraints

- Chỉ sửa diagram POC5 + docs phần cứng; **không sửa code firmware**
  (`pocs/poc5-cloud-device/src`, `include`) vì pin GPIO không đổi.
- File `diagram.json` ở root (Bài 1 blink, GPIO2, DevKitC v4) và `AGENTS.md`
  (baseline DevKitC v4 của root) **để nguyên** — khác project, ngoài phạm vi.
- Mọi `type`/pin trong `diagram.json` phải khớp danh sách pin hợp lệ đã kiểm
  chứng ở trên; không tự đoán identifier (AGENTS.md).
- Kết luận "lint sạch" chỉ được đưa ra sau khi chạy lint trên **file thật**
  sau khi sửa (Task 2), không dựa vào kết quả lint bản sao ở /tmp.
- Token: `export WOKWI_CLI_TOKEN='<token>'` từ shell, không ghi vào file commit.

---

## Vấn đề 1 (Task 1): Đổi `diagram.json` POC5 sang phần tử 30 chân

**Files:**
- Modify: `pocs/poc5-cloud-device/diagram.json`

**Interfaces:**
- Consumes: danh sách pin hợp lệ của `board-esp32-devkit-v1` (đã kiểm chứng ở trên)
- Produces: diagram POC5 mô phỏng đúng form board thật (30 chân), cùng 6 GPIO 18/19/21/22/23/25

- [x] **Step 1: Đổi phần tử**

Thay trong `parts`:

```diff
-    { "type": "board-esp32-devkit-c-v4", "id": "esp", "top": 10, "left": 10, "attrs": {} },
+    { "type": "board-esp32-devkit-v1", "id": "esp", "top": 10, "left": 10, "attrs": {} },
```

- [x] **Step 2: Đổi tên pin trong `connections`**

Bảng ánh xạ (chỉ các prefix `esp:`, các id linh kiện khác giữ nguyên):

| Cũ (DevKitC v4) | Mới (DevKit V1) | Dùng ở |
|---|---|---|
| `esp:18` | `esp:D18` | LED setup (vàng) |
| `esp:19` | `esp:D19` | LED client (xanh dương) |
| `esp:21` | `esp:D21` | LED wifi (xanh lá) |
| `esp:22` | `esp:D22` | LED server (trắng) |
| `esp:23` | `esp:D23` | Real_Device (đỏ) |
| `esp:25` | `esp:D25` | Nút setup |
| `esp:TX` | `esp:TX0` | Serial monitor |
| `esp:RX` | `esp:RX0` | Serial monitor |
| `esp:GND.1` | *(không đổi)* | GND chung |

Kết quả mong đợi (block `connections`):

```json
[ "esp:D18", "ledSetup:A", "yellow", [] ],
[ "ledSetup:C", "esp:GND.1", "black", [] ],
[ "esp:D19", "ledClient:A", "blue", [] ],
[ "ledClient:C", "esp:GND.1", "black", [] ],
[ "esp:D21", "ledWifi:A", "green", [] ],
[ "ledWifi:C", "esp:GND.1", "black", [] ],
[ "esp:D22", "ledServer:A", "white", [] ],
[ "ledServer:C", "esp:GND.1", "black", [] ],
[ "esp:D23", "realDevice:A", "red", [] ],
[ "realDevice:C", "esp:GND.1", "black", [] ],
[ "esp:D25", "setupButton:1.l", "orange", [] ],
[ "setupButton:2.l", "esp:GND.1", "black", [] ],
[ "esp:TX0", "$serialMonitor:RX", "", [] ],
[ "esp:RX0", "$serialMonitor:TX", "", [] ]
```

- [x] **Step 3: Xác thực JSON**

Run:

```bash
node -e 'JSON.parse(require("fs").readFileSync("pocs/poc5-cloud-device/diagram.json", "utf8"))'
```

Expected: không throw.

- [x] **Step 4: Commit**

```bash
git add pocs/poc5-cloud-device/diagram.json
git commit -m "feat(poc5): switch Wokwi diagram to 30-pin ESP32 DevKit V1 part"
```

---

## Vấn đề 2 (Task 2): Xác minh bằng `wokwi-cli lint`

**Files:**
- Modify: (none — lint không đổi file)

**Interfaces:**
- Consumes: diagram đã đổi ở Task 1; `WOKWI_CLI_TOKEN` do người dùng cấp
- Produces: bằng chứng lint (1 info `unsupported-part`, 0 error/warning) để
  ghi nhận lại vào STATUS.md

- [x] **Step 1: Export token**

```bash
export WOKWI_CLI_TOKEN='<token-đã-cấp>'
# Token không được ghi vào file, commit hay log (AGENTS.md: Token Wokwi).
```

- [x] **Step 2: Lint diagram POC5**

Run:

```bash
cd pocs/poc5-cloud-device && wokwi-cli lint .
```

Expected:

```text
ℹ [unsupported-part] Part "esp" uses undocumented type "board-esp32-devkit-v1".
  This part may change or be removed in future versions. (esp)

Found 1 info
```

Nếu xuất hiện `error` hoặc `warning` (đặc biệt `invalid-pin`) → dừng, đối chiếu
danh sách pin hợp lệ trong mục "Đã xác minh" ở đầu file này, sửa lại
`diagram.json` rồi lint lại.

- [x] **Step 3: Smoke test Serial sau khi đổi phần tử**

Chân `TX`/`RX` (DevKitC v4) đổi thành `TX0`/`RX0` (DevKit V1). Source element
DevKit V1 khai báo `TX0`/`RX0` mang signal `usart(0)` — tức vẫn là UART0,
giống `TX`/`RX` của DevKitC v4. Nhưng lint chỉ validate **tên pin**, không
validate wiring serial monitor → phải chạy một lần simulate để chắc rằng
`$serialMonitor` vẫn bắt được output:

```bash
cd /Users/toannguyen/Documents/esp32-learning
./scripts/poc.sh 5 simulate --expect-text HTTP_SERVER_STARTED
```

Expected: POC5 firmware boot tới `HTTP_SERVER_STARTED` (marker provisioning)
và không có dấu hiệu Serial chết (mất `HTTP_SERVER_STARTED` → UART0 không
được nối đúng sang `$serialMonitor` trong phần tử mới; đối chiếu lại tên
chân TX/RX với source element trước khi sửa tiếp).

> Bước này dùng chung `WOKWI_CLI_TOKEN` từ Step 1 (xem README POC5 +
> AGENTS.md "Token Wokwi"). Chỉ chạy simulate, không chạy lại full TLS/ngrok
> (out of scope — xem "Không trong phạm vi").

- [x] **Step 4: Cập nhật ghi nhận lint trong STATUS.md**

Sửa mục "Wokwi lint info còn lại" trong `pocs/poc5-cloud-device/STATUS.md`:
identifier đổi từ `board-esp32-devkit-c-v4` sang `board-esp32-devkit-v1`
(cùng lý do giữ: schema/docs Wokwi chấp nhận, không có warning/error).

- [x] **Step 5: Commit**

```bash
git add pocs/poc5-cloud-device/STATUS.md
git commit -m "docs(poc5): record DevKit V1 lint + simulate result in STATUS"
```

---

## Vấn đề 3 (Task 3): Cập nhật docs phần cứng cho khớp board thật

**Files:**
- Modify: `docs/hardware/KIT-COMPONENTS.md`
- Modify: `docs/hardware/HARDWARE-SETUP-POC5.md`
- Modify: `docs/pocs/POC-05-CLOUD-WEBSOCKET-DEVICE.md`

**Interfaces:**
- Consumes: thông tin board thật (nhãn ESP32 DEVKITV1, 30 chân, hàng trái đã đọc)
- Produces: docs nhất quán: board thật = DevKit V1 30 chân; phần tử Wokwi POC5
  = `board-esp32-devkit-v1` (30 chân, khớp form thật)

- [x] **Step 1: `docs/hardware/KIT-COMPONENTS.md`**

Dòng 17 (mục 1.1), đổi:

```diff
-| Board ESP32 | 1 | Kit không ghi model cụ thể; thường là bản clone 30 chân (loại DevKitC). |
+| Board ESP32 | 1 | Ghi nhãn **ESP32 DevKit V1, 30 chân** (kiểm tra vật lý 2026-08-27). Hàng trái (từ trên): EN, VP, VN, D34, D35, D32, D33, D25, D26, D27, D14, D12, D13, GND, VIN. |
```

> Ghi chú khi đối chiếu lại board thật: bản ghi 2026-08-27 có "D12" lặp 2 lần —
> đây là lỗi gõ thừa, board chỉ có 15 chân mỗi hàng (khớp 1-1 layout phần tử
> `board-esp32-devkit-v1` trong source Wokwi). Hàng phải (từ trên xuống):
> D23, D22, TX0, RX0, D21, D19, D18, D5, TX2, RX2, D4, D2, D15, GND, 3V3.
> Đặc điểm cần ghi nhớ khi nối mạch: **hàng trái không có `3V3` (chỉ có
> `VIN`)**; chân `3V3` nằm ở vị trí cuối (dưới cùng) hàng phải, ngay cạnh
> `GND.1` — dùng chân đó khi cần nguồn 3.3V cho breadboard. Nếu đối chiếu
> phát hiện board khác layout này thì sửa lại bảng cho khớp board thật.

Thêm vào mục 5 (Trạng thái xác minh): board đã về, model xác nhận từ nhãn vật lý
(2026-08-27); 6 GPIO POC5 có trên board (D25 hàng trái; D18/D19/D21/D22/D23
hàng phải — đã đối chiếu với layout chuẩn + phần tử Wokwi).

- [x] **Step 2: `docs/hardware/HARDWARE-SETUP-POC5.md`**

- Note pin map (gần dòng 223, "Lưu ý pin map: POC5 khóa theo ESP32 DevKitC"):
  đổi thành "POC5 khóa theo số GPIO; phần tử Wokwi là
  `board-esp32-devkit-v1` (30 chân), cùng form board thật ESP32 DevKit V1 —
  nối dây theo số in trên chân."
- Mục 10 (Trạng thái xác minh): cập nhật — board đã về (2026-08-27), là
  ESP32 DevKit V1 30 chân; việc đối chiếu GPIO 18/19/21/22/23/25 chuyển từ
  "chờ kit về" sang "đã đối chiếu với layout chuẩn + phần tử Wokwi; xác nhận
  lại hàng phải khi nối dây".

- [x] **Step 3: `docs/pocs/POC-05-CLOUD-WEBSOCKET-DEVICE.md`**

- Mục 5 (gần dòng 139): "Pin map mục tiêu cho ESP32 DevKitC V4" →
  "Pin map mục tiêu cho ESP32 DevKit V1 (30 chân; phần tử Wokwi
  `board-esp32-devkit-v1`)".
- Đoạn nêu identifier part (gần dòng 152): `board-esp32-devkit-c-v4` →
  `board-esp32-devkit-v1`.
- Link tham khảo (dòng 468) có thể bổ sung trang DevKit V1 bên cạnh link
  DevKitC V4 (tuỳ chọn, giữ link cũ).

- [x] **Step 4: Verify không còn reference cũ sai nghĩa**

Run:

```bash
grep -rn "devkit-c-v4" docs/hardware/ docs/pocs/ pocs/poc5-cloud-device/ || echo "clean"
```

Expected: `clean` (nếu còn hit trong STATUS.md là phần "baseline cũ" được
cố ý giữ làm lịch sử thì chấp nhận, ghi chú rõ).

- [x] **Step 5: Commit**

```bash
git add docs/hardware/KIT-COMPONENTS.md docs/hardware/HARDWARE-SETUP-POC5.md \
  docs/pocs/POC-05-CLOUD-WEBSOCKET-DEVICE.md
git commit -m "docs: record kit board as ESP32 DevKit V1 30-pin, align POC5 Wokwi part"
```

---

## Không trong phạm vi (chủ đích)

- `diagram.json` + `wokwi.toml` ở **root** (Bài 1 blink, GPIO2) và baseline
  DevKitC v4 trong `AGENTS.md` — khác project; muốn đổi thì làm plan riêng.
- Code firmware POC5 (`src/`, `include/app_config.h`) — GPIO không đổi.
- `wokwi.toml` POC5 — không đổi (chỉ trỏ build artifact).
- Chạy lại simulator headless POC5 (TLS/ngrok) — ngoài scope, phần này STATUS
  đã nêu là gate tiếp theo.

## Self-Review Checklist

- [x] **Mỗi "vấn đề" là 1 Task riêng** (1: diagram, 2: lint, 3: docs) — khớp 3
  thay đổi đã nêu với user, mỗi task có Files/Steps/Expected/Commit riêng.
- [x] **Không placeholder**: pin mapping, danh sách pin hợp lệ, output lint
  mong đợi đều cụ thể (từ kết quả đã chạy 2026-08-30).
- [x] **Token không lọt vào repo**: chỉ reference cách export, không ghi giá trị.
- [x] **Không sửa firmware/root diagram**: phạm vi gói gọn POC5 + 3 docs phần cứng.
