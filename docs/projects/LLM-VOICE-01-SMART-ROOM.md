# Đề án 1 — Trợ lý giọng nói cho một phòng thông minh

## 1. Tóm tắt

Đề án xây dựng một trợ lý có thể hiểu yêu cầu tự nhiên như “bật đèn”, “phòng
nóng quá” hoặc “tắt mọi thứ”, sau đó gọi một tập tool hữu hạn để điều khiển tối
đa ba thiết bị trong một phòng.

Đây là đề án nên triển khai đầu tiên vì POC nhỏ nhất chỉ cần LED GPIO2 đã có
trong repository. Microphone, nhận dạng giọng nói và LLM chạy trên máy tính hoặc
Home Assistant; ESP32 trong Wokwi chỉ nhận lệnh có cấu trúc và điều khiển GPIO.

## 2. Phạm vi thiết bị

Giữ phạm vi ban đầu ở tối đa ba endpoint:

1. đèn: LED trong Wokwi, relay hoặc bóng đèn thông minh khi dùng phần cứng thật;
2. quạt/rèm: servo trong Wokwi, quạt hoặc motor controller ở giai đoạn thật;
3. nhiệt độ: DHT22 trong Wokwi, cảm biến tương ứng ở giai đoạn thật.

Không đưa khóa cửa, bếp, máy sưởi hoặc tải điện lưới tự chế vào POC đầu tiên.

## 3. Kiến trúc mục tiêu

```text
Giọng nói trên máy tính/điện thoại
              │
              ▼
       STT → LLM/Assist
              │ tool có schema cố định
              ▼
      Home Assistant/gateway
              │ MQTT hoặc HTTP
              ▼
       ESP32 PlatformIO/Wokwi
              │
              ├── GPIO2 → đèn
              ├── PWM   → servo/quạt
              └── GPIO  ← DHT22
```

LLM không được sinh code GPIO hoặc topic tùy ý. Nó chỉ được chọn các tool đã
khai báo, ví dụ:

```json
{ "tool": "set_light", "arguments": { "on": true } }
```

Firmware phải validate tên tool và toàn bộ argument trước khi thay đổi phần
cứng, sau đó trả trạng thái thực tế thay vì giả định lệnh đã thành công.

## 4. POC nhỏ nhất trong repository hiện tại

Giữ nguyên baseline:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
```

POC đầu tiên chỉ cần:

- LED hiện tại trên GPIO2;
- một command executor nhận `set_light` qua Serial để kiểm thử cục bộ;
- sau đó thay transport bằng MQTT hoặc HTTP qua `Wokwi-GUEST`;
- một gateway phía máy tính chuyển text hoặc kết quả LLM thành JSON hợp lệ;
- ESP32 gửi lại trạng thái `on`, `off` hoặc lỗi validation.

Microphone chưa cần xuất hiện ở bước này. Có thể nhập câu lệnh bằng text để
kiểm chứng trước ranh giới LLM → tool → ESP32 → GPIO.

## 5. Lộ trình triển khai

### Giai đoạn A — Tool contract và một LED

- Định nghĩa tool `set_light(on)`.
- Inject JSON qua Serial hoặc test harness.
- Quan sát LED đổi trạng thái và Serial phản hồi trạng thái thật.

### Giai đoạn B — LLM và mạng trong Wokwi

- Kết nối ESP32 với MQTT broker hoặc HTTP gateway.
- Dùng text input trên máy tính để gọi LLM.
- Chỉ chuyển tool call đã validate đến ESP32.
- Kiểm thử mất mạng, lệnh trùng và argument không hợp lệ.

### Giai đoạn C — Giọng nói trên gateway

- Thêm STT/TTS trên Home Assistant, trình duyệt hoặc máy tính.
- Giữ nguyên schema tool và firmware ESP32.
- Đo thời gian từ lúc kết thúc câu nói đến lúc LED đổi trạng thái.

### Giai đoạn D — Thiết bị thật

- Chuyển firmware actuator sang ESP32 thật trước.
- Thêm ESP32-S3 voice satellite có microphone, speaker và PSRAM, hoặc dùng một
  voice kit chính thức như ESP-VoCat.
- Chỉ chuyển audio frontend sang phần cứng; không thay contract điều khiển đã
  kiểm chứng trong Wokwi.

## 6. Ranh giới mô phỏng

Wokwi phù hợp để kiểm chứng Wi-Fi, MQTT/HTTP, JSON validation, GPIO, LED, servo
và cảm biến. Không dùng Wokwi để tuyên bố microphone, I2S hoặc speech recognition
đã hoạt động: danh sách phần cứng chính thức không liệt kê microphone và I2S
không được mô phỏng đầy đủ cho chuỗi audio ESP32-S3.

## 7. Tiêu chí nghiệm thu POC

POC chỉ hoàn thành khi quan sát được:

- câu text hoặc transcript tạo đúng một tool call thuộc allowlist;
- JSON sai bị từ chối mà không thay đổi GPIO;
- LED thật sự đổi trạng thái trong simulator;
- trạng thái được ESP32 gửi ngược về gateway;
- mất mạng không làm firmware treo và không tạo hành động lặp ngoài ý muốn;
- API key, Wi-Fi password và token không nằm trong firmware hoặc Git.

Build thành công mà chưa quan sát GPIO và phản hồi runtime chưa được tính là
nghiệm thu.

## 8. Giá trị và rủi ro chính

Giá trị lớn nhất là đường đi từ POC một LED đến phòng thật gần như không thay
đổi kiến trúc. Rủi ro chính gồm LLM gọi nhầm tool, độ trễ mạng và trạng thái giữa
gateway với ESP32 không đồng bộ. Giảm rủi ro bằng schema chặt, idempotency key,
timeout hữu hạn và state acknowledgement từ thiết bị.

## 9. Nguồn chính thức

- [Home Assistant API for Large Language Models](https://developers.home-assistant.io/docs/core/llm/)
- [Home Assistant MQTT integration](https://www.home-assistant.io/integrations/mqtt/)
- [ESPHome Voice Assistant](https://esphome.io/components/voice_assistant/)
- [Wokwi ESP32 Wi-Fi Networking](https://docs.wokwi.com/guides/esp32-wifi)
- [Wokwi Supported Hardware](https://docs.wokwi.com/getting-started/supported-hardware)
- [Wokwi ESP32 Simulation](https://docs.wokwi.com/guides/esp32)
- [PlatformIO ESP32 Dev Module](https://docs.platformio.org/en/latest/boards/espressif32/esp32dev.html)

## 10. Trạng thái

Tài liệu này là đề xuất để lựa chọn và lập kế hoạch. Chưa có firmware, gateway
hoặc kiểm thử runtime nào của đề án được triển khai trong repository chính.
