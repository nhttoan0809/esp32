#include "keypad_driver.h"

KeypadDriver::KeypadDriver()
    : _rowPins{PIN_KEYPAD_R1, PIN_KEYPAD_R2, PIN_KEYPAD_R3, PIN_KEYPAD_R4},
      _colPins{PIN_KEYPAD_C1, PIN_KEYPAD_C2, PIN_KEYPAD_C3, PIN_KEYPAD_C4},
      _keyMap{
          {'1', '2', '3', 'A'},
          {'4', '5', '6', 'B'},
          {'7', '8', '9', 'C'},
          {'*', '0', '#', 'D'}
      },
      _currentKey('\0'),
      _lastDetectedKey('\0'),
      _pressedKeyBuffer('\0'),
      _state(KEY_STATE_IDLE),
      _lastDebounceTime(0),
      _pressStartTime(0) {}

void KeypadDriver::begin() {
    // Khởi tạo các chân Hàng (Rows) làm OUTPUT và đặt mức HIGH
    for (uint8_t r = 0; r < ROWS; r++) {
        pinMode(_rowPins[r], OUTPUT);
        digitalWrite(_rowPins[r], HIGH);
    }

    // Khởi tạo các chân Cột (Cols) làm INPUT_PULLUP
    for (uint8_t c = 0; c < COLS; c++) {
        pinMode(_colPins[c], INPUT_PULLUP);
    }
}

char KeypadDriver::scanMatrix() {
    for (uint8_t r = 0; r < ROWS; r++) {
        // Kéo hàng r xuống LOW để quét
        digitalWrite(_rowPins[r], LOW);
        delayMicroseconds(5); // Ổn định điện áp trên đường mạch

        for (uint8_t c = 0; c < COLS; c++) {
            if (digitalRead(_colPins[c]) == LOW) {
                // Phím tại hàng r, cột c được nhấn
                digitalWrite(_rowPins[r], HIGH); // Khôi phục hàng
                return _keyMap[r][c];
            }
        }

        // Khôi phục hàng r về HIGH
        digitalWrite(_rowPins[r], HIGH);
    }

    return '\0'; // Không có phím nào được nhấn
}

void KeypadDriver::update() {
    char detected = scanMatrix();
    uint32_t now = millis();

    // Thuật toán khử rung (Debouncing State Machine)
    if (detected != _lastDetectedKey) {
        _lastDebounceTime = now;
        _lastDetectedKey = detected;
        return;
    }

    // Khi tín hiệu ổn định vượt quá 20ms
    if ((now - _lastDebounceTime) > 20) {
        if (detected != _currentKey) {
            if (detected != '\0') {
                // Sự kiện phím mới được nhấn xuống
                _currentKey = detected;
                _state = KEY_STATE_PRESSED;
                _pressedKeyBuffer = detected;
                _pressStartTime = now;
            } else {
                // Sự kiện phím vừa được nhả ra
                _currentKey = '\0';
                _state = KEY_STATE_RELEASED;
            }
        } else {
            if (_currentKey != '\0') {
                // Phím vẫn đang tiếp tục được giữ
                if ((now - _pressStartTime) > 400) {
                    _state = KEY_STATE_HOLD;
                }
            } else {
                _state = KEY_STATE_IDLE;
            }
        }
    }
}

char KeypadDriver::getKey() {
    char key = _pressedKeyBuffer;
    _pressedKeyBuffer = '\0'; // Xóa buffer sau khi đọc một lần
    return key;
}

bool KeypadDriver::isKeyPressed(char key) {
    return (_currentKey == key && (_state == KEY_STATE_PRESSED || _state == KEY_STATE_HOLD));
}

bool KeypadDriver::isAnyKeyDown() const {
    return (_currentKey != '\0' && (_state == KEY_STATE_PRESSED || _state == KEY_STATE_HOLD));
}

char KeypadDriver::getCurrentKey() const {
    return _currentKey;
}

KeyState KeypadDriver::getKeyState() const {
    return _state;
}
