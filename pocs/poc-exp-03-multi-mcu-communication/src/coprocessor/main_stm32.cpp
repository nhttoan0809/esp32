/**
 * @file main_stm32.cpp
 * @brief Firmware for STM32F4 Black Pill (ARM Cortex-M4) - Math & Motion Coprocessor
 * @project EXP-03 Distributed Multi-MCU Communication Lab
 */

#include <Arduino.h>
#include <Wire.h>
#include "../../include/protocol_defs.h"

// ============================================================================
// ĐỊNH NGHĨA CHÂN NGOẠI VI (PINOUT STM32F4 BLACK PILL)
// ============================================================================
#define PIN_LED_BUILDIN     PC13    // Đèn LED tích hợp (Active LOW: LOW = Sáng, HIGH = Tắt)

// Chân giao tiếp I2C1 (Nối với ESP32 GPIO 21 & GPIO 22, mức logic 3.3V)
#define PIN_I2C_SDA         PB7
#define PIN_I2C_SCL         PB6

// Chân điều khiển 4 pha Động cơ bước 28BYJ-48 qua IC đệm ULN2003
#define PIN_STEP_IN1        PA0
#define PIN_STEP_IN2        PA1
#define PIN_STEP_IN3        PA2
#define PIN_STEP_IN4        PA3

// ============================================================================
// BẢNG BƯỚC NỬA BƯỚC (HALF-STEPPING SEQUENCE - 8 BƯỚC)
// ============================================================================
static const uint8_t HALF_STEP_SEQ[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

// ============================================================================
// BIẾN TOÀN CỤC & THANH GHI I2C
// ============================================================================
static volatile uint8_t  current_i2c_reg = I2C_REG_WHO_AM_I;
static volatile uint8_t  sys_status = STM32_STATUS_BIT_READY;

// Biến điều khiển động cơ bước
static volatile int32_t  stepper_current_pos = 0;
static volatile int32_t  stepper_target_pos = 0;
static volatile uint32_t step_interval_us = 1200; // Tương đương ~12 RPM
static volatile bool     stepper_running = false;
static uint8_t           stepper_step_idx = 0;
static unsigned long     last_step_us = 0;

// Biến kết quả FPU Benchmark
static I2CDspResult      dsp_result = {0, 0};
static volatile bool     dsp_trigger_flag = false;
static volatile uint8_t  dsp_algo_type = 1;
static volatile uint16_t dsp_iterations = 200;

// Bộ đếm nhịp tim
static unsigned long     last_heartbeat_ms = 0;
static bool              hb_state = false;

// ============================================================================
// HÀM ĐIỀU KHIỂN CÁC PHA ULN2003
// ============================================================================
static void set_stepper_phases(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    digitalWrite(PIN_STEP_IN1, a ? HIGH : LOW);
    digitalWrite(PIN_STEP_IN2, b ? HIGH : LOW);
    digitalWrite(PIN_STEP_IN3, c ? HIGH : LOW);
    digitalWrite(PIN_STEP_IN4, d ? HIGH : LOW);
}

static void stop_stepper_coils() {
    set_stepper_phases(0, 0, 0, 0);
    stepper_running = false;
    sys_status &= ~STM32_STATUS_BIT_STEPPER_BUSY;
}

// ============================================================================
// HÀM CHẠY THUẬT TOÁN FPU DSP BENCHMARK (ARM CORTEX-M4 FPU)
// ============================================================================
static void execute_dsp_benchmark(uint8_t algo, uint16_t iters) {
    sys_status |= STM32_STATUS_BIT_DSP_BUSY;

    unsigned long start_us = micros();
    float accumulator = 1.0001f;
    float coeff = 0.9998f;

    // Vòng lặp tính toán số thực dấu chấm động 32-bit (Single-Precision FPU)
    for (uint16_t i = 0; i < iters; ++i) {
        float x = (float)i * 0.0314159f;
        float sin_approx = x - (x * x * x) / 6.0f + (x * x * x * x * x) / 120.0f;
        accumulator = (accumulator * coeff) + (sin_approx * 0.5f);
    }

    unsigned long end_us = micros();
    uint32_t elapsed_us = (uint32_t)(end_us - start_us);

    dsp_result.exec_time_us = elapsed_us;
    dsp_result.result_hash  = (uint16_t)(*(uint32_t*)&accumulator & 0xFFFF);

    sys_status &= ~STM32_STATUS_BIT_DSP_BUSY;

    // In log chẩn đoán ra kênh Serial1 (PA9/PA10) kết nối tới module HW-896
    Serial1.print(F("[STM32 FPU] Algo:"));
    Serial1.print(algo);
    Serial1.print(F(" | Iters:"));
    Serial1.print(iters);
    Serial1.print(F(" | Time:"));
    Serial1.print(elapsed_us);
    Serial1.print(F(" us | Hash:0x"));
    Serial1.println(dsp_result.result_hash, HEX);
}

// ============================================================================
// XỬ LÝ NGẮT I2C SLAVE: ON RECEIVE (MASTER GHI VÀO SLAVE)
// ============================================================================
static void on_i2c_receive(int count) {
    if (count < 1) return;

    uint8_t reg = Wire.read();
    current_i2c_reg = reg;
    int remaining = count - 1;

    switch (reg) {
        case I2C_REG_STEPPER_CMD: {
            if (remaining >= (int)sizeof(I2CStepperCmd)) {
                I2CStepperCmd cmd;
                uint8_t* p = (uint8_t*)&cmd;
                for (size_t i = 0; i < sizeof(I2CStepperCmd); ++i) {
                    p[i] = Wire.read();
                }

                // Tính toán vị trí đích
                int32_t steps = (int32_t)cmd.step_count;
                if (cmd.direction == 1) {
                    stepper_target_pos = stepper_current_pos - steps;
                } else {
                    stepper_target_pos = stepper_current_pos + steps;
                }

                // Tính step interval theo RPM (giả định 4096 half-steps/vòng)
                uint8_t rpm = (cmd.speed_rpm == 0) ? 12 : cmd.speed_rpm;
                // interval_us = 60 * 1,000,000 / (4096 * rpm)
                step_interval_us = (uint32_t)(60000000UL / (4096UL * (uint32_t)rpm));
                if (step_interval_us < 800) step_interval_us = 800; // Giới hạn tốc độ an toàn

                stepper_running = true;
                sys_status |= STM32_STATUS_BIT_STEPPER_BUSY;
            }
            break;
        }

        case I2C_REG_STEPPER_STOP: {
            if (remaining >= 1) {
                Wire.read(); // Đọc giá trị
            }
            stop_stepper_coils();
            break;
        }

        case I2C_REG_DSP_START: {
            if (remaining >= 2) {
                dsp_algo_type = Wire.read();
                uint8_t iter_byte = Wire.read();
                dsp_iterations = (iter_byte == 0) ? 200 : ((uint16_t)iter_byte * 10);
                dsp_trigger_flag = true;
            }
            break;
        }

        default:
            // Bỏ qua các byte thừa nếu có
            while (Wire.available()) {
                Wire.read();
            }
            break;
    }
}

// ============================================================================
// XỬ LÝ NGẮT I2C SLAVE: ON REQUEST (MASTER ĐỌC TỪ SLAVE)
// ============================================================================
static void on_i2c_request() {
    switch (current_i2c_reg) {
        case I2C_REG_WHO_AM_I:
            Wire.write((uint8_t)STM32_I2C_SLAVE_ADDR);
            break;

        case I2C_REG_SYS_STATUS:
            Wire.write((uint8_t)sys_status);
            break;

        case I2C_REG_STEPPER_POS: {
            int32_t pos = stepper_current_pos;
            Wire.write((const uint8_t*)&pos, sizeof(pos));
            break;
        }

        case I2C_REG_DSP_RESULT:
            Wire.write((const uint8_t*)&dsp_result, sizeof(dsp_result));
            break;

        default:
            Wire.write((uint8_t)0xFF);
            break;
    }
}

// ============================================================================
// HÀM KHỞI TẠO (SETUP)
// ============================================================================
void setup() {
    // 1. Cấu hình cổng debug Serial1 trên PA9 (TX) và PA10 (RX) cho mạch nạp HW-896
    Serial1.begin(115200);
    Serial1.println(F("\n============================================="));
    Serial1.println(F("[STM32F4] Coprocessor Node Initializing..."));
    Serial1.println(F("============================================="));

    // 2. Cấu hình đèn LED tích hợp trên bo
    pinMode(PIN_LED_BUILDIN, OUTPUT);
    digitalWrite(PIN_LED_BUILDIN, HIGH); // Mặc định tắt LED (Active LOW)

    // 3. Cấu hình chân điều khiển động cơ bước
    pinMode(PIN_STEP_IN1, OUTPUT);
    pinMode(PIN_STEP_IN2, OUTPUT);
    pinMode(PIN_STEP_IN3, OUTPUT);
    pinMode(PIN_STEP_IN4, OUTPUT);
    stop_stepper_coils();

    // 4. Khởi tạo bus I2C Slave trên PB6 / PB7
    #if defined(PIN_I2C_SDA) && defined(PIN_I2C_SCL)
    Wire.setSDA(PIN_I2C_SDA);
    Wire.setSCL(PIN_I2C_SCL);
    #endif
    Wire.begin(STM32_I2C_SLAVE_ADDR);
    Wire.onReceive(on_i2c_receive);
    Wire.onRequest(on_i2c_request);

    sys_status = STM32_STATUS_BIT_READY;

    Serial1.println(F("[STM32F4] I2C Slave Ready @ Address 0x42"));
}

// ============================================================================
// VÒNG LẶP CHÍNH (LOOP)
// ============================================================================
void loop() {
    unsigned long now_us = micros();
    unsigned long now_ms = millis();

    // 1. Điều khiển bước động cơ 28BYJ-48 (Non-blocking)
    if (stepper_running && (now_us - last_step_us >= step_interval_us)) {
        last_step_us = now_us;

        if (stepper_current_pos < stepper_target_pos) {
            stepper_step_idx = (stepper_step_idx + 1) & 0x07;
            stepper_current_pos++;
            set_stepper_phases(
                HALF_STEP_SEQ[stepper_step_idx][0],
                HALF_STEP_SEQ[stepper_step_idx][1],
                HALF_STEP_SEQ[stepper_step_idx][2],
                HALF_STEP_SEQ[stepper_step_idx][3]
            );
        } else if (stepper_current_pos > stepper_target_pos) {
            stepper_step_idx = (stepper_step_idx + 7) & 0x07;
            stepper_current_pos--;
            set_stepper_phases(
                HALF_STEP_SEQ[stepper_step_idx][0],
                HALF_STEP_SEQ[stepper_step_idx][1],
                HALF_STEP_SEQ[stepper_step_idx][2],
                HALF_STEP_SEQ[stepper_step_idx][3]
            );
        } else {
            // Đã đạt vị trí mong muốn -> Tắt cuộn dây tránh nóng động cơ
            stop_stepper_coils();
        }
    }

    // 2. Thực hiện tác vụ tính toán FPU ngoài ngắt
    if (dsp_trigger_flag) {
        dsp_trigger_flag = false;
        execute_dsp_benchmark(dsp_algo_type, dsp_iterations);
    }

    // 3. Nhấp nháy LED nhịp tim mỗi 500ms
    if (now_ms - last_heartbeat_ms >= 500) {
        last_heartbeat_ms = now_ms;
        hb_state = !hb_state;
        digitalWrite(PIN_LED_BUILDIN, hb_state ? LOW : HIGH); // Active LOW
    }
}
