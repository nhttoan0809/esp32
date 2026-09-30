#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// Finite State Machine States
enum LockState {
    STATE_LOCKED,
    STATE_ENTERING_PIN,
    STATE_UNLOCKED,
    STATE_ACCESS_DENIED,
    STATE_LOCKOUT
};

// Authentication Factor Method
enum AuthMethod {
    AUTH_NONE,
    AUTH_RFID,
    AUTH_PIN,
    AUTH_WEB
};

// Access Event Log Entry
struct LogEntry {
    char timestamp[24];
    AuthMethod method;
    char identifier[24];
    bool granted;
};

// Security Operating Mode
enum SecurityMode {
    MODE_ANY_FACTOR,       // Any 1 of 3: RFID OR PIN OR Web
    MODE_DUAL_FACTOR       // High Security: RFID AND PIN
};

#endif // TYPES_H
