/**
 * relay_actuator.cpp
 * Hiện thực điều khiển Relay 2 kênh (Active LOW)
 */

#include "relay_actuator.h"

RelayActuator::RelayActuator() : ch1State(false), ch2State(false) {}

void RelayActuator::begin() {
    pinMode(RELAY_CH1_PIN, OUTPUT);
    pinMode(RELAY_CH2_PIN, OUTPUT);

    // Mặc định ban đầu tắt cả 2 tải (mức HIGH cho Active LOW)
    digitalWrite(RELAY_CH1_PIN, RELAY_INACTIVE_LEVEL);
    digitalWrite(RELAY_CH2_PIN, RELAY_INACTIVE_LEVEL);
    ch1State = false;
    ch2State = false;

    Serial.println(F("[RELAY] Relays initialized (Active LOW, Initial state: OFF)."));
}

void RelayActuator::setCh1(bool state) {
    ch1State = state;
    digitalWrite(RELAY_CH1_PIN, state ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL);
    Serial.printf("[RELAY] Channel 1 (Desk Lamp): %s\n", state ? "ON" : "OFF");
}

void RelayActuator::setCh2(bool state) {
    ch2State = state;
    digitalWrite(RELAY_CH2_PIN, state ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL);
    Serial.printf("[RELAY] Channel 2 (Desk Fan): %s\n", state ? "ON" : "OFF");
}

bool RelayActuator::toggleCh1() {
    setCh1(!ch1State);
    return ch1State;
}

bool RelayActuator::toggleCh2() {
    setCh2(!ch2State);
    return ch2State;
}
