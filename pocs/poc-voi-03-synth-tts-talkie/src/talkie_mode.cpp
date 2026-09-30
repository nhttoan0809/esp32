#include "talkie_mode.h"

TalkieMode::TalkieMode(AudioEngine& audio, TM1637Driver& display)
    : _audio(audio), _display(display) {}

void TalkieMode::begin() {
    // Talkie tự khởi tạo trong constructor
}

void TalkieMode::enter() {
    _audio.stopTone();
    _display.showMode(MODE_TALKIE);
    Serial.println(F("[MODE] Entered OFFLINE TTS TALKIE Mode. Press 1-5 to trigger synthesized speech."));
}

void TalkieMode::playRobotFormant(uint16_t f1, uint16_t f2, uint16_t durationMs) {
    uint32_t half = durationMs / 2;
    _audio.playTone(f1, half);
    delay(half);
    _audio.playTone(f2, half);
    delay(half);
    _audio.stopTone();
}

void TalkieMode::sayCount() {
    _display.displayString("COUn");
    Serial.println(F("[TALKIE] Voice Output: \"ZERO ONE TWO THREE FOUR FIVE\""));

    _audio.prepareForDAC();

#if TALKIE_LIB_AVAILABLE
    _voice.say(sp2_ZERO);
    _voice.say(sp2_ONE);
    _voice.say(sp2_TWO);
    _voice.say(sp2_THREE);
    _voice.say(sp2_FOUR);
    _voice.say(sp2_FIVE);
    _voice.terminateHardware();
#else
    playRobotFormant(400, 600, 150);
    playRobotFormant(500, 750, 150);
    playRobotFormant(600, 900, 150);
#endif

    _audio.resumePWM();
    _display.showMode(MODE_TALKIE);
}

void TalkieMode::saySecurityAlert() {
    _display.displayString("dAnG");
    Serial.println(F("[TALKIE] Voice Output: \"WARNING! DANGER! ALERT!\""));

    _audio.prepareForDAC();

#if TALKIE_LIB_AVAILABLE
    _voice.say(sp4_WARNING);
    _voice.say(sp2_DANGER);
    _voice.say(sp2_ALERT);
    _voice.terminateHardware();
#else
    playRobotFormant(800, 400, 200);
    playRobotFormant(900, 450, 200);
    playRobotFormant(1000, 500, 250);
#endif

    _audio.resumePWM();
    _display.showMode(MODE_TALKIE);
}

void TalkieMode::sayTemperatureReport() {
    _display.displayString("rEAd");
    Serial.println(F("[TALKIE] Voice Output: \"SYSTEM READY. TEMPERATURE THIRTY DEGREES.\""));

    _audio.prepareForDAC();

#if TALKIE_LIB_AVAILABLE
    _voice.say(sp2_READY);
    _voice.say(sp2_TEMPERATURE);
    _voice.say(sp2_THREE);
    _voice.say(sp2_ZERO);
    _voice.say(sp2_DEGREES);
    _voice.terminateHardware();
#else
    playRobotFormant(450, 550, 180);
    playRobotFormant(550, 700, 180);
    playRobotFormant(600, 800, 220);
#endif

    _audio.resumePWM();
    _display.showMode(MODE_TALKIE);
}

void TalkieMode::saySystemCheck() {
    _display.displayString("CHk ");
    Serial.println(F("[TALKIE] Voice Output: \"CHECK. START. SYSTEM READY.\""));

    _audio.prepareForDAC();

#if TALKIE_LIB_AVAILABLE
    _voice.say(sp2_CHECK);
    _voice.say(sp2_START);
    _voice.say(sp2_READY);
    _voice.terminateHardware();
#else
    playRobotFormant(523, 659, 150);
    playRobotFormant(784, 1046, 250);
#endif

    _audio.resumePWM();
    _display.showMode(MODE_TALKIE);
}

void TalkieMode::saySystemStop() {
    _display.displayString("StOP");
    Serial.println(F("[TALKIE] Voice Output: \"STOP. SYSTEM CLEAR.\""));

    _audio.prepareForDAC();

#if TALKIE_LIB_AVAILABLE
    _voice.say(sp2_STOP);
    _voice.say(sp3_CLEAR);
    _voice.terminateHardware();
#else
    playRobotFormant(600, 400, 200);
    playRobotFormant(500, 300, 250);
#endif

    _audio.resumePWM();
    _display.showMode(MODE_TALKIE);
}

void TalkieMode::update(KeypadDriver& keypad) {
    char key = keypad.getKey();
    if (key == '\0') return;

    switch (key) {
        case '1':
            sayCount();
            break;
        case '2':
            saySecurityAlert();
            break;
        case '3':
            sayTemperatureReport();
            break;
        case '4':
            saySystemCheck();
            break;
        case '5':
            saySystemStop();
            break;
        default:
            break;
    }
}
