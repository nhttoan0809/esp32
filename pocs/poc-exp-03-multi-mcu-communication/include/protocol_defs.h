#ifndef PROTOCOL_DEFS_H
#define PROTOCOL_DEFS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 1. GIAO THỨC UART: ESP32 MASTER <-> ARDUINO NANO SUB-CONTROLLER
// ============================================================================
#define UART_FRAME_START_BYTE   0xAA
#define UART_FRAME_END_BYTE     0x55
#define UART_MAX_PAYLOAD_LEN    32

// Các loại mã gói tin (Packet Types)
#define PKT_TYPE_CMD_PING           0x05
#define PKT_TYPE_CMD_SET_RELAY      0x01
#define PKT_TYPE_CMD_TRIGGER_BUZZER 0x02

#define PKT_TYPE_RESP_ACK           0x80
#define PKT_TYPE_RESP_NACK          0x81
#define PKT_TYPE_TELEMETRY          0x10

#pragma pack(push, 1)

// Khung phần đầu gói tin (Header)
typedef struct {
    uint8_t start_byte;     // Luôn là 0xAA
    uint8_t packet_type;    // Loại gói tin
    uint8_t seq_num;        // Số thứ tự gói tin xoay vòng (0 - 255)
    uint8_t payload_len;    // Kích thước payload (N bytes)
} UartPacketHeader;

// Payload cho lệnh điều khiển Relay
typedef struct {
    uint8_t relay_index;    // 0: Relay 1 (Kênh chính)
    uint8_t state;          // 0: TẮT (NC), 1: BẬT (NO đóng)
} RelayCmdPayload;

// Payload cho lệnh kích hoạt Buzzer
typedef struct {
    uint16_t freq_hz;       // Tần số âm thanh (Hz)
    uint16_t duration_ms;   // Thời gian phát (ms)
} BuzzerCmdPayload;

// Payload Telemetry định kỳ gửi từ Arduino Nano sang ESP32
typedef struct {
    uint16_t acs712_raw;    // Giá trị ADC cảm biến dòng ACS712 (0 - 1023)
    uint16_t pot_raw;       // Giá trị ADC biến trở xoay 10k (0 - 1023)
    uint8_t  relay_status;  // Trạng thái relay hiện tại (bit 0: Relay 1)
    uint8_t  button_status; // Trạng thái nút bấm (bit 0: Btn 1, bit 1: Btn 2)
    uint16_t vcc_mv;        // Ước lượng điện áp nguồn 5V (mV)
} NanoTelemetryPayload;

// Payload phản hồi xác nhận ACK / NACK
typedef struct {
    uint8_t ack_seq;        // Số seq của gói tin được xác nhận
    uint8_t status_code;    // 0: Thành công (OK), 1+: Mã lỗi
} AckPayload;

#pragma pack(pop)

// Hàm tính XOR Checksum chung cho gói tin
static inline uint8_t calculate_xor_checksum(uint8_t packet_type, uint8_t seq_num, uint8_t len, const uint8_t* payload) {
    uint8_t checksum = packet_type ^ seq_num ^ len;
    for (uint8_t i = 0; i < len; ++i) {
        checksum ^= payload[i];
    }
    return checksum;
}

// ============================================================================
// 2. GIAO THỨC I2C: ESP32 MASTER <-> STM32F4 BLACK PILL COPROCESSOR
// ============================================================================
#define STM32_I2C_SLAVE_ADDR    0x42

// Danh mục địa chỉ thanh ghi (Register Addresses)
#define I2C_REG_WHO_AM_I        0x00 // Trả về 0x42
#define I2C_REG_SYS_STATUS      0x01 // Trả về Bitmask trạng thái hệ thống
#define I2C_REG_STEPPER_CMD     0x10 // Ghi lệnh điều khiển bước [Dir, RPM, Steps_H, Steps_L]
#define I2C_REG_STEPPER_STOP    0x14 // Ghi 0x01 để dừng khẩn cấp
#define I2C_REG_STEPPER_POS     0x15 // Đọc vị trí bước hiện tại (int32_t: 4 bytes)
#define I2C_REG_DSP_START       0x20 // Ghi lệnh bắt đầu benchmark FPU [Algo, Iterations]
#define I2C_REG_DSP_RESULT      0x24 // Đọc kết quả benchmark [Exec_Time_us: 4 bytes, Crc: 2 bytes]

// Bitmask cho thanh ghi I2C_REG_SYS_STATUS
#define STM32_STATUS_BIT_READY          (1 << 0) // Sẵn sàng
#define STM32_STATUS_BIT_STEPPER_BUSY   (1 << 1) // Động cơ đang quay
#define STM32_STATUS_BIT_DSP_BUSY       (1 << 2) // FPU đang tính toán
#define STM32_STATUS_BIT_ERROR          (1 << 3) // Lỗi phần cứng hoặc tham số

#pragma pack(push, 1)

// Cấu trúc lệnh gửi tới động cơ bước qua I2C
typedef struct {
    uint8_t  direction;     // 0: Chiều kim đồng hồ (CW), 1: Ngược chiều (CCW)
    uint8_t  speed_rpm;     // Tốc độ mong muốn (ví dụ: 10 - 20 RPM)
    uint16_t step_count;    // Số bước cần dịch chuyển (uint16_t)
} I2CStepperCmd;

// Cấu trúc đọc kết quả FPU Benchmark từ STM32
typedef struct {
    uint32_t exec_time_us;  // Thời gian thực thi phép toán (microsecond)
    uint16_t result_hash;   // Hash kết quả kiểm tra tính toàn vẹn
} I2CDspResult;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_DEFS_H
