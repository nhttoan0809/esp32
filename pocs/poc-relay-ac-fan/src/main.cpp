#include <Arduino.h>

// ===== PIN DEFINITIONS =====
static const int PIN_RELAY  = 26; // Active LOW: LOW = ON, HIGH = OFF
static const int PIN_LED    = 27; // Status LED: HIGH = ON, LOW = OFF
static const int PIN_BUTTON = 14; // INPUT_PULLUP: LOW = Pressed

// ===== CONSTANTS =====
static const unsigned long DEBOUNCE_DELAY_MS = 50;
static const unsigned long FORCE_OFF_HOLD_MS = 3000;

// ===== STATE =====
static bool relayActive = false; // Initial safe state: OFF

// Button tracking state
static int lastButtonReading = HIGH;
static int buttonState = HIGH;
static unsigned long lastDebounceTime = 0;
static unsigned long pressStartTime = 0;
static bool isPressing = false;
static bool forceOffTriggered = false;

void applyRelayState(bool active) {
  relayActive = active;
  // Active LOW relay: LOW turns coil ON, HIGH turns coil OFF
  digitalWrite(PIN_RELAY, relayActive ? LOW : HIGH);
  // Status LED: HIGH when ON, LOW when OFF
  digitalWrite(PIN_LED, relayActive ? HIGH : LOW);

  Serial.print("[RELAY] State: ");
  Serial.println(relayActive ? "ON" : "OFF");
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // Initial safe state: Relay OFF, LED OFF
  applyRelayState(false);

  Serial.println("[BOOT] Relay AC Fan Controller Initialized");
  Serial.println("[RELAY] State: OFF");
}

void loop() {
  int reading = digitalRead(PIN_BUTTON);

  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        // Button pressed down
        isPressing = true;
        pressStartTime = millis();
        forceOffTriggered = false;
      } else {
        // Button released
        if (isPressing && !forceOffTriggered) {
          // Short press released: Toggle relay
          applyRelayState(!relayActive);
        }
        isPressing = false;
      }
    }
  }

  // Check long press (Safety override: Force OFF)
  if (isPressing && !forceOffTriggered) {
    if ((millis() - pressStartTime) >= FORCE_OFF_HOLD_MS) {
      forceOffTriggered = true;
      Serial.println("[SAFETY] Force OFF triggered");
      applyRelayState(false);
    }
  }

  lastButtonReading = reading;
  delay(10);
}
