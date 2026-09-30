#ifndef VOI03_TALKIE_MODE_H
#define VOI03_TALKIE_MODE_H

#include <Arduino.h>
#include "config.h"
#include "audio_engine.h"
#include "tm1637_driver.h"
#include "keypad_driver.h"

#if __has_include(<Talkie.h>)
#include <Talkie.h>
#include <Vocab_US_Large.h>
#define TALKIE_LIB_AVAILABLE 1
#else
#define TALKIE_LIB_AVAILABLE 0
#endif

class TalkieMode {
public:
    TalkieMode(AudioEngine& audio, TM1637Driver& display);

    void begin();
    void enter();
    void update(KeypadDriver& keypad);

    void sayCount();
    void saySecurityAlert();
    void sayTemperatureReport();
    void saySystemCheck();
    void saySystemStop();

private:
    AudioEngine& _audio;
    TM1637Driver& _display;

#if TALKIE_LIB_AVAILABLE
    Talkie _voice;
#endif

    void playRobotFormant(uint16_t f1, uint16_t f2, uint16_t durationMs);
};

#endif // VOI03_TALKIE_MODE_H
