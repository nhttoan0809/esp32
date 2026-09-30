#ifndef FEEDBACK_H
#define FEEDBACK_H

#include <Arduino.h>

class Feedback {
public:
    Feedback();
    void begin();
    void update();

    void beepShort();
    void beepSuccess();
    void beepDenied();
    void startAlarm();
    void stopAlarm();

    void setUnlockLed(bool on);
    void blinkUnlockLed(int count, unsigned long intervalMs = 100);

private:
    enum SoundPattern {
        PATTERN_NONE,
        PATTERN_SHORT,
        PATTERN_SUCCESS,
        PATTERN_DENIED,
        PATTERN_ALARM
    };

    SoundPattern currentPattern;
    unsigned long patternStartTime;
    int patternStep;
    bool buzzerState;
    bool alarmActive;

    // Non-blocking LED blink state
    bool ledBlinking;
    int ledBlinkRemaining;
    unsigned long ledBlinkInterval;
    unsigned long lastLedToggleTime;
    bool currentLedState;

    void setBuzzer(bool on);
};

extern Feedback feedback;

#endif // FEEDBACK_H
