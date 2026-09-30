/**
 * @file main_nano.cpp
 * @brief Firmware for Arduino Nano (ATmega328P, 5V) - I/O Sub-Controller
 * @project EXP-03 Distributed Multi-MCU Communication Lab
 */

#include <Arduino.h>
#include "../../include/protocol_defs.h"

// ============================================================================
// ĐỊNH NGHĨA CHÂN NGOẠI VI (PINOUT)
// ============================================================================
#define PIN_RELAY_1     7   // Ngõ ra điều khiển Relay 1 (Active LOW: LOW = BẬT, HIGH = TẮT)
#define PIN_BUZZER      9   // Ngõ ra xung PWM điều khiển Passive Buzzer
#define PIN_LED_HB      13  // Đèn LED tích hợp trên bo Nano (Báo nhịp tim Heartbeat)

#define PIN_BTN_1       2   // Nút nhấn ngõ vào số 1 (Nối GND, bật INPUT_PULLUP)
#define PIN_BTN_2       3   // Nút nhấn ngõ vào số 2 (Nối GND, bật INPUT_PULLUP)

#define PIN_ADC_ACS712  A0  // Ngõ vào tương tự 0 - 5V đọc cảm biến dòng ACS712
#define PIN_ADC_POT     A1  // Ngõ vào tương tự 0 - 5V đọc biến trở xoay 10k

// ============================================================================
// KHAI BÁO BIẾN TRẠNG THÁI & BỘ ĐỆM GIAO THỨC UART
// ============================================================================
enum RxState {
    RX_STATE_WAIT_START = 0,
    RX_STATE_TYPE,
    RX_STATE_SEQ,
    RX_STATE_LEN,
    RX_STATE_PAYLOAD,
    RX_STATE_CHECKSUM,
    RX_STATE_END
};

static RxState rx_state = RX_STATE_WAIT_START;
static uint8_t rx_pkt_type = 0;
static uint8_t rx_seq_num = 0;
static uint8_t rx_payload_len = 0;
static uint8_t rx_payload_idx = 0;
static uint8_t rx_payload[UART_MAX_PAYLOAD_LEN];
static uint8_t rx_checksum = 0;

static uint8_t tx_seq_counter = 0;
static unsigned long last_telemetry_ms = 0;
static unsigned long last_heartbeat_ms = 0;
static bool hb_led_state = false;

// ============================================================================
// HÀM HỖ TRỢ: ĐO ĐIỆN ÁP NGUỒN VCC NỘI BỘ (AVR 1.1V BANDGAP)
// ============================================================================
static uint16_t read_vcc_mv() {
    // Đọc điện áp tham chiếu nội 1.1V dựa trên điện áp tham chiếu VCC
    #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
    delayMicroseconds(250); // Chờ ổn định điện áp
    ADCSRA |= _BV(ADSC);    // Bắt đầu chuyển đổi ADC
    while (bit_is_set(ADCSRA, ADSC));
    uint8_t low  = ADCL;
    uint8_t high = ADCH;
    uint32_t result = (high << 8) | low;
    if (result == 0) return 5000;
    // 1.1V * 1023 * 1000 = 1125300L
    return (uint16_t)(1125300L / result);
    #else
    return 5000; // Mặc định 5.0V nếu mô phỏng
    #endif
}

// ============================================================================
// HÀM GỬI GÓI TIN UART (PACKET TRANSMISSION)
// ============================================================================
static void send_uart_packet(uint8_t packet_type, const uint8_t* payload, uint8_t len) {
    uint8_t seq = tx_seq_counter++;
    uint8_t checksum = calculate_xor_checksum(packet_type, seq, len, payload);

    Serial.write(UART_FRAME_START_BYTE);
    Serial.write(packet_type);
    Serial.write(seq);
    Serial.write(len);
    if (len > 0 && payload != NULL) {
        Serial.write(payload, len);
    }
    Serial.write(checksum);
    Serial.write(UART_FRAME_END_BYTE);
}

static void send_ack(uint8_t ack_seq, uint8_t status_code) {
    AckPayload ack;
    ack.ack_seq = ack_seq;
    ack.status_code = status_code;
    send_uart_packet(PKT_TYPE_RESP_ACK, (const uint8_t*)&ack, sizeof(AckPayload));
}

// ============================================================================
// HÀM XỬ LÝ LỆNH TỪ MASTER (COMMAND DISPATCHER)
// ============================================================================
static void dispatch_rx_packet() {
    switch (rx_pkt_type) {
        case PKT_TYPE_CMD_PING: {
            send_ack(rx_seq_num, 0);
            break;
        }

        case PKT_TYPE_CMD_SET_RELAY: {
            if (rx_payload_len >= sizeof(RelayCmdPayload)) {
                const RelayCmdPayload* cmd = (const RelayCmdPayload*)rx_payload;
                if (cmd->relay_index == 0) {
                    // Relay kích Active LOW: LOW = Đóng tiếp điểm BẬT tải, HIGH = TẮT
                    digitalWrite(PIN_RELAY_1, cmd->state ? LOW : HIGH);
                    send_ack(rx_seq_num, 0);
                } else {
                    send_ack(rx_seq_num, 2); // Mã lỗi: Index không hợp lệ
                }
            } else {
                send_ack(rx_seq_num, 1); // Mã lỗi: Sai kích thước payload
            }
            break;
        }

        case PKT_TYPE_CMD_TRIGGER_BUZZER: {
            if (rx_payload_len >= sizeof(BuzzerCmdPayload)) {
                const BuzzerCmdPayload* cmd = (const BuzzerCmdPayload*)rx_payload;
                if (cmd->freq_hz > 0 && cmd->duration_ms > 0) {
                    tone(PIN_BUZZER, cmd->freq_hz, cmd->duration_ms);
                } else {
                    noTone(PIN_BUZZER);
                }
                send_ack(rx_seq_num, 0);
            } else {
                send_ack(rx_seq_num, 1);
            }
            break;
        }

        default:
            // Không nhận diện được opcode
            send_ack(rx_seq_num, 0xFF);
            break;
    }
}

// ============================================================================
// MÁY TRẠNG THÁI GIẢI MÃ KHUNG TIN UART
// ============================================================================
static void process_uart_rx() {
    while (Serial.available() > 0) {
        uint8_t byte_in = (uint8_t)Serial.read();

        switch (rx_state) {
            case RX_STATE_WAIT_START:
                if (byte_in == UART_FRAME_START_BYTE) {
                    rx_state = RX_STATE_TYPE;
                }
                break;

            case RX_STATE_TYPE:
                rx_pkt_type = byte_in;
                rx_state = RX_STATE_SEQ;
                break;

            case RX_STATE_SEQ:
                rx_seq_num = byte_in;
                rx_state = RX_STATE_LEN;
                break;

            case RX_STATE_LEN:
                rx_payload_len = byte_in;
                rx_payload_idx = 0;
                if (rx_payload_len > UART_MAX_PAYLOAD_LEN) {
                    // Quá kích thước bộ đệm an toàn -> Reset FSM
                    rx_state = RX_STATE_WAIT_START;
                } else if (rx_payload_len == 0) {
                    rx_state = RX_STATE_CHECKSUM;
                } else {
                    rx_state = RX_STATE_PAYLOAD;
                }
                break;

            case RX_STATE_PAYLOAD:
                rx_payload[rx_payload_idx++] = byte_in;
                if (rx_payload_idx >= rx_payload_len) {
                    rx_state = RX_STATE_CHECKSUM;
                }
                break;

            case RX_STATE_CHECKSUM:
                rx_checksum = byte_in;
                rx_state = RX_STATE_END;
                break;

            case RX_STATE_END:
                if (byte_in == UART_FRAME_END_BYTE) {
                    // Kiểm tra tính toàn vẹn Checksum
                    uint8_t expected_chk = calculate_xor_checksum(
                        rx_pkt_type, rx_seq_num, rx_payload_len, rx_payload
                    );
                    if (expected_chk == rx_checksum) {
                        dispatch_rx_packet();
                    } else {
                        // Checksum không khớp -> Báo NACK
                        AckPayload nack;
                        nack.ack_seq = rx_seq_num;
                        nack.status_code = 0xFE; // Mã lỗi Checksum
                        send_uart_packet(PKT_TYPE_RESP_NACK, (const uint8_t*)&nack, sizeof(AckPayload));
                    }
                }
                rx_state = RX_STATE_WAIT_START;
                break;
        }
    }
}

// ============================================================================
// HÀM KHỞI TẠO (SETUP)
// ============================================================================
void setup() {
    // Khởi tạo cổng Serial nối tiếp UART với ESP32 Serial2 (115200 baud)
    Serial.begin(115200);

    // Cấu hình chân điều khiển ngõ ra
    pinMode(PIN_RELAY_1, OUTPUT);
    digitalWrite(PIN_RELAY_1, HIGH); // Mặc định TẮT relay (Active LOW)

    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    pinMode(PIN_LED_HB, OUTPUT);
    digitalWrite(PIN_LED_HB, LOW);

    // Cấu hình nút nhấn ngõ vào số
    pinMode(PIN_BTN_1, INPUT_PULLUP);
    pinMode(PIN_BTN_2, INPUT_PULLUP);

    // Phát tiếng bíp ngắn 2000Hz (60ms) báo khởi động hoàn tất
    tone(PIN_BUZZER, 2000, 60);
}

// ============================================================================
// VÒNG LẶP CHÍNH (LOOP)
// ============================================================================
void loop() {
    // 1. Phân tích gói tin đến từ ESP32 Master
    process_uart_rx();

    unsigned long current_ms = millis();

    // 2. Nhấp nháy đèn LED Heartbeat mỗi 500ms
    if (current_ms - last_heartbeat_ms >= 500) {
        last_heartbeat_ms = current_ms;
        hb_led_state = !hb_led_state;
        digitalWrite(PIN_LED_HB, hb_led_state ? HIGH : LOW);
    }

    // 3. Gửi Telemetry định kỳ mỗi 250ms
    if (current_ms - last_telemetry_ms >= 250) {
        last_telemetry_ms = current_ms;

        NanoTelemetryPayload telem;
        telem.acs712_raw = (uint16_t)analogRead(PIN_ADC_ACS712);
        telem.pot_raw    = (uint16_t)analogRead(PIN_ADC_POT);

        // Trạng thái relay: 1 nếu tiếp điểm đang đóng (chân ở mức LOW)
        telem.relay_status = (digitalRead(PIN_RELAY_1) == LOW) ? 1 : 0;

        // Trạng thái nút nhấn (Active LOW): 1 nếu đang nhấn
        uint8_t btn1 = (digitalRead(PIN_BTN_1) == LOW) ? 1 : 0;
        uint8_t btn2 = (digitalRead(PIN_BTN_2) == LOW) ? 1 : 0;
        telem.button_status = (btn1 << 0) | (btn2 << 1);

        telem.vcc_mv = read_vcc_mv();

        send_uart_packet(PKT_TYPE_TELEMETRY, (const uint8_t*)&telem, sizeof(NanoTelemetryPayload));
    }
}
