#include "feedback.h"
#include "config.h"

Feedback feedback;

Feedback::Feedback() 
    : currentPattern(PATTERN_NONE),
      patternStartTime(0),
      patternStep(0),
      buzzerState(false),
      alarmActive(false),
      ledBlinking(false),
      ledBlinkRemaining(0),
      ledBlinkInterval(100),
      lastLedToggleTime(0),
      currentLedState(false) {}

void Feedback::begin() {
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    pinMode(PIN_LED_UNLOCK, OUTPUT);
    digitalWrite(PIN_LED_UNLOCK, LOW);
}

void Feedback::setBuzzer(bool on) {
    buzzerState = on;
    digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
}

void Feedback::setUnlockLed(bool on) {
    currentLedState = on;
    digitalWrite(PIN_LED_UNLOCK, on ? HIGH : LOW);
    ledBlinking = false;
}

void Feedback::beepShort() {
    if (alarmActive) return;
    currentPattern = PATTERN_SHORT;
    patternStartTime = millis();
    patternStep = 0;
    setBuzzer(true);
}

void Feedback::beepSuccess() {
    if (alarmActive) return;
    currentPattern = PATTERN_SUCCESS;
    patternStartTime = millis();
    patternStep = 0;
    setBuzzer(true);
}

void Feedback::beepDenied() {
    if (alarmActive) return;
    currentPattern = PATTERN_DENIED;
    patternStartTime = millis();
    patternStep = 0;
    setBuzzer(true);
}

void Feedback::startAlarm() {
    alarmActive = true;
    currentPattern = PATTERN_ALARM;
    patternStartTime = millis();
    patternStep = 0;
    setBuzzer(true);
}

void Feedback::stopAlarm() {
    alarmActive = false;
    currentPattern = PATTERN_NONE;
    setBuzzer(false);
}

void Feedback::blinkUnlockLed(int count, unsigned long intervalMs) {
    ledBlinking = true;
    ledBlinkRemaining = count * 2;
    ledBlinkInterval = intervalMs;
    lastLedToggleTime = millis();
    currentLedState = true;
    digitalWrite(PIN_LED_UNLOCK, HIGH);
}

void Feedback::update() {
    unsigned long now = millis();

    // 1. Handle LED Blinking
    if (ledBlinking) {
        if (now - lastLedToggleTime >= ledBlinkInterval) {
            lastLedToggleTime = now;
            currentLedState = !currentLedState;
            digitalWrite(PIN_LED_UNLOCK, currentLedState ? HIGH : LOW);
            ledBlinkRemaining--;
            if (ledBlinkRemaining <= 0) {
                ledBlinking = false;
                digitalWrite(PIN_LED_UNLOCK, LOW);
            }
        }
    }

    // 2. Handle Buzzer Patterns
    if (currentPattern == PATTERN_NONE) {
        return;
    }

    unsigned long elapsed = now - patternStartTime;

    switch (currentPattern) {
        case PATTERN_SHORT:
            if (elapsed >= 50) {
                setBuzzer(false);
                currentPattern = PATTERN_NONE;
            }
            break;

        case PATTERN_SUCCESS:
            // Step 0: Beep 1 ON (100ms)
            // Step 1: Pause (80ms)
            // Step 2: Beep 2 ON (100ms)
            if (patternStep == 0 && elapsed >= 100) {
                setBuzzer(false);
                patternStep = 1;
                patternStartTime = now;
            } else if (patternStep == 1 && elapsed >= 80) {
                setBuzzer(true);
                patternStep = 2;
                patternStartTime = now;
            } else if (patternStep == 2 && elapsed >= 100) {
                setBuzzer(false);
                currentPattern = PATTERN_NONE;
            }
            break;

        case PATTERN_DENIED:
            if (elapsed >= 800) {
                setBuzzer(false);
                currentPattern = PATTERN_NONE;
            }
            break;

        case PATTERN_ALARM:
            // Continuous alarm warble: 200ms ON, 150ms OFF
            if (buzzerState && elapsed >= 200) {
                setBuzzer(false);
                patternStartTime = now;
            } else if (!buzzerState && elapsed >= 150) {
                setBuzzer(true);
                patternStartTime = now;
            }
            break;

        default:
            currentPattern = PATTERN_NONE;
            setBuzzer(false);
            break;
    }
}
