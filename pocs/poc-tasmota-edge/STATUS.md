# Trạng Thái POC-B (Tasmota Edge)

- **Trạng thái:** Sẵn sàng kiểm thử & nạp cấu hình
- **Ngày hoàn thành:** 2026-10-04
- **Các thành phần đã xác thực:**
  - [x] Template JSON cấu hình chân GPIO (`templates/esp32_devkit_template.json`)
  - [x] Tập lệnh cấu hình & Rules tự chủ tại biên (`rules/edge_rules.txt`)
  - [x] Trình mô phỏng thiết bị rìa Tasmota độc lập (`scripts/simulate_tasmota_node.py`)
  - [x] Sơ đồ mạch Wokwi `diagram.json` chuẩn hoá nhãn theo quy tắc `AGENTS.md`
  - [x] Kịch bản nạp & triển khai `scripts/apply_tasmota_config.sh`
