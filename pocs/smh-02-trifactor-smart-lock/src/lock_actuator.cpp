#include "lock_actuator.h"
#include "config.h"
#include "feedback.h"

LockActuator lockActuator;

LockActuator::LockActuator()
    : unlocked(false),
      unlockTimestamp(0),
      currentAngle(SERVO_POS_LOCKED) {}

void LockActuator::begin() {
    ESP32PWM::allocateTimer(0);
    servo.setPeriodHertz(50);
    servo.attach(PIN_SERVO, 500, 2400);
    lock();
}

void LockActuator::setAngle(int angle) {
    currentAngle = angle;
    servo.write(angle);
}

void LockActuator::unlock() {
    unlocked = true;
    unlockTimestamp = millis();
    setAngle(SERVO_POS_UNLOCKED);
    feedback.setUnlockLed(true);
}

void LockActuator::lock() {
    unlocked = false;
    unlockTimestamp = 0;
    setAngle(SERVO_POS_LOCKED);
    feedback.setUnlockLed(false);
}

bool LockActuator::isUnlocked() const {
    return unlocked;
}

unsigned long LockActuator::getRemainingRelockMs() const {
    if (!unlocked) return 0;
    unsigned long elapsed = millis() - unlockTimestamp;
    if (elapsed >= AUTO_RELOCK_DELAY_MS) return 0;
    return AUTO_RELOCK_DELAY_MS - elapsed;
}

int LockActuator::getRemainingRelockSeconds() const {
    unsigned long remainingMs = getRemainingRelockMs();
    if (remainingMs == 0) return 0;
    return (int)((remainingMs + 999) / 1000);
}

void LockActuator::update() {
    if (unlocked) {
        if (millis() - unlockTimestamp >= AUTO_RELOCK_DELAY_MS) {
            lock();
            feedback.beepShort(); // Audio confirmation of auto-relock
        }
    }
}
