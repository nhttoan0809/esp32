#ifndef LOCK_ACTUATOR_H
#define LOCK_ACTUATOR_H

#include <Arduino.h>
#include <ESP32Servo.h>

class LockActuator {
public:
    LockActuator();
    void begin();
    void update();

    void unlock();
    void lock();

    bool isUnlocked() const;
    unsigned long getRemainingRelockMs() const;
    int getRemainingRelockSeconds() const;

private:
    Servo servo;
    bool unlocked;
    unsigned long unlockTimestamp;
    int currentAngle;

    void setAngle(int angle);
};

extern LockActuator lockActuator;

#endif // LOCK_ACTUATOR_H
