/**
 * @file main.cpp
 * @brief POC VOI-02: Acoustic Clap Switch with Multi-Channel Relay Control
 * @details Implements acoustic debouncing, multi-clap pattern recognition,
 *          OLED real-time status display, passive buzzer audio feedback,
 *          and post-execution mechanical relay click acoustic isolation.
 *
 * Hardware:
 *   - ESP32 DevKit V1 (30 pins)
 *   - HW-484 Sound Sensor (DO: GPIO 17, AO: GPIO 34)
 *   - 2-Channel Relay Module 5V (IN1: GPIO 26, IN2: GPIO 25) - Active LOW
 *   - OLED 0.96" SSD1306 (SDA: GPIO 21, SCL: GPIO 22)
 *   - Passive Buzzer (GPIO 19)
 *   - Status LEDs (CH1: GPIO 13, CH2: GPIO 14)
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================================
// HARDWARE PIN DEFINITIONS (ESP32 DevKit V1 30-Pin)
// ============================================================================
#define PIN_SOUND_DO      17    // HW-484 Digital Out / Clap Trigger Button (Active LOW)
#define PIN_SOUND_AO      34    // HW-484 Analog Out / Potentiometer (ADC1_CH6)
#define PIN_OLED_SDA      21    // I2C Data Line
#define PIN_OLED_SCL      22    // I2C Clock Line
#define PIN_RELAY_CH1     26    // Relay 1 Control - Light (Active LOW)
#define PIN_RELAY_CH2     25    // Relay 2 Control - Fan (Active LOW)
#define PIN_BUZZER        19    // Passive Buzzer PWM Signal
#define PIN_LED_CH1       13    // Status LED Channel 1 (Green)
#define PIN_LED_CH2       14    // Status LED Channel 2 (Blue)

// ============================================================================
// ACOUSTIC TIMING CONSTANTS (Milliseconds)
// ============================================================================
const unsigned long BLANKING_TIME_MS      = 150;  // Acoustic debounce / echo ring-down lockout
const unsigned long CLAP_WINDOW_MS        = 650;  // Detection window for subsequent claps
const unsigned long COOLDOWN_TIME_MS      = 450;  // Relay mechanical click acoustic lockout
const unsigned long OLED_REFRESH_MS       = 50;   // Display refresh period (20Hz)
const unsigned long SERIAL_LOG_INTERVAL_MS = 2000; // Heartbeat log period

// OLED Configuration
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============================================================================
// STATE MACHINE ENUMERATION
// ============================================================================
enum SystemState {
  STATE_IDLE,       // Waiting for initial clap
  STATE_LISTENING,  // First clap detected, collecting pattern
  STATE_EXECUTING,  // Processing command based on clap count
  STATE_COOLDOWN    // Acoustic isolation while relay mechanical vibration settles
};

// ============================================================================
// GLOBAL SYSTEM STATE & VOLATILE ISR FLAGS
// ============================================================================
volatile unsigned long lastInterruptTime = 0;
volatile bool clapInterruptFlag = false;

SystemState currentState = STATE_IDLE;
uint8_t clapCount = 0;
unsigned long windowStartTime = 0;
unsigned long cooldownStartTime = 0;
unsigned long lastOledRefresh = 0;
unsigned long lastSerialLog = 0;

// Relay & Actuator States (Active LOW: LOW = ON, HIGH = OFF)
bool relay1State = false; // false = OFF (HIGH), true = ON (LOW)
bool relay2State = false; // false = OFF (HIGH), true = ON (LOW)

String lastActionText = "READY (IDLE)";

// ============================================================================
// BUZZER AUDIO FEEDBACK HELPER FUNCTIONS
// ============================================================================
void playTone(uint16_t freq, uint16_t duration_ms) {
  tone(PIN_BUZZER, freq, duration_ms);
}

void playFeedbackTick() {
  playTone(2400, 30);
}

void playCh1ConfirmMelody() {
  tone(PIN_BUZZER, 1800, 50);
  delay(60);
  tone(PIN_BUZZER, 2400, 80);
}

void playCh2ConfirmMelody() {
  tone(PIN_BUZZER, 2000, 40);
  delay(50);
  tone(PIN_BUZZER, 2500, 50);
  delay(60);
  tone(PIN_BUZZER, 3000, 80);
}

void playAllOffMelody() {
  tone(PIN_BUZZER, 2400, 50);
  delay(60);
  tone(PIN_BUZZER, 1800, 60);
  delay(70);
  tone(PIN_BUZZER, 1100, 120);
}

// ============================================================================
// INTERRUPT SERVICE ROUTINE (EXT INTERRUPT ON GPIO 17)
// ============================================================================
void IRAM_ATTR onClapInterrupt() {
  unsigned long now = millis();
  // Acoustic Debounce: Ignore any pulse within BLANKING_TIME_MS
  if (now - lastInterruptTime > BLANKING_TIME_MS) {
    lastInterruptTime = now;
    clapInterruptFlag = true;
  }
}

// ============================================================================
// HARDWARE ACTUATOR UPDATE
// ============================================================================
void updateActuators() {
  // Relays are Active LOW
  digitalWrite(PIN_RELAY_CH1, relay1State ? LOW : HIGH);
  digitalWrite(PIN_RELAY_CH2, relay2State ? LOW : HIGH);

  // Status LEDs are Active HIGH
  digitalWrite(PIN_LED_CH1, relay1State ? HIGH : LOW);
  digitalWrite(PIN_LED_CH2, relay2State ? HIGH : LOW);
}

// ============================================================================
// OLED UI RENDERER
// ============================================================================
void renderOledUI(uint16_t soundLevel) {
  display.clearDisplay();

  // 1. Header Banner
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(F("VOI-02 CLAP SWITCH"));

  // Sound VU Level indicator (top right)
  uint8_t vuWidth = map(soundLevel, 0, 4095, 0, 24);
  display.drawRect(102, 0, 26, 7, SSD1306_WHITE);
  if (vuWidth > 0) {
    display.fillRect(103, 1, vuWidth, 5, SSD1306_WHITE);
  }
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  // 2. Channel Status Panels
  display.setCursor(0, 13);
  display.print(F("CH1 [LIGHT]: "));
  if (relay1State) {
    display.fillRect(78, 12, 34, 9, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.print(F(" ON "));
    display.setTextColor(SSD1306_WHITE);
  } else {
    display.print(F("[OFF]"));
  }

  display.setCursor(0, 24);
  display.print(F("CH2 [ FAN ]: "));
  if (relay2State) {
    display.fillRect(78, 23, 34, 9, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.print(F(" ON "));
    display.setTextColor(SSD1306_WHITE);
  } else {
    display.print(F("[OFF]"));
  }

  display.drawLine(0, 35, 127, 35, SSD1306_WHITE);

  // 3. FSM Status & Clap Counter
  display.setCursor(0, 39);
  if (currentState == STATE_LISTENING) {
    display.print(F("CLAP DETECTED: "));
    display.print(clapCount);

    // Remaining window countdown bar
    long elapsed = millis() - windowStartTime;
    long remaining = CLAP_WINDOW_MS - elapsed;
    if (remaining < 0) remaining = 0;
    uint8_t timerBar = map(remaining, 0, CLAP_WINDOW_MS, 0, 128);
    display.fillRect(0, 49, timerBar, 3, SSD1306_WHITE);
  } else if (currentState == STATE_COOLDOWN) {
    display.print(F("COOLDOWN LOCKOUT..."));
  } else {
    display.print(F("WAITING FOR CLAP..."));
  }

  // 4. Action Banner / Notification Footer
  display.setCursor(0, 54);
  display.print(F("> "));
  display.print(lastActionText);

  display.display();
}

// ============================================================================
// SYSTEM SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("\n=================================================="));
  Serial.println(F("  VOI-02: Acoustic Clap Switch with Multi-Relay   "));
  Serial.println(F("=================================================="));

  // Configure Hardware Pins
  pinMode(PIN_SOUND_DO, INPUT_PULLUP);
  pinMode(PIN_SOUND_AO, INPUT);
  pinMode(PIN_RELAY_CH1, OUTPUT);
  pinMode(PIN_RELAY_CH2, OUTPUT);
  pinMode(PIN_LED_CH1, OUTPUT);
  pinMode(PIN_LED_CH2, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // Initial actuator states (All OFF)
  updateActuators();
  noTone(PIN_BUZZER);

  // Initialize OLED SSD1306 (I2C)
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("[ERROR] OLED SSD1306 initialization failed!"));
  } else {
    Serial.println(F("[OK] OLED SSD1306 initialized successfully"));
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.println(F("VOI-02 CLAP SWITCH"));
    display.setCursor(10, 35);
    display.println(F("Initializing..."));
    display.display();
    delay(1000);
  }

  // Attach Interrupt on Sound DO Pin (FALLING Edge when clap sound hits mic)
  attachInterrupt(digitalPinToInterrupt(PIN_SOUND_DO), onClapInterrupt, FALLING);
  Serial.printf("[OK] Ext Interrupt attached to GPIO %d (FALLING)\n", PIN_SOUND_DO);

  // Startup audio indicator (Double chirp)
  playTone(1800, 60);
  delay(80);
  playTone(2400, 100);

  Serial.println(F("[VOI-02] Verification Marker: ACOUSTIC_CLAP_SWITCH_READY"));
  Serial.println(F("[SYSTEM] Ready! Listening for clap acoustic patterns..."));
}

// ============================================================================
// MAIN LOOP & FINITE STATE MACHINE (NON-BLOCKING)
// ============================================================================
void loop() {
  unsigned long now = millis();
  uint16_t soundLevel = analogRead(PIN_SOUND_AO);

  // 1. Process Ext Interrupt Flags
  if (clapInterruptFlag) {
    clapInterruptFlag = false;

    // Reject sound triggers during relay mechanical cooldown
    if (currentState == STATE_COOLDOWN) {
      Serial.println(F("[ISOLATE] Clap pulse ignored during relay cooldown lockout"));
    } else if (currentState == STATE_IDLE) {
      currentState = STATE_LISTENING;
      clapCount = 1;
      windowStartTime = now;
      lastActionText = "Clap #1 detected";
      Serial.printf("[ACOUSTIC] Clap #1 recorded at t=%lu ms\n", now);
      playFeedbackTick();
    } else if (currentState == STATE_LISTENING) {
      clapCount++;
      windowStartTime = now; // Re-arm detection window for next clap
      lastActionText = "Clap #" + String(clapCount) + " detected";
      Serial.printf("[ACOUSTIC] Clap #%d recorded at t=%lu ms\n", clapCount, now);
      playFeedbackTick();
    }
  }

  // 2. FSM Execution & State Transitions
  switch (currentState) {
    case STATE_IDLE:
      // Awaiting first clap interrupt
      break;

    case STATE_LISTENING:
      // Check if window has expired without further claps
      if (now - windowStartTime >= CLAP_WINDOW_MS) {
        currentState = STATE_EXECUTING;
      }
      break;

    case STATE_EXECUTING:
      Serial.printf("[EXECUTE] Processing pattern: %d clap(s)\n", clapCount);

      if (clapCount == 1) {
        relay1State = !relay1State;
        lastActionText = relay1State ? "CH1 (LIGHT): ON" : "CH1 (LIGHT): OFF";
        Serial.printf("[ACTION] 1 Clap -> Toggled Relay 1 (Light): %s\n", relay1State ? "ON" : "OFF");
        updateActuators();
        playCh1ConfirmMelody();
      } else if (clapCount == 2) {
        relay2State = !relay2State;
        lastActionText = relay2State ? "CH2 (FAN): ON" : "CH2 (FAN): OFF";
        Serial.printf("[ACTION] 2 Claps -> Toggled Relay 2 (Fan): %s\n", relay2State ? "ON" : "OFF");
        updateActuators();
        playCh2ConfirmMelody();
      } else if (clapCount >= 3) {
        relay1State = false;
        relay2State = false;
        lastActionText = "ALL DEVICES OFF";
        Serial.println(F("[ACTION] 3+ Claps -> Turned OFF both channels (Master OFF)"));
        updateActuators();
        playAllOffMelody();
      }

      // Transition to Cooldown to isolate mechanical relay acoustics
      cooldownStartTime = millis();
      currentState = STATE_COOLDOWN;
      clapCount = 0;
      clapInterruptFlag = false; // Clear any residual spikes
      break;

    case STATE_COOLDOWN:
      // Discard any vibrations caused by the relay opening/closing
      clapInterruptFlag = false;
      if (now - cooldownStartTime >= COOLDOWN_TIME_MS) {
        currentState = STATE_IDLE;
        lastActionText = "READY (IDLE)";
        Serial.println(F("[SYSTEM] Cooldown complete. Ready for next command."));
      }
      break;
  }

  // 3. Periodic UI Refresh
  if (now - lastOledRefresh >= OLED_REFRESH_MS) {
    lastOledRefresh = now;
    renderOledUI(soundLevel);
  }

  // 4. Periodic Serial Heartbeat
  if (now - lastSerialLog >= SERIAL_LOG_INTERVAL_MS) {
    lastSerialLog = now;
    if (currentState == STATE_IDLE) {
      Serial.printf("[HEARTBEAT] Idle | CH1:%s CH2:%s | Mic Analog Level: %u\n",
                    relay1State ? "ON" : "OFF",
                    relay2State ? "ON" : "OFF",
                    soundLevel);
    }
  }
}
