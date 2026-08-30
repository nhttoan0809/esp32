# Đề án 2 — Trợ lý an toàn và hỗ trợ người lớn tuổi, offline-first

## 1. Tóm tắt

Đề án cung cấp một số hành động thiết yếu có thể chạy tại chỗ ngay cả khi mất
Internet, đồng thời dùng LLM cho câu hỏi tự nhiên, diễn giải trạng thái và những
yêu cầu không thuộc tập lệnh an toàn cố định.

Nguyên tắc cốt lõi là LLM không nằm trên đường thực thi bắt buộc của SOS. Nút
bấm vật lý và command offline đã xác minh phải tiếp tục hoạt động khi gateway,
LLM hoặc mạng không khả dụng.

## 2. Phạm vi thiết bị

Giữ phạm vi ở tối đa ba endpoint:

1. đèn đầu giường;
2. còi hoặc đèn báo SOS điện áp thấp;
3. cảm biến cửa hoặc cảm biến chuyển động.

POC không tự động gọi dịch vụ khẩn cấp, mở khóa cửa hoặc gửi thông báo cho người
thật. Những hành động đó cần quy trình đồng ý, danh tính người nhận và kiểm thử
vận hành riêng.

## 3. Kiến trúc mục tiêu

```text
                    ┌── command offline ──> state machine ──> GPIO
Microphone thật ────┤
                    └── câu ngoài allowlist ──> gateway/LLM
                                                   │
                                                   └── tool đã validate

Nút SOS vật lý ─────────────────────────────> state machine ──> còi/đèn
```

Các hành động cục bộ nên dùng ID cố định thay vì câu tự do:

```json
{ "intent": "SOS_TRIGGER", "source": "button" }
```

LLM có thể trả lời “cửa đang mở” hoặc ánh xạ một câu diễn đạt khác sang intent,
nhưng không được hủy SOS, mở khóa hoặc tắt cảnh báo mà không có rule xác thực.

## 4. POC nhỏ nhất trong Wokwi

POC đầu tiên vẫn dùng `esp32dev` + Arduino và không cố mô phỏng microphone:

- LED GPIO2 đại diện cho đèn;
- thêm `wokwi-pushbutton` làm nút SOS;
- thêm `wokwi-buzzer` hoặc LED thứ hai làm cảnh báo;
- inject các intent `LIGHT_ON`, `LIGHT_OFF`, `SOS_TRIGGER` qua Serial/MQTT;
- thêm một intent không xác định để kiểm tra nhánh fallback tới gateway giả lập.

Mục tiêu của Wokwi là kiểm chứng state machine, ưu tiên sự kiện, debounce, khả
năng phục hồi sau mất mạng và việc nút SOS không phụ thuộc LLM.

## 5. Lộ trình triển khai

### Giai đoạn A — State machine an toàn

- Nút SOS vật lý kích hoạt cảnh báo cục bộ.
- Intent offline điều khiển đèn.
- Lệnh không hợp lệ không làm thay đổi trạng thái.
- Reboot không tạo SOS giả hoặc xóa cảnh báo đang cần xử lý.

### Giai đoạn B — LLM fallback

- Gateway nhận text thay cho giọng nói.
- LLM chỉ được gọi các tool đọc trạng thái và điều khiển đèn.
- SOS luôn do state machine xác định; LLM chỉ có thể giải thích trạng thái.

### Giai đoạn C — Speech recognition trên phần cứng thật

- Chuyển audio frontend sang ESP32-S3-Korvo, ESP32-S3-BOX hoặc board audio tương
  đương được Espressif hỗ trợ.
- Dùng ESP-SR AFE + WakeNet + MultiNet cho wake word và command cố định.
- Đo false accept, false reject và hành vi trong tiếng ồn thực tế.

### Giai đoạn D — Vận hành giới hạn

- Thêm heartbeat và cảnh báo mất kết nối tới gateway.
- Lưu event ID và timestamp để tránh gửi trùng.
- Chỉ tích hợp thông báo ra bên ngoài sau khi có quy tắc retry, xác nhận và hủy.

## 6. Giới hạn ngôn ngữ

Tài liệu MultiNet hiện hành công bố hỗ trợ command tiếng Trung và tiếng Anh.
Không mặc định ESP-SR nhận dạng được tiếng Việt. Nếu yêu cầu dùng tiếng Việt,
STT phải chạy ở gateway/cloud hoặc sử dụng một model khác đã được đo kiểm độc
lập; các command an toàn vẫn cần nút vật lý hoặc đường fallback xác định.

## 7. Ranh giới mô phỏng

Wokwi dùng để kiểm chứng GPIO, button, buzzer, networking và state machine. Audio
capture, AFE, wake word và MultiNet phải được kiểm thử trên board thật vì Wokwi
không cung cấp chuỗi microphone/I2S đầy đủ để làm cổng nghiệm thu speech.

## 8. Tiêu chí nghiệm thu

### POC mô phỏng

- Nút SOS kích hoạt cảnh báo khi gateway hoàn toàn tắt.
- Intent hợp lệ tạo đúng một hành động.
- Intent sai hoặc payload lỗi không thay đổi GPIO.
- Mất MQTT/HTTP không chặn vòng lặp chính.
- Serial ghi rõ nguồn sự kiện, intent ID và trạng thái cuối.

### Prototype thật

- Quan sát wake word và command thực tế trên board audio.
- Đo false accept/false reject với nhiều người nói và tiếng ồn dự kiến.
- Xác minh command an toàn tiếp tục hoạt động khi Internet bị ngắt.
- Xác minh tiếng Việt bằng dữ liệu thực trước khi công bố hỗ trợ.

## 9. Giá trị và rủi ro chính

Đề án phù hợp khi ưu tiên độ tin cậy, riêng tư và khả năng hoạt động khi offline.
Rủi ro lớn nhất là hiểu sai giọng nói trong tình huống nhạy cảm. Vì vậy speech
recognition không được là cơ chế duy nhất cho SOS, và LLM không được tự quyết
định hành động có hậu quả lớn.

## 10. Nguồn chính thức

- [ESP-SR Getting Started cho ESP32-S3](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/getting_started/readme.html)
- [ESP-SR MultiNet Command Word](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/speech_command_recognition/README.html)
- [ESP-SR WakeNet](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/wake_word_engine/README.html)
- [ESP-SR Audio Front-end Framework](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/audio_front_end/README.html)
- [ESP32-S3-Korvo-1 hardware guide](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md)
- [Wokwi ESP32 Simulation](https://docs.wokwi.com/guides/esp32)
- [Wokwi Supported Hardware](https://docs.wokwi.com/getting-started/supported-hardware)

## 11. Trạng thái

Tài liệu này là đề xuất kiến trúc, không phải thiết bị an toàn đã được chứng
nhận. Chưa có firmware, audio model hoặc kiểm thử runtime của đề án được triển
khai trong repository chính.
