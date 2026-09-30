#include <Arduino.h>
#include "config.h"
#include "audio_engine.h"
#include "tm1637_driver.h"
#include "pot_driver.h"
#include "keypad_driver.h"
#include "synth_mode.h"
#include "jukebox_mode.h"
#include "talkie_mode.h"

// Khởi tạo các ngoại vi phần cứng
AudioEngine audio(PIN_BUZZER);
TM1637Driver display(PIN_TM1637_CLK, PIN_TM1637_DIO);
PotentiometerDriver pot(PIN_POTENTIOMETER);
KeypadDriver keypad;

// Khởi tạo các bộ điều khiển chế độ
SynthMode synth(audio, display, pot);
JukeboxMode jukebox(audio, display, pot);
TalkieMode talkie(audio, display);

// Trạng thái FSM toàn cục
SystemMode currentMode = MODE_ORGAN;
uint32_t lastHeartbeatTime = 0;

void switchMode(SystemMode newMode) {
    if (newMode >= MODE_COUNT) newMode = MODE_ORGAN;

    // Dừng âm thanh của chế độ cũ
    audio.stopTone();
    currentMode = newMode;

    switch (currentMode) {
        case MODE_ORGAN:
            synth.enterOrgan();
            break;
        case MODE_JUKEBOX:
            jukebox.enter();
            break;
        case MODE_TALKIE:
            talkie.enter();
            break;
        case MODE_SFX:
            synth.enterSFX();
            break;
        default:
            break;
    }
}

void printBanner() {
    Serial.println();
    Serial.println(F("================================================================="));
    Serial.println(F("  [VOI-03] SYNTHESIZER & TTS TALKIE SYSTEM READY                "));
    Serial.println(F("  Platform: ESP32 DevKit V1 (30-Pin) | Dual-Target: Real & Wokwi "));
    Serial.println(F("================================================================="));
    Serial.println(F("Controls:"));
    Serial.println(F("  [D] : Cycle Mode (Organ -> Jukebox -> Talkie -> SFX)"));
    Serial.println(F("  [A] : Fast Switch -> Organ Synthesizer Mode"));
    Serial.println(F("  [B] : Fast Switch -> Offline TTS Talkie Mode"));
    Serial.println(F("  [C] : Fast Switch -> Jukebox Retro Chiptune Mode"));
    Serial.println(F("  [1-0]: Notes / Songs / Phrases / SFX in respective modes"));
    Serial.println(F("  Potentiometer: Pitch Bend in Organ | Tempo BPM in Jukebox"));
    Serial.println(F("================================================================="));
}

void setup() {
    Serial.begin(115200);
    delay(300);

    printBanner();

    // Khởi tạo phần cứng
    audio.begin();
    display.begin();
    pot.begin();
    keypad.begin();
    talkie.begin();

    // Hiệu ứng khởi động ngắn trên màn hình TM1637
    display.displayString("VOI3");
    audio.playTone(NOTE_C5, 80);
    delay(90);
    audio.playTone(NOTE_G5, 120);
    delay(130);
    audio.stopTone();

    // Vào chế độ mặc định: Organ Synthesizer
    switchMode(MODE_ORGAN);
}

void loop() {
    // 1. Quét định kỳ các ngõ vào
    keypad.update();
    pot.update();
    audio.update();

    // 2. Xử lý các phím nóng chuyển đổi chế độ toàn cục
    // Phím D: chuyển vòng tròn
    if (keypad.isKeyPressed('D')) {
        char k = keypad.getKey(); // Clear buffer
        (void)k;
        SystemMode nextMode = (SystemMode)((currentMode + 1) % MODE_COUNT);
        switchMode(nextMode);
        delay(200); // Debounce chuyển mode
        return;
    }

    // Phím A: Organ
    if (keypad.isKeyPressed('A')) {
        char k = keypad.getKey();
        (void)k;
        if (currentMode != MODE_ORGAN) switchMode(MODE_ORGAN);
        return;
    }

    // Phím B: Talkie
    if (keypad.isKeyPressed('B')) {
        char k = keypad.getKey();
        (void)k;
        if (currentMode != MODE_TALKIE) switchMode(MODE_TALKIE);
        return;
    }

    // Phím C: Jukebox
    if (keypad.isKeyPressed('C')) {
        char k = keypad.getKey();
        (void)k;
        if (currentMode != MODE_JUKEBOX) switchMode(MODE_JUKEBOX);
        return;
    }

    // 3. Dispatch tới chế độ FSM hiện tại
    switch (currentMode) {
        case MODE_ORGAN:
            synth.updateOrgan(keypad);
            break;
        case MODE_JUKEBOX:
            jukebox.update(keypad);
            break;
        case MODE_TALKIE:
            talkie.update(keypad);
            break;
        case MODE_SFX:
            synth.updateSFX(keypad);
            break;
        default:
            break;
    }

    // 4. Nhịp tim chẩn đoán hệ thống (Diagnostic Heartbeat mỗi 5s)
    uint32_t now = millis();
    if (now - lastHeartbeatTime >= 5000) {
        lastHeartbeatTime = now;
        const char* modeName = (currentMode == MODE_ORGAN)   ? "ORGAN" :
                               (currentMode == MODE_JUKEBOX) ? "JUKEBOX" :
                               (currentMode == MODE_TALKIE)  ? "TALKIE" : "SFX";
        Serial.printf("[HEARTBEAT] Mode: %s | Pot Raw: %u | Bend: %+dHz | Tempo: %u BPM | Uptime: %lus\n",
                      modeName, pot.getFiltered(), pot.getPitchBendHz(), pot.getTempoBpm(), now / 1000);
    }
}
