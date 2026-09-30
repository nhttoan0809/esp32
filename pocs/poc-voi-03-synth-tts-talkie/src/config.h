#ifndef VOI03_CONFIG_H
#define VOI03_CONFIG_H

#include <Arduino.h>

// ==========================================
// 1. HARDWARE PIN DEFINITIONS (ESP32 DevKit V1 30-Pin)
// ==========================================

// Passive Piezo Buzzer: Nối GPIO 25 (Kênh phần cứng DAC1 / LEDC PWM)
#define PIN_BUZZER          25

// Biến trở xoay 10kΩ: Nối GPIO 34 (Kênh ADC1_CH6, Input-only)
#define PIN_POTENTIOMETER   34

// Màn hình LED 7 đoạn TM1637 (Giao thức 2 dây)
#define PIN_TM1637_CLK      4
#define PIN_TM1637_DIO      23

// Bàn phím ma trận 4x4 (Keypad Membrane)
// 4 chân Hàng (Rows) cấu hình làm OUTPUT (Active LOW scan)
#define PIN_KEYPAD_R1       13
#define PIN_KEYPAD_R2       14
#define PIN_KEYPAD_R3       27
#define PIN_KEYPAD_R4       26

// 4 chân Cột (Cols) cấu hình làm INPUT_PULLUP
#define PIN_KEYPAD_C1       18
#define PIN_KEYPAD_C2       19
#define PIN_KEYPAD_C3       21
#define PIN_KEYPAD_C4       22

// ==========================================
// 2. MUSICAL NOTES FREQUENCIES (Hz)
// ==========================================
#define NOTE_REST 0

// Octave 3
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247

// Octave 4 (Thang âm trung tâm)
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494

// Octave 5 (Thang âm cao)
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988

// Octave 6
#define NOTE_C6  1047
#define NOTE_D6  1175
#define NOTE_E6  1319
#define NOTE_G6  1568

// ==========================================
// 3. SYSTEM MODES (FINITE STATE MACHINE)
// ==========================================
enum SystemMode {
    MODE_ORGAN = 0,    // Đàn Organ điện tử 16 phím kèm Pitch Bend
    MODE_JUKEBOX,      // Máy phát nhạc retro chiptune kèm chỉnh Tempo BPM
    MODE_TALKIE,       // Bộ đọc giọng nói tổng hợp LPC Offline TTS
    MODE_SFX,          // Máy tạo hiệu ứng âm thanh game 8-bit
    MODE_COUNT         // Tổng số chế độ
};

// ==========================================
// 4. POTENTIOMETER & ADC CONSTANTS
// ==========================================
#define POT_ADC_RESOLUTION  12      // 12-bit ADC (0 - 4095)
#define POT_SMA_SAMPLES     8       // Bộ lọc trung bình trượt 8 mẫu
#define POT_DEADBAND        35      // Ngưỡng lọc rung nhiễu (Deadband)

// Pitch Bend Range (Hz)
#define PITCH_BEND_MIN     -50
#define PITCH_BEND_MAX      50

// Tempo Range (BPM)
#define TEMPO_MIN_BPM       60
#define TEMPO_MAX_BPM       240
#define TEMPO_DEFAULT_BPM   120

#endif // VOI03_CONFIG_H
