/**
 * display_matrix.cpp
 * Hiện thực Driver MAX7219 qua phần cứng SPI của ESP32 và hoạt họa cử chỉ
 */

#include "display_matrix.h"

// Thanh ghi MAX7219
#define REG_NOOP        0x00
#define REG_DIGIT0      0x01
#define REG_DECODE_MODE 0x09
#define REG_INTENSITY   0x0A
#define REG_SCAN_LIMIT  0x0B
#define REG_SHUTDOWN    0x0C
#define REG_DISPLAY_TEST 0x0F

static const uint8_t PROGMEM ICON_LAMP_ON[8] = {
    0b00111100,
    0b01000010,
    0b01011010,
    0b01111110,
    0b00111100,
    0b00111100,
    0b00011000,
    0b00011000
};

static const uint8_t PROGMEM ICON_LAMP_OFF[8] = {
    0b00111100,
    0b01000010,
    0b01000010,
    0b01000010,
    0b00100100,
    0b00111100,
    0b00011000,
    0b00011000
};

static const uint8_t PROGMEM ICON_FAN_FRAME1[8] = {
    0b00011000,
    0b00011000,
    0b00011000,
    0b11111111,
    0b11111111,
    0b00011000,
    0b00011000,
    0b00011000
};

static const uint8_t PROGMEM ICON_FAN_FRAME2[8] = {
    0b11000011,
    0b01100110,
    0b00111100,
    0b00011000,
    0b00011000,
    0b00111100,
    0b01100110,
    0b11000011
};

DisplayMatrix::DisplayMatrix()
    : currentAnimation(GESTURE_IDLE),
      animStartTime(0),
      lastFrameTime(0),
      animStep(0),
      currentLampState(false),
      currentFanState(false) {
    for (uint8_t i = 0; i < 8; i++) {
        displayBuffer[i] = 0;
    }
}

void DisplayMatrix::writeRegister(uint8_t reg, uint8_t data) {
    digitalWrite(MATRIX_CS_PIN, LOW);
    SPI.transfer(reg);
    SPI.transfer(data);
    digitalWrite(MATRIX_CS_PIN, HIGH);
}

void DisplayMatrix::begin() {
    pinMode(MATRIX_CS_PIN, OUTPUT);
    digitalWrite(MATRIX_CS_PIN, HIGH);

    // Khởi tạo SPI: SCK=18, MISO=-1 (không dùng), MOSI=23, SS=5
    SPI.begin(MATRIX_CLK_PIN, -1, MATRIX_DIN_PIN, MATRIX_CS_PIN);
    SPI.setFrequency(10000000); // 10 MHz SPI Clock

    writeRegister(REG_DISPLAY_TEST, 0x00); // Test OFF
    writeRegister(REG_DECODE_MODE, 0x00);  // No decode (Raw bit matrix)
    writeRegister(REG_SCAN_LIMIT, 0x07);   // Scan all 8 rows (0..7)
    writeRegister(REG_INTENSITY, 0x05);    // Mức sáng 5 (0..15)
    writeRegister(REG_SHUTDOWN, 0x01);     // Bật màn hình (Normal operation)

    clear();
    renderIdle();
    Serial.println(F("[MATRIX] MAX7219 8x8 Hardware SPI driver initialized."));
}

void DisplayMatrix::setIntensity(uint8_t intensity) {
    if (intensity > 15) intensity = 15;
    writeRegister(REG_INTENSITY, intensity);
}

void DisplayMatrix::clear() {
    for (uint8_t r = 0; r < 8; r++) {
        displayBuffer[r] = 0;
        writeRegister(REG_DIGIT0 + r, 0x00);
    }
}

void DisplayMatrix::setRow(uint8_t row, uint8_t value) {
    if (row < 8) {
        displayBuffer[row] = value;
        writeRegister(REG_DIGIT0 + row, value);
    }
}

void DisplayMatrix::setLed(uint8_t row, uint8_t col, bool state) {
    if (row < 8 && col < 8) {
        if (state) {
            displayBuffer[row] |= (1 << (7 - col));
        } else {
            displayBuffer[row] &= ~(1 << (7 - col));
        }
        writeRegister(REG_DIGIT0 + row, displayBuffer[row]);
    }
}

void DisplayMatrix::showPattern(const uint8_t rows[8]) {
    for (uint8_t r = 0; r < 8; r++) {
        setRow(r, rows[r]);
    }
}

void DisplayMatrix::triggerAnimation(GestureType gesture, bool lampState, bool fanState) {
    currentAnimation = gesture;
    currentLampState = lampState;
    currentFanState = fanState;
    animStartTime = millis();
    lastFrameTime = millis();
    animStep = 0;
    update();
}

void DisplayMatrix::update() {
    unsigned long now = millis();

    if (currentAnimation == GESTURE_IDLE) {
        // Cập nhật idle định kỳ
        if (now - lastFrameTime > 2000) {
            lastFrameTime = now;
            renderIdle();
        }
        return;
    }

    // Kết thúc hoạt họa sau 850ms
    if (now - animStartTime > 850) {
        currentAnimation = GESTURE_IDLE;
        renderIdle();
        return;
    }

    switch (currentAnimation) {
        case GESTURE_SWIPE_RIGHT:
            if (now - lastFrameTime >= 65) {
                lastFrameTime = now;
                renderSwipeRight(animStep++);
                if (animStep > 8) animStep = 0;
            }
            break;

        case GESTURE_SWIPE_LEFT:
            if (now - lastFrameTime >= 65) {
                lastFrameTime = now;
                renderSwipeLeft(animStep++);
                if (animStep > 8) animStep = 0;
            }
            break;

        case GESTURE_DOUBLE_TAP:
            renderLamp(currentLampState);
            break;

        case GESTURE_HOLD:
            if (now - lastFrameTime >= 90) {
                lastFrameTime = now;
                renderFan(animStep++);
            }
            break;

        default:
            break;
    }
}

void DisplayMatrix::renderIdle() {
    clear();
    // 4 chấm trung tâm
    setLed(3, 3, true);
    setLed(3, 4, true);
    setLed(4, 3, true);
    setLed(4, 4, true);

    // Chỉ báo trạng thái Đèn (Góc trên trái)
    if (currentLampState) {
        setLed(0, 0, true);
        setLed(0, 1, true);
    }
    // Chỉ báo trạng thái Quạt (Góc trên phải)
    if (currentFanState) {
        setLed(0, 6, true);
        setLed(0, 7, true);
    }
}

void DisplayMatrix::renderSwipeRight(uint8_t step) {
    clear();
    int offset = (int)step - 4;
    for (int r = 0; r < 8; r++) {
        uint8_t arrow = 0;
        if (r == 3 || r == 4) {
            arrow = 0b11111111;
        } else if (r == 2 || r == 5) {
            arrow = 0b00111100;
        } else if (r == 1 || r == 6) {
            arrow = 0b00011000;
        } else {
            arrow = 0b00001000;
        }

        uint8_t shifted = 0;
        if (offset >= 0) {
            shifted = arrow >> offset;
        } else {
            shifted = arrow << (-offset);
        }
        setRow(r, shifted);
    }
}

void DisplayMatrix::renderSwipeLeft(uint8_t step) {
    clear();
    int offset = (int)step - 4;
    for (int r = 0; r < 8; r++) {
        uint8_t arrow = 0;
        if (r == 3 || r == 4) {
            arrow = 0b11111111;
        } else if (r == 2 || r == 5) {
            arrow = 0b00111100;
        } else if (r == 1 || r == 6) {
            arrow = 0b00011000;
        } else {
            arrow = 0b00010000;
        }

        uint8_t shifted = 0;
        if (offset >= 0) {
            shifted = arrow << offset;
        } else {
            shifted = arrow >> (-offset);
        }
        setRow(r, shifted);
    }
}

void DisplayMatrix::renderLamp(bool isOn) {
    uint8_t icon[8];
    for (int r = 0; r < 8; r++) {
        icon[r] = isOn ? pgm_read_byte(&ICON_LAMP_ON[r]) : pgm_read_byte(&ICON_LAMP_OFF[r]);
    }
    showPattern(icon);
}

void DisplayMatrix::renderFan(uint8_t frame) {
    uint8_t icon[8];
    const uint8_t* src = (frame % 2 == 0) ? ICON_FAN_FRAME1 : ICON_FAN_FRAME2;
    for (int r = 0; r < 8; r++) {
        icon[r] = pgm_read_byte(&src[r]);
    }
    showPattern(icon);
}
