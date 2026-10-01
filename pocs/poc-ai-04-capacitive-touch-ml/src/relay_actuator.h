/**
 * relay_actuator.h
 * Điều khiển Module Relay 2 Kênh 5V (Active LOW)
 */

#ifndef RELAY_ACTUATOR_H
#define RELAY_ACTUATOR_H

#include "config.h"

class RelayActuator {
public:
    RelayActuator();
    void begin();

    void setCh1(bool state);
    void setCh2(bool state);
    bool toggleCh1();
    bool toggleCh2();

    bool isCh1On() const { return ch1State; }
    bool isCh2On() const { return ch2State; }

private:
    bool ch1State;
    bool ch2State;
};

#endif // RELAY_ACTUATOR_H
