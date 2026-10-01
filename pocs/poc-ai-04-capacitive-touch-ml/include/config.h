/**
 * config.h
 * Cấu hình phần cứng, chân GPIO và tham số hệ thống cho POC AI-04
 * Bàn Cảm Ứng Nhận Diện Cử Chỉ Bằng Machine Learning (ESP32 DevKit V1 30-pin)
 */

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>

// --- Định cấu hình chân Touch Pins (ESP32 30-pin) ---
// Tránh các chân Strapping (GPIO 0, 2, 12, 15)
#define TOUCH_PIN_0   4   // T0 - Pad 0 (Leftmost)
#define TOUCH_PIN_1   13  // T4 - Pad 1 (Mid-Left)
#define TOUCH_PIN_2   14  // T6 - Pad 2 (Mid-Right)
#define TOUCH_PIN_3   27  // T7 - Pad 3 (Rightmost)
#define NUM_TOUCH_PADS 4

// --- Định cấu hình chân giao tiếp MAX7219 (VSPI) ---
#define MATRIX_DIN_PIN 23  // VSPI MOSI
#define MATRIX_CLK_PIN 18  // VSPI SCK
#define MATRIX_CS_PIN  5   // VSPI CS
#define MATRIX_DEVICES 1   // 1 module ma trận 8x8

// --- Định cấu hình Module Relay 2 Kênh 5V (Active LOW) ---
#define RELAY_CH1_PIN 25  // Kênh 1: Đèn bàn (Desk Lamp)
#define RELAY_CH2_PIN 26  // Kênh 2: Quạt bàn (Desk Fan)
#define RELAY_ACTIVE_LEVEL LOW
#define RELAY_INACTIVE_LEVEL HIGH

// --- Đèn LED trạng thái trên bo mạch ---
#define STATUS_LED_PIN 2

// --- Tham số Lấy mẫu & Trích xuất Đặc trưng ---
#define SAMPLING_RATE_HZ   50    // 50 Hz = chu kỳ 20ms
#define SAMPLING_PERIOD_MS (1000 / SAMPLING_RATE_HZ)
#define WINDOW_SIZE        20    // 20 frames = 400ms cửa sổ quan sát
#define CALIBRATION_CYCLES 50    // 50 mẫu hiệu chuẩn baseline khi boot
#define TOUCH_DELTA_MIN    15    // Ngưỡng tối thiểu tính là có chạm (Delta)
#define GESTURE_COOLDOWN_MS 450  // Thời gian chặn re-trigger sau khi nhận diện cử chỉ

// --- Phân loại Cử chỉ (Gesture Types) ---
enum GestureType {
    GESTURE_IDLE = 0,
    GESTURE_SWIPE_RIGHT = 1,
    GESTURE_SWIPE_LEFT = 2,
    GESTURE_DOUBLE_TAP = 3,
    GESTURE_HOLD = 4
};

#endif // APP_CONFIG_H
