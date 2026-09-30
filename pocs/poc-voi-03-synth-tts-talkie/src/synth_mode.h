#ifndef VOI03_SYNTH_MODE_H
#define VOI03_SYNTH_MODE_H

#include <Arduino.h>
#include "config.h"
#include "audio_engine.h"
#include "tm1637_driver.h"
#include "pot_driver.h"
#include "keypad_driver.h"

struct NoteDefinition {
    char key;
    const char* name;
    uint16_t baseFreq;
    uint8_t octave;
};

class SynthMode {
public:
    SynthMode(AudioEngine& audio, TM1637Driver& display, PotentiometerDriver& pot);

    void enterOrgan();
    void enterSFX();

    void updateOrgan(KeypadDriver& keypad);
    void updateSFX(KeypadDriver& keypad);

private:
    AudioEngine& _audio;
    TM1637Driver& _display;
    PotentiometerDriver& _pot;

    int8_t _octaveOffset;
    char _activeKey;
    uint16_t _currentNoteBaseFreq;
    int16_t _lastPitchBend;

    const NoteDefinition* findNote(char key);
    uint16_t calculateActualFreq(uint16_t baseFreq, int8_t octaveOffset, int16_t pitchBend);

    // SFX non-blocking generator
    void playLaser();
    void playCoin();
    void playJump();
    void playPowerUp();
    void playSiren();
};

#endif // VOI03_SYNTH_MODE_H
