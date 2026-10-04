#!/usr/bin/env bash
# ==============================================================================
# Script hỗ trợ nạp firmware Tasmota và cấu hình Template cho ESP32 DevKit V1
# ==============================================================================

set -e

PORT=${1:-"/dev/cu.usbserial-0001"}
BAUD=${2:-"921600"}
FIRMWARE_URL="http://ota.tasmota.com/tasmota32/release/tasmota32.factory.bin"

echo "================================================================="
echo "  POC-B: Flash & Cấu hình Tasmota Runtime Monolith cho ESP32     "
echo "================================================================="
echo "Cổng nạp: $PORT (Baud: $BAUD)"

# 1. Tải binary chính thức nếu chưa có
mkdir -p bin
if [ ! -f "bin/tasmota32.factory.bin" ]; then
    echo ">>> Đang tải pre-compiled binary tasmota32.factory.bin..."
    curl -L "$FIRMWARE_URL" -o "bin/tasmota32.factory.bin"
fi

# 2. Xoá flash và nạp firmware qua esptool
echo ">>> Nạp firmware Tasmota..."
esptool.py --chip esp32 --port "$PORT" --baud "$BAUD" erase_flash
esptool.py --chip esp32 --port "$PORT" --baud "$BAUD" write_flash 0x0 bin/tasmota32.factory.bin

echo "================================================================="
echo "✅ NẠP FIRMWARE HOÀN TẤT!"
echo "Các bước tiếp theo tại Runtime (Không cần biên dịch lại):"
echo "1. Kết nối vào Wi-Fi SoftAP của thiết bị: tasmota-XXXXXX"
echo "2. Truy cập http://192.168.4.1 để cấu hình Wi-Fi mạng nhà."
echo "3. Vào Configuration -> Configure Other -> dán nội dung từ:"
echo "   templates/esp32_devkit_template.json"
echo "4. Vào Tools -> Console và dán các lệnh trong:"
echo "   rules/edge_rules.txt"
echo "================================================================="
