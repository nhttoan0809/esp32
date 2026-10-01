/**
 * display_matrix.h
 * Điều khiển màn hình LED ma trận 8x8 MAX7219 qua phần cứng SPI chuẩn của ESP32
 * Không phụ thuộc thư viện bên ngoài (Zero-dependency driver)
 */

#ifndef DISPLAY_MATRIX_H
#define DISPLAY_MATRIX_H

#include "config.h"
#include <SPI.h>

class DisplayMatrix {
public:
    DisplayMatrix();
    void begin();
    void update(); // Cập nhật khung hình hoạt họa non-blocking

    void triggerAnimation(GestureType gesture, bool lampState, bool fanState);
    void showPattern(const uint8_t rows[8]);
    void clear();

    void setRow(uint8_t row, uint8_t value);
    void setLed(uint8_t row, uint8_t col, bool state);
    void setIntensity(uint8_t intensity); // 0 .. 15

private:
    GestureType currentAnimation;
    unsigned long animStartTime;
    unsigned long lastFrameTime;
    uint8_t animStep;
    bool currentLampState;
    bool currentFanState;
    uint8_t displayBuffer[8];

    void writeRegister(uint8_t reg, uint8_t data);
    void renderIdle();
    void renderSwipeRight(uint8_t step);
    void renderSwipeLeft(uint8_t step);
    void renderLamp(bool isOn);
    void renderFan(uint8_t frame);
};

#endif // DISPLAY_MATRIX_H
