#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "types.h"
#include "feedback.h"
#include "lock_actuator.h"
#include "rtc_manager.h"
#include "display_manager.h"
#include "keypad_handler.h"
#include "rfid_handler.h"
#include "auth_manager.h"
#include "web_portal.h"

// System State Machine
static LockState currentState = STATE_LOCKED;
static unsigned long stateTimer = 0;
static String lastGrantedMethod = "";
static String lastDeniedReason = "";

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println();
    Serial.println(F("=================================================="));
    Serial.println(F("🛡️  POC SMH-02: TRI-FACTOR SMART ACCESS LOCK"));
    Serial.println(F("Hardware: ESP32 DevKit V1 (30-Pin)"));
    Serial.println(F("Factors : RFID Mifare • Keypad 4x4 • Web Portal"));
    Serial.println(F("=================================================="));

    // 1. Initialize Wire I2C Bus for OLED & RTC
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    // 2. Initialize Subsystems
    feedback.begin();
    displayManager.begin();
    rtcManager.begin();
    lockActuator.begin();
    keypadHandler.begin();
    rfidHandler.begin();
    authManager.begin();
    webPortal.begin();

    feedback.beepSuccess();
    Serial.println(F("[SYSTEM] All subsystems initialized successfully."));
    Serial.print(F("[SYSTEM] Web Portal IP Address: "));
    Serial.println(webPortal.getIpAddress());
    Serial.println(F("=================================================="));
}

void loop() {
    // 1. Subsystem periodic updates (non-blocking)
    feedback.update();
    lockActuator.update();
    rtcManager.update();
    displayManager.update();
    keypadHandler.update();
    rfidHandler.update();
    authManager.update();
    webPortal.update();

    // 2. Global Lockout Guard
    if (authManager.isLockoutActive() && currentState != STATE_LOCKOUT) {
        currentState = STATE_LOCKOUT;
        keypadHandler.reset();
    }

    // 3. Finite State Machine
    switch (currentState) {

        case STATE_LOCKED: {
            // Check if door was remotely unlocked via Web
            if (lockActuator.isUnlocked()) {
                lastGrantedMethod = "Web Remote";
                currentState = STATE_UNLOCKED;
                break;
            }

            // Check Factor 1: RFID Tap
            if (rfidHandler.hasCard()) {
                String uid = rfidHandler.getCardUid();
                Serial.print(F("[INPUT] Card tapped: "));
                Serial.println(uid);

                if (authManager.verifyRfid(uid)) {
                    lockActuator.unlock();
                    feedback.beepSuccess();
                    lastGrantedMethod = "RFID (" + uid + ")";
                    currentState = STATE_UNLOCKED;
                } else {
                    feedback.beepDenied();
                    lastDeniedReason = "Unknown Card (" + uid + ")";
                    stateTimer = millis();
                    currentState = STATE_ACCESS_DENIED;
                }
                break;
            }

            // Check Factor 2: Keypad PIN Entry Started
            if (keypadHandler.isTyping()) {
                currentState = STATE_ENTERING_PIN;
                break;
            }

            // Render Locked Home Screen
            displayManager.showHome(
                rtcManager.getFormattedTime(),
                rtcManager.getFormattedDate(),
                STATE_LOCKED,
                webPortal.isConnected()
            );
            break;
        }

        case STATE_ENTERING_PIN: {
            // Check if door was remotely unlocked while entering PIN
            if (lockActuator.isUnlocked()) {
                lastGrantedMethod = "Web Remote";
                keypadHandler.reset();
                currentState = STATE_UNLOCKED;
                break;
            }

            // Render current PIN typing status
            displayManager.showPinInput(
                keypadHandler.getMaskedPin(),
                keypadHandler.getPinLength()
            );

            // Check if PIN was submitted with '#'
            if (keypadHandler.hasSubmittedPin()) {
                String pin = keypadHandler.getSubmittedPin();
                Serial.println(F("[INPUT] PIN submitted for verification"));

                if (authManager.verifyPin(pin)) {
                    lockActuator.unlock();
                    feedback.beepSuccess();
                    lastGrantedMethod = "Keypad PIN";
                    currentState = STATE_UNLOCKED;
                } else {
                    feedback.beepDenied();
                    lastDeniedReason = "Wrong PIN Code";
                    stateTimer = millis();
                    currentState = STATE_ACCESS_DENIED;
                }
                break;
            }

            // If user stopped typing or canceled with '*'
            if (!keypadHandler.isTyping()) {
                currentState = STATE_LOCKED;
            }
            break;
        }

        case STATE_UNLOCKED: {
            // Render countdown progress bar
            int remainingSec = lockActuator.getRemainingRelockSeconds();
            displayManager.showAccessGranted(lastGrantedMethod, remainingSec);

            // Once auto-relock is completed by lockActuator
            if (!lockActuator.isUnlocked()) {
                currentState = STATE_LOCKED;
            }
            break;
        }

        case STATE_ACCESS_DENIED: {
            displayManager.showAccessDenied(
                lastDeniedReason,
                authManager.getFailedAttempts()
            );

            if (authManager.isLockoutActive()) {
                currentState = STATE_LOCKOUT;
                break;
            }

            if (millis() - stateTimer >= ACCESS_DENIED_DISPLAY_MS) {
                currentState = STATE_LOCKED;
            }
            break;
        }

        case STATE_LOCKOUT: {
            int remainingSec = authManager.getRemainingLockoutSeconds();
            displayManager.showLockout(remainingSec);

            if (!authManager.isLockoutActive()) {
                currentState = STATE_LOCKED;
            }
            break;
        }
    }
}
