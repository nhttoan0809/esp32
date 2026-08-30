# Đề án 3 — Trợ lý giọng nói quản lý kịch bản và năng lượng

## 1. Tóm tắt

Đề án dùng LLM để hiểu mục tiêu cấp cao như “tôi đi ra ngoài”, “chuẩn bị phòng
để ngủ” hoặc “có thiết bị nào còn bật không”. LLM chỉ chọn một scene hoặc tool
được định nghĩa trước; Home Assistant/gateway và ESP32 thực thi rule xác định.

Cách tiếp cận này tận dụng khả năng hiểu ngôn ngữ của LLM mà không cho model tự
phát lệnh điện tùy ý.

## 2. Phạm vi thiết bị

Giữ POC ở tối đa ba endpoint:

1. đèn;
2. quạt hoặc một tải điện áp thấp;
3. cảm biến chuyển động PIR.

Trong Wokwi, tải được biểu diễn bằng LED, servo hoặc relay không nối điện lưới.
Ở giai đoạn thật, ưu tiên ổ cắm thông minh hoặc relay module đã được chứng nhận;
không dùng mạch breadboard để đóng cắt điện lưới.

## 3. Kiến trúc mục tiêu

```text
Voice/Text → STT → LLM
                     │
                     ▼
          activate_scene("away")
                     │
                     ▼
       Home Assistant/rule engine
          │          │          │
          ▼          ▼          ▼
        đèn        quạt       PIR state
          └──────── ESP32/MQTT ─┘
```

Tool ban đầu chỉ nên gồm:

```json
{ "tool": "activate_scene", "arguments": { "scene": "away" } }
```

```json
{ "tool": "get_room_status", "arguments": {} }
```

Các scene hợp lệ được allowlist, ví dụ `away`, `sleep` và `normal`. Tool không
nhận GPIO number, MQTT topic hoặc biểu thức automation do LLM tự tạo.

## 4. POC nhỏ nhất trong repository hiện tại

Giữ board `esp32dev`, framework Arduino và Wokwi hiện tại. POC đầu tiên gồm:

- LED GPIO2 đại diện cho đèn;
- thêm LED hoặc servo đại diện cho quạt;
- thêm `wokwi-pir-motion-sensor`;
- gateway text-based gọi `activate_scene("away")`;
- ESP32 tắt hai actuator, đọc PIR và gửi trạng thái ngược về gateway.

Nếu PIR vẫn báo có người, rule engine có thể yêu cầu xác nhận trước khi kích
hoạt scene. Quyết định xác nhận thuộc rule xác định, không dựa vào suy đoán tự do
của LLM.

## 5. Lộ trình triển khai

### Giai đoạn A — Scene executor trong Wokwi

- Cài đặt ba scene hữu hạn.
- Inject scene qua Serial.
- Quan sát cả hai actuator và trạng thái PIR.
- Từ chối scene hoặc argument không thuộc allowlist.

### Giai đoạn B — MQTT và Home Assistant

- ESP32 publish state/availability và subscribe command.
- Home Assistant biểu diễn các endpoint thành entity.
- Rule engine gọi scene và xác nhận state acknowledgement.

### Giai đoạn C — LLM và giọng nói trên gateway

- LLM ánh xạ câu tự nhiên sang `activate_scene` hoặc `get_room_status`.
- STT/TTS chạy trên Home Assistant, trình duyệt hoặc máy tính.
- Thêm confirmation cho hành động ảnh hưởng nhiều thiết bị.

### Giai đoạn D — Voice hub và thiết bị thật

- Chuyển node actuator sang ESP32 thật.
- Dùng voice satellite ESP32-S3, ESP-VoCat hoặc ESP32-S3-BOX làm giao diện phòng.
- Kết nối tải thật thông qua thiết bị đã chứng nhận và vẫn giữ scene contract.

## 6. Ranh giới mô phỏng

Wokwi kiểm chứng được scene logic, PIR, GPIO, servo/relay, Wi-Fi và MQTT/HTTP.
Microphone, speaker, wake word và full-duplex voice phải được kiểm thử trên voice
hub thật. Không suy ra audio hoạt động chỉ vì networking và firmware build pass.

## 7. Tiêu chí nghiệm thu POC

- Một câu text tạo đúng scene thuộc allowlist.
- Scene không hợp lệ không thay đổi actuator.
- Hai actuator đổi trạng thái thật trong simulator.
- PIR được đọc và phản ánh trong kết quả trả về.
- Gateway chỉ báo thành công sau khi nhận acknowledgement từ ESP32.
- Lệnh lặp cùng request ID không tạo hành động ngoài dự kiến.
- Mất mạng để lại trạng thái an toàn đã định nghĩa và firmware tiếp tục chạy.

## 8. Giá trị và rủi ro chính

Đề án có giá trị cao vì một câu nói có thể điều phối nhiều hành động, đồng thời
scene hữu hạn giúp dễ audit và kiểm thử hơn tool điều khiển tự do. Rủi ro gồm
scene sai ngữ cảnh, trạng thái thiết bị cũ và điều khiển tải điện nguy hiểm. Cần
state acknowledgement, confirmation và phần cứng điện được chứng nhận.

ESP-VoCat là lựa chọn voice hub giai đoạn thật vì kit chính thức có ESP32-S3,
dual microphone, speaker, wake word cục bộ và định hướng tích hợp large-model
agent control. Đây không phải điều kiện bắt buộc cho POC Wokwi.

## 9. Nguồn chính thức

- [Home Assistant API for Large Language Models](https://developers.home-assistant.io/docs/core/llm/)
- [Home Assistant MQTT integration](https://www.home-assistant.io/integrations/mqtt/)
- [Home Assistant Assist](https://www.home-assistant.io/voice_control/)
- [ESPHome Voice Assistant](https://esphome.io/components/voice_assistant/)
- [Espressif ESP-VoCat](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp-vocat/index.html)
- [Wokwi ESP32 Wi-Fi Networking](https://docs.wokwi.com/guides/esp32-wifi)
- [Wokwi PIR Motion Sensor](https://docs.wokwi.com/parts/wokwi-pir-motion-sensor)
- [Wokwi Relay Module](https://docs.wokwi.com/parts/wokwi-relay-module)
- [Wokwi ESP32 Simulation](https://docs.wokwi.com/guides/esp32)

## 10. Trạng thái

Tài liệu này là đề xuất để đánh giá và lập kế hoạch. Chưa có scene executor,
Home Assistant integration hoặc kiểm thử runtime nào được triển khai trong
repository chính.
