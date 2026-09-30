#include "keypad_handler.h"
#include "config.h"
#include "feedback.h"

KeypadHandler keypadHandler;

KeypadHandler::KeypadHandler()
    : keys{
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
      },
      rowPins{PIN_KEYPAD_R1, PIN_KEYPAD_R2, PIN_KEYPAD_R3, PIN_KEYPAD_R4},
      colPins{PIN_KEYPAD_C1, PIN_KEYPAD_C2, PIN_KEYPAD_C3, PIN_KEYPAD_C4},
      keypad(makeKeymap(keys), rowPins, colPins, 4, 4),
      pinIndex(0),
      pinSubmitted(false),
      lastInputTime(0) {
    memset(pinBuffer, 0, sizeof(pinBuffer));
}

void KeypadHandler::begin() {
    reset();
}

void KeypadHandler::reset() {
    pinIndex = 0;
    memset(pinBuffer, 0, sizeof(pinBuffer));
    pinSubmitted = false;
    lastInputTime = 0;
}

bool KeypadHandler::isTyping() const {
    return (pinIndex > 0);
}

int KeypadHandler::getPinLength() const {
    return pinIndex;
}

String KeypadHandler::getMaskedPin() const {
    String masked = "";
    for (int i = 0; i < pinIndex; i++) {
        masked += "*";
    }
    return masked;
}

bool KeypadHandler::hasSubmittedPin() {
    return pinSubmitted;
}

String KeypadHandler::getSubmittedPin() {
    if (!pinSubmitted) return "";
    String pinStr = String(pinBuffer);
    reset();
    return pinStr;
}

void KeypadHandler::update() {
    char key = keypad.getKey();

    if (key != NO_KEY) {
        lastInputTime = millis();

        if (key >= '0' && key <= '9') {
            if (pinIndex < (int)sizeof(pinBuffer) - 1) {
                pinBuffer[pinIndex++] = key;
                pinBuffer[pinIndex] = '\0';
                feedback.beepShort();
            }
        } else if (key == '*') {
            // Cancel or Backspace
            if (pinIndex > 0) {
                pinIndex--;
                pinBuffer[pinIndex] = '\0';
            }
            feedback.beepShort();
        } else if (key == '#') {
            // Confirm / Submit PIN
            if (pinIndex > 0) {
                pinSubmitted = true;
                feedback.beepShort();
            }
        } else {
            // Special function keys (A, B, C, D)
            feedback.beepShort();
        }
    }

    // Auto-timeout if inactive while typing PIN
    if (pinIndex > 0 && !pinSubmitted) {
        if (millis() - lastInputTime >= PIN_INPUT_TIMEOUT_MS) {
            reset();
            feedback.beepShort();
        }
    }
}
