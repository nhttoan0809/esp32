/**
 * @file main_master.cpp
 * @brief Firmware for ESP32 DevKit V1 (30 pins) - Master Gateway & Coordinator
 * @project EXP-03 Distributed Multi-MCU Communication Lab
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include "../../include/protocol_defs.h"
#include "../../include/web_dashboard.h"

// ============================================================================
// CẤU HÌNH CHÂN PHẦN CỨNG (PIN CONFIGURATION - RULE 7)
// ============================================================================
// 1. Tuyến I2C Master tới STM32F4 Black Pill (3.3V Logic)
#define PIN_I2C_SDA         21
#define PIN_I2C_SCL         22

// 2. Tuyến Hardware Serial2 tới Arduino Nano (5V Logic qua phân áp)
#define PIN_UART2_RX        16  // Nhận dữ liệu từ Nano TX (qua cầu phân áp 1k/2k về 3.3V)
#define PIN_UART2_TX        17  // Xuất dữ liệu sang Nano RX (3.3V trực tiếp)

// 3. Đèn LED chỉ thị trạng thái hoạt động (mắc nối tiếp điện trở 220Ω)
#define PIN_LED_I2C         25  // LED Xanh lục: Báo hoạt động truyền thông bus I2C
#define PIN_LED_UART        26  // LED Vàng: Báo nhận gói tin UART từ Nano
#define PIN_LED_SYS         27  // LED Xanh dương: Báo trạng thái mạng / Master System

// ============================================================================
// CẤU HÌNH WI-FI & WEB SERVER
// ============================================================================
static const char* AP_SSID = "ESP32-EXP03-MASTER";
static const char* AP_PASS = "12345678";

static WebServer server(80);
static WebSocketsServer webSocket(81);

// ============================================================================
// BIẾN TRẠNG THÁI & DỮ LIỆU TELEMETRY
// ============================================================================
// 1. Trạng thái ESP32 Master
static uint32_t uart_rx_count = 0;
static uint32_t uart_err_count = 0;

// 2. Trạng thái STM32F4 Coprocessor (Node 2)
static bool     stm32_online = false;
static uint8_t  stm32_status_reg = 0;
static int32_t  stm32_stepper_pos = 0;
static uint32_t stm32_dsp_time_us = 0;
static uint16_t stm32_dsp_hash = 0;
static unsigned long last_stm32_rx_ms = 0;

// 3. Trạng thái Arduino Nano Sub-Controller (Node 3)
static bool     nano_online = false;
static NanoTelemetryPayload nano_data = {512, 0, 0, 0, 5000};
static unsigned long last_nano_rx_ms = 0;
static uint8_t  master_uart_seq = 0;

// 4. Bộ đếm thời gian
static unsigned long last_i2c_poll_ms = 0;
static unsigned long last_ws_broadcast_ms = 0;
static unsigned long last_heartbeat_ms = 0;
static bool sys_led_state = false;

// ============================================================================
// GIAO THỨC UART: TRUYỀN GÓI TIN TỚI ARDUINO NANO
// ============================================================================
static void send_uart2_packet(uint8_t packet_type, const uint8_t* payload, uint8_t len) {
    uint8_t seq = master_uart_seq++;
    uint8_t checksum = calculate_xor_checksum(packet_type, seq, len, payload);

    Serial2.write(UART_FRAME_START_BYTE);
    Serial2.write(packet_type);
    Serial2.write(seq);
    Serial2.write(len);
    if (len > 0 && payload != NULL) {
        Serial2.write(payload, len);
    }
    Serial2.write(checksum);
    Serial2.write(UART_FRAME_END_BYTE);
}

// ============================================================================
// MÁY TRẠNG THÁI GIẢI MÃ KHUNG TIN UART2 TỪ NANO
// ============================================================================
enum UartRxState {
    U_WAIT_START = 0,
    U_TYPE,
    U_SEQ,
    U_LEN,
    U_PAYLOAD,
    U_CHECKSUM,
    U_END
};

static UartRxState u_state = U_WAIT_START;
static uint8_t u_pkt_type = 0;
static uint8_t u_seq_num = 0;
static uint8_t u_payload_len = 0;
static uint8_t u_payload_idx = 0;
static uint8_t u_payload[UART_MAX_PAYLOAD_LEN];
static uint8_t u_checksum = 0;

static void process_uart2_rx() {
    while (Serial2.available() > 0) {
        uint8_t b = (uint8_t)Serial2.read();

        switch (u_state) {
            case U_WAIT_START:
                if (b == UART_FRAME_START_BYTE) {
                    u_state = U_TYPE;
                }
                break;

            case U_TYPE:
                u_pkt_type = b;
                u_state = U_SEQ;
                break;

            case U_SEQ:
                u_seq_num = b;
                u_state = U_LEN;
                break;

            case U_LEN:
                u_payload_len = b;
                u_payload_idx = 0;
                if (u_payload_len > UART_MAX_PAYLOAD_LEN) {
                    u_state = U_WAIT_START;
                    uart_err_count++;
                } else if (u_payload_len == 0) {
                    u_state = U_CHECKSUM;
                } else {
                    u_state = U_PAYLOAD;
                }
                break;

            case U_PAYLOAD:
                u_payload[u_payload_idx++] = b;
                if (u_payload_idx >= u_payload_len) {
                    u_state = U_CHECKSUM;
                }
                break;

            case U_CHECKSUM:
                u_checksum = b;
                u_state = U_END;
                break;

            case U_END:
                if (b == UART_FRAME_END_BYTE) {
                    uint8_t expected_chk = calculate_xor_checksum(
                        u_pkt_type, u_seq_num, u_payload_len, u_payload
                    );
                    if (expected_chk == u_checksum) {
                        uart_rx_count++;
                        last_nano_rx_ms = millis();
                        nano_online = true;

                        // Nháy LED UART Activity
                        digitalWrite(PIN_LED_UART, HIGH);

                        if (u_pkt_type == PKT_TYPE_TELEMETRY && u_payload_len >= sizeof(NanoTelemetryPayload)) {
                            memcpy(&nano_data, u_payload, sizeof(NanoTelemetryPayload));
                        }
                    } else {
                        uart_err_count++;
                    }
                } else {
                    uart_err_count++;
                }
                u_state = U_WAIT_START;
                break;
        }
    }
}

// ============================================================================
// GIAO THỨC I2C: ĐỌC DỮ LIỆU TỪ STM32F4 COPROCESSOR
// ============================================================================
static void poll_stm32_coprocessor() {
    // 1. Kiểm tra WHO_AM_I & Trạng thái hệ thống
    Wire.beginTransmission(STM32_I2C_SLAVE_ADDR);
    Wire.write(I2C_REG_SYS_STATUS);
    if (Wire.endTransmission() == 0) {
        if (Wire.requestFrom((uint8_t)STM32_I2C_SLAVE_ADDR, (uint8_t)1) == 1) {
            stm32_status_reg = Wire.read();
            stm32_online = true;
            last_stm32_rx_ms = millis();
            digitalWrite(PIN_LED_I2C, HIGH);
        }
    } else {
        // Hết thời gian chờ phản hồi I2C
        if (millis() - last_stm32_rx_ms > 1500) {
            stm32_online = false;
        }
    }

    // 2. Nếu STM32 online, đọc vị trí bước hiện tại
    if (stm32_online) {
        Wire.beginTransmission(STM32_I2C_SLAVE_ADDR);
        Wire.write(I2C_REG_STEPPER_POS);
        if (Wire.endTransmission() == 0) {
            if (Wire.requestFrom((uint8_t)STM32_I2C_SLAVE_ADDR, (uint8_t)sizeof(int32_t)) == sizeof(int32_t)) {
                uint8_t* p = (uint8_t*)&stm32_stepper_pos;
                for (size_t i = 0; i < sizeof(int32_t); ++i) {
                    p[i] = Wire.read();
                }
            }
        }

        // 3. Đọc kết quả DSP Benchmark nếu có
        Wire.beginTransmission(STM32_I2C_SLAVE_ADDR);
        Wire.write(I2C_REG_DSP_RESULT);
        if (Wire.endTransmission() == 0) {
            if (Wire.requestFrom((uint8_t)STM32_I2C_SLAVE_ADDR, (uint8_t)sizeof(I2CDspResult)) == sizeof(I2CDspResult)) {
                I2CDspResult res;
                uint8_t* p = (uint8_t*)&res;
                for (size_t i = 0; i < sizeof(I2CDspResult); ++i) {
                    p[i] = Wire.read();
                }
                stm32_dsp_time_us = res.exec_time_us;
                stm32_dsp_hash = res.result_hash;
            }
        }
    }
}

// ============================================================================
// GIAO DIỆN WEBSOCKET & XỬ LÝ LỆNH TỪ TRÌNH DUYỆT (BROWSER CLIENT)
// ============================================================================
static void broadcast_telemetry() {
    if (webSocket.connectedClients() == 0) return;

    // Chuẩn bị chuỗi JSON gọn nhẹ truyền tải dữ liệu toàn hệ thống
    char json_buf[512];
    uint32_t uptime_sec = millis() / 1000;
    uint32_t free_heap = ESP.getFreeHeap();

    bool is_stepper_moving = (stm32_status_reg & STM32_STATUS_BIT_STEPPER_BUSY) != 0;

    snprintf(json_buf, sizeof(json_buf),
        "{\"master\":{\"uptime\":%u,\"heap\":%u,\"ip\":\"%s\",\"uart_errs\":%u},"
        "\"stm32\":{\"online\":%s,\"moving\":%s,\"pos\":%ld,\"dsp_time\":%u},"
        "\"nano\":{\"online\":%s,\"acs712\":%u,\"pot\":%u,\"relay\":%u,\"btn\":%u,\"vcc\":%u}}",
        uptime_sec, free_heap, WiFi.localIP().toString().c_str(), uart_err_count,
        stm32_online ? "true" : "false", is_stepper_moving ? "true" : "false", (long)stm32_stepper_pos, stm32_dsp_time_us,
        nano_online ? "true" : "false", nano_data.acs712_raw, nano_data.pot_raw,
        nano_data.relay_status, nano_data.button_status, nano_data.vcc_mv
    );

    webSocket.broadcastTXT(json_buf);
}

static void on_websocket_event(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_TEXT) {
        String msg = String((char*)payload);

        // 1. Lệnh điều khiển Relay (Nano)
        if (msg.indexOf("\"cmd\":\"relay\"") >= 0) {
            uint8_t state = (msg.indexOf("\"state\":1") >= 0) ? 1 : 0;
            RelayCmdPayload p;
            p.relay_index = 0;
            p.state = state;
            send_uart2_packet(PKT_TYPE_CMD_SET_RELAY, (const uint8_t*)&p, sizeof(p));
            Serial.printf("[MASTER] Sent Relay Cmd -> Nano: State %u\n", state);
        }
        // 2. Lệnh kích hoạt Buzzer (Nano)
        else if (msg.indexOf("\"cmd\":\"buzzer\"") >= 0) {
            BuzzerCmdPayload p;
            p.freq_hz = 2400;
            p.duration_ms = 80;
            send_uart2_packet(PKT_TYPE_CMD_TRIGGER_BUZZER, (const uint8_t*)&p, sizeof(p));
            Serial.println(F("[MASTER] Sent Buzzer Cmd -> Nano"));
        }
        // 3. Lệnh điều khiển Động cơ bước (STM32)
        else if (msg.indexOf("\"cmd\":\"stepper\"") >= 0) {
            uint8_t dir = (msg.indexOf("\"dir\":1") >= 0) ? 1 : 0;
            I2CStepperCmd cmd;
            cmd.direction = dir;
            cmd.speed_rpm = 15;
            cmd.step_count = 512; // 45 độ

            Wire.beginTransmission(STM32_I2C_SLAVE_ADDR);
            Wire.write(I2C_REG_STEPPER_CMD);
            Wire.write((const uint8_t*)&cmd, sizeof(cmd));
            Wire.endTransmission();
            Serial.printf("[MASTER] Sent Stepper Cmd -> STM32: Dir %u, Steps 512\n", dir);
        }
        // 4. Lệnh dừng khẩn cấp Động cơ bước (STM32)
        else if (msg.indexOf("\"cmd\":\"stepper_stop\"") >= 0) {
            Wire.beginTransmission(STM32_I2C_SLAVE_ADDR);
            Wire.write(I2C_REG_STEPPER_STOP);
            Wire.write((uint8_t)0x01);
            Wire.endTransmission();
            Serial.println(F("[MASTER] Sent Stepper Emergency Stop -> STM32"));
        }
        // 5. Lệnh chạy FPU DSP Benchmark (STM32)
        else if (msg.indexOf("\"cmd\":\"dsp_bench\"") >= 0) {
            Wire.beginTransmission(STM32_I2C_SLAVE_ADDR);
            Wire.write(I2C_REG_DSP_START);
            Wire.write((uint8_t)1);  // Algo: Float Matrix
            Wire.write((uint8_t)20); // 20 * 10 = 200 iterations
            Wire.endTransmission();
            Serial.println(F("[MASTER] Triggered FPU DSP Benchmark -> STM32"));
        }
    }
}

// ============================================================================
// HÀM KHỞI TẠO (SETUP)
// ============================================================================
void setup() {
    // 1. Khởi tạo cổng Serial Monitor chính (115200 baud)
    Serial.begin(115200);
    delay(500);

    Serial.println(F("\n========================================================"));
    Serial.println(F(" POC EXP-03: Distributed Multi-MCU Communication Lab"));
    Serial.println(F(" Node 1: ESP32 DevKit V1 (Master Gateway & Coordinator)"));
    Serial.println(F("========================================================"));

    // 2. Khởi tạo chân đèn LED báo trạng thái
    pinMode(PIN_LED_I2C, OUTPUT);
    pinMode(PIN_LED_UART, OUTPUT);
    pinMode(PIN_LED_SYS, OUTPUT);

    digitalWrite(PIN_LED_I2C, LOW);
    digitalWrite(PIN_LED_UART, LOW);
    digitalWrite(PIN_LED_SYS, HIGH);

    // 3. Khởi tạo bus I2C Master trên GPIO 21 & GPIO 22
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);
    Serial.println(F("[I2C] Master Bus Initialized @ 100kHz (SDA:21, SCL:22)"));

    // 4. Khởi tạo Hardware Serial2 giao tiếp với Arduino Nano
    Serial2.begin(115200, SERIAL_8N1, PIN_UART2_RX, PIN_UART2_TX);
    Serial.println(F("[UART2] Serial2 Initialized @ 115200bps (RX:16, TX:17)"));

    // 5. Thiết lập mạng Wi-Fi (SoftAP để người dùng kết nối trực tiếp)
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print(F("[WIFI] SoftAP Started: "));
    Serial.println(AP_SSID);
    Serial.print(F("[WIFI] Web Dashboard URL: http://"));
    Serial.println(WiFi.softAPIP());

    // 6. Cấu hình WebServer & WebSockets
    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", INDEX_HTML);
    });

    server.begin();
    webSocket.begin();
    webSocket.onEvent(on_websocket_event);
    Serial.println(F("[HTTP] Web Server & WebSocket Service Ready (Port 80/81)"));

    Serial.println(F("[READY] ESP32 Master Gateway Running!"));
}

// ============================================================================
// VÒNG LẶP CHÍNH (LOOP)
// ============================================================================
void loop() {
    unsigned long now_ms = millis();

    // 1. Phục vụ Web Server & WebSockets
    server.handleClient();
    webSocket.loop();

    // 2. Tiếp nhận và phân tích gói tin từ Arduino Nano (Serial2)
    process_uart2_rx();

    // Kiểm tra timeout kết nối Arduino Nano (> 1500ms không có gói tin)
    if (nano_online && (now_ms - last_nano_rx_ms > 1500)) {
        nano_online = false;
        Serial.println(F("[WARN] Arduino Nano Node is OFFLINE (UART Timeout)"));
    }

    // 3. Quét định kỳ STM32 Coprocessor qua bus I2C mỗi 100ms
    if (now_ms - last_i2c_poll_ms >= 100) {
        last_i2c_poll_ms = now_ms;
        poll_stm32_coprocessor();
    }

    // 4. Đẩy dữ liệu Telemetry tới tất cả Web Client qua WebSocket mỗi 100ms (10 Hz)
    if (now_ms - last_ws_broadcast_ms >= 100) {
        last_ws_broadcast_ms = now_ms;
        broadcast_telemetry();

        // Tắt đèn báo sau mỗi nhịp truyền nhận để tạo hiệu ứng chớp sáng
        digitalWrite(PIN_LED_I2C, LOW);
        digitalWrite(PIN_LED_UART, LOW);
    }

    // 5. Nhấp nháy đèn LED System mỗi 1000ms
    if (now_ms - last_heartbeat_ms >= 1000) {
        last_heartbeat_ms = now_ms;
        sys_led_state = !sys_led_state;
        digitalWrite(PIN_LED_SYS, sys_led_state ? HIGH : LOW);

        #if defined(WOKWI_SIMULATION)
        // Trong chế độ mô phỏng Wokwi không có phần cứng vật lý, tạo mock telemetry hợp lý
        if (!nano_online) {
            nano_data.pot_raw = (nano_data.pot_raw + 25) % 1024;
            nano_data.acs712_raw = 512 + (nano_data.pot_raw / 10);
            nano_online = true;
        }
        if (!stm32_online) {
            stm32_online = true;
            stm32_dsp_time_us = 184;
            stm32_dsp_hash = 0x5A3C;
        }
        #endif
    }
}
