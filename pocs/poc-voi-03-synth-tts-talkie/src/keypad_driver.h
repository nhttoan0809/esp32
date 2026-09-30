#ifndef VOI03_KEYPAD_DRIVER_H
#define VOI03_KEYPAD_DRIVER_H

#include <Arduino.h>
#include "config.h"

enum KeyState {
    KEY_STATE_IDLE = 0,
    KEY_STATE_PRESSED,
    KEY_STATE_HOLD,
    KEY_STATE_RELEASED
};

struct KeyEvent {
    char key;
    KeyState state;
};

class KeypadDriver {
public:
    KeypadDriver();

    void begin();
    void update(); // Quét phím định kỳ non-blocking

    char getKey(); // Trả về phím vừa được nhấn (chỉ 1 lần per press), hoặc '\0'
    bool isKeyPressed(char key);
    bool isAnyKeyDown() const;
    char getCurrentKey() const;
    KeyState getKeyState() const;

private:
    static const uint8_t ROWS = 4;
    static const uint8_t COLS = 4;

    uint8_t _rowPins[ROWS];
    uint8_t _colPins[COLS];
    char _keyMap[ROWS][COLS];

    char _currentKey;
    char _lastDetectedKey;
    char _pressedKeyBuffer;
    KeyState _state;
    uint32_t _lastDebounceTime;
    uint32_t _pressStartTime;

    char scanMatrix();
};

#endif // VOI03_KEYPAD_DRIVER_H
