#ifndef KEYPAD_HANDLER_H
#define KEYPAD_HANDLER_H

#include <Arduino.h>
#include <Keypad.h>

class KeypadHandler {
public:
    KeypadHandler();
    void begin();
    void update();

    bool hasSubmittedPin();
    String getSubmittedPin();

    bool isTyping() const;
    String getMaskedPin() const;
    int getPinLength() const;
    void reset();

private:
    char keys[4][4];
    byte rowPins[4];
    byte colPins[4];
    Keypad keypad;

    char pinBuffer[16];
    int pinIndex;
    bool pinSubmitted;
    unsigned long lastInputTime;
};

extern KeypadHandler keypadHandler;

#endif // KEYPAD_HANDLER_H
