/**
 * main.cpp
 * Ứng dụng chính cho POC AI-04: Bàn Cảm Ứng Nhận Diện Cử Chỉ Bằng Machine Learning
 * Nền tảng: ESP32 DevKit V1 (30 chân) + MAX7219 + Relay 2 Kênh
 */

#include <Arduino.h>
#include "config.h"
#include "touch_sampler.h"
#include "feature_extractor.h"
#include "ml_inferencer.h"
#include "display_matrix.h"
#include "relay_actuator.h"

// Khởi tạo các module hệ thống
static TouchSampler touchSampler;
static FeatureExtractor featureExtractor;
static MLInferencer mlInferencer;
static DisplayMatrix displayMatrix;
static RelayActuator relayActuator;

static unsigned long lastGestureTime = 0;
static bool inCooldown = false;

void handleGesture(GestureType gesture, float confidence, const float features[MODEL_INPUT_DIM]) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    Serial.println(F("--------------------------------------------------"));
    Serial.printf("[AI] >>> DETECTED GESTURE: %s (Confidence: %.1f%%)\n",
                  mlInferencer.getGestureName(gesture), confidence * 100.0f);
    Serial.printf("[AI] Feature Vector: [t0:%.2f, t1:%.2f, t2:%.2f, t3:%.2f, asym:%.2f, grad:%.2f, peaks:%.2f, hold:%.2f]\n",
                  features[0], features[1], features[2], features[3],
                  features[4], features[5], features[6], features[7]);

    switch (gesture) {
        case GESTURE_SWIPE_RIGHT:
            Serial.println(F("[ACTION] Next Track -> Playing next song (>>>)"));
            displayMatrix.triggerAnimation(GESTURE_SWIPE_RIGHT, relayActuator.isCh1On(), relayActuator.isCh2On());
            break;

        case GESTURE_SWIPE_LEFT:
            Serial.println(F("[ACTION] Prev Track -> Rewinding previous song (<<<)"));
            displayMatrix.triggerAnimation(GESTURE_SWIPE_LEFT, relayActuator.isCh1On(), relayActuator.isCh2On());
            break;

        case GESTURE_DOUBLE_TAP: {
            bool lampState = relayActuator.toggleCh1();
            Serial.printf("[ACTION] Desk Lamp Toggled -> %s\n", lampState ? "ON [LIGHTING]" : "OFF");
            displayMatrix.triggerAnimation(GESTURE_DOUBLE_TAP, lampState, relayActuator.isCh2On());
            break;
        }

        case GESTURE_HOLD: {
            bool fanState = relayActuator.toggleCh2();
            Serial.printf("[ACTION] Desk Fan Toggled -> %s\n", fanState ? "ON [COOLING]" : "OFF");
            displayMatrix.triggerAnimation(GESTURE_HOLD, relayActuator.isCh1On(), fanState);
            break;
        }

        default:
            break;
    }
    Serial.println(F("--------------------------------------------------"));
    digitalWrite(STATUS_LED_PIN, LOW);
}

void handleSerialCommands() {
    if (!Serial.available()) return;
    char c = Serial.read();
    float dummyFeatures[MODEL_INPUT_DIM] = {0};

    switch (c) {
        case 'r':
        case 'R':
            Serial.println(F("[CMD] Simulating SWIPE_RIGHT"));
            dummyFeatures[0] = 0.1f; dummyFeatures[1] = 0.3f; dummyFeatures[2] = 0.6f; dummyFeatures[3] = 0.8f;
            dummyFeatures[4] = -0.3f; dummyFeatures[5] = 0.8f; dummyFeatures[6] = 0.33f; dummyFeatures[7] = 0.35f;
            handleGesture(GESTURE_SWIPE_RIGHT, 0.99f, dummyFeatures);
            break;

        case 'l':
        case 'L':
            Serial.println(F("[CMD] Simulating SWIPE_LEFT"));
            dummyFeatures[3] = 0.1f; dummyFeatures[2] = 0.3f; dummyFeatures[1] = 0.6f; dummyFeatures[0] = 0.8f;
            dummyFeatures[4] = 0.3f; dummyFeatures[5] = -0.8f; dummyFeatures[6] = 0.33f; dummyFeatures[7] = 0.35f;
            handleGesture(GESTURE_SWIPE_LEFT, 0.99f, dummyFeatures);
            break;

        case 'd':
        case 'D':
            Serial.println(F("[CMD] Simulating DOUBLE_TAP"));
            dummyFeatures[0] = 0.3f; dummyFeatures[1] = 0.3f; dummyFeatures[2] = -1.0f; dummyFeatures[3] = -1.0f;
            dummyFeatures[4] = 0.0f; dummyFeatures[5] = 0.0f; dummyFeatures[6] = 0.67f; dummyFeatures[7] = 0.30f;
            handleGesture(GESTURE_DOUBLE_TAP, 0.99f, dummyFeatures);
            break;

        case 'h':
        case 'H':
            Serial.println(F("[CMD] Simulating HOLD"));
            dummyFeatures[0] = 0.2f; dummyFeatures[1] = 0.2f; dummyFeatures[2] = -1.0f; dummyFeatures[3] = -1.0f;
            dummyFeatures[4] = 0.0f; dummyFeatures[5] = 0.0f; dummyFeatures[6] = 0.33f; dummyFeatures[7] = 0.85f;
            handleGesture(GESTURE_HOLD, 0.99f, dummyFeatures);
            break;

        case 's':
        case 'S':
            Serial.printf("[STATUS] Lamp: %s | Fan: %s | P0:%d P1:%d P2:%d P3:%d\n",
                          relayActuator.isCh1On() ? "ON" : "OFF",
                          relayActuator.isCh2On() ? "ON" : "OFF",
                          touchSampler.getRaw(0), touchSampler.getRaw(1),
                          touchSampler.getRaw(2), touchSampler.getRaw(3));
            break;

        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(250);

    Serial.println();
    Serial.println(F("======================================================"));
    Serial.println(F(" POC AI-04: Capacitive Touch Gesture Recognition (ML) "));
    Serial.println(F(" Target: ESP32 DevKit V1 (30-pin) | Pure C++ TinyML   "));
    Serial.println(F(" Features: MAX7219 Dot Matrix + 2-Channel Relay 5V    "));
    Serial.println(F("======================================================"));

    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    relayActuator.begin();
    displayMatrix.begin();
    touchSampler.begin();
    featureExtractor.reset();

    Serial.println(F("[SYS] System ready. Listening for touch gestures..."));
    Serial.println(F("[SYS] Quick Guide:"));
    Serial.println(F("  - Swipe Pad 0->3: Next Track (>>>)"));
    Serial.println(F("  - Swipe Pad 3->0: Previous Track (<<<)"));
    Serial.println(F("  - Double Tap: Toggle Lamp (Relay CH1)"));
    Serial.println(F("  - Hold (>700ms): Toggle Fan (Relay CH2)"));
    Serial.println(F("  - Serial Debug Keys: 'r' (Right), 'l' (Left), 'd' (DoubleTap), 'h' (Hold), 's' (Status)"));
    Serial.println();
}

void loop() {
    handleSerialCommands();

    bool newSample = touchSampler.update();
    if (newSample) {
        bool gestureReady = featureExtractor.pushFrame(touchSampler);

        if (inCooldown && (millis() - lastGestureTime >= GESTURE_COOLDOWN_MS)) {
            inCooldown = false;
        }

        if (gestureReady && !inCooldown) {
            float features[MODEL_INPUT_DIM];
            featureExtractor.extractFeatures(features);

            float confidence = 0.0f;
            float probs[MODEL_OUTPUT_DIM];
            GestureType gesture = mlInferencer.predict(features, &confidence, probs);

            if (gesture != GESTURE_IDLE) {
                lastGestureTime = millis();
                inCooldown = true;
                handleGesture(gesture, confidence, features);
            }
            featureExtractor.reset();
        }
    }

    displayMatrix.update();
}
