#include "synth_mode.h"

static const NoteDefinition NOTE_TABLE[] = {
    {'1', "C", NOTE_C4, 4},
    {'2', "d", NOTE_D4, 4},
    {'3', "E", NOTE_E4, 4},
    {'4', "F", NOTE_F4, 4},
    {'5', "G", NOTE_G4, 4},
    {'6', "A", NOTE_A4, 4},
    {'7', "b", NOTE_B4, 4},
    {'8', "C", NOTE_C5, 5},
    {'9', "d", NOTE_D5, 5},
    {'0', "E", NOTE_E5, 5}
};
static const size_t NOTE_TABLE_SIZE = sizeof(NOTE_TABLE) / sizeof(NOTE_TABLE[0]);

SynthMode::SynthMode(AudioEngine& audio, TM1637Driver& display, PotentiometerDriver& pot)
    : _audio(audio), _display(display), _pot(pot),
      _octaveOffset(0), _activeKey('\0'), _currentNoteBaseFreq(0), _lastPitchBend(0) {}

void SynthMode::enterOrgan() {
    _audio.stopTone();
    _activeKey = '\0';
    _currentNoteBaseFreq = 0;
    _octaveOffset = 0;
    _lastPitchBend = 0;
    _display.showMode(MODE_ORGAN);
    Serial.println(F("[MODE] Entered ORGAN SYNTHESIZER Mode. Press 1-0 to play notes, * / # for Octave."));
}

void SynthMode::enterSFX() {
    _audio.stopTone();
    _activeKey = '\0';
    _display.showMode(MODE_SFX);
    Serial.println(F("[MODE] Entered 8-BIT SFX Mode. Press 1-5 for retro game sounds."));
}

const NoteDefinition* SynthMode::findNote(char key) {
    for (size_t i = 0; i < NOTE_TABLE_SIZE; i++) {
        if (NOTE_TABLE[i].key == key) {
            return &NOTE_TABLE[i];
        }
    }
    return nullptr;
}

uint16_t SynthMode::calculateActualFreq(uint16_t baseFreq, int8_t octaveOffset, int16_t pitchBend) {
    if (baseFreq == 0) return 0;

    int32_t freq = baseFreq;
    if (octaveOffset > 0) {
        freq <<= octaveOffset;
    } else if (octaveOffset < 0) {
        freq >>= (-octaveOffset);
    }

    freq += pitchBend;
    if (freq < 30) freq = 30;
    if (freq > 8000) freq = 8000;

    return (uint16_t)freq;
}

void SynthMode::updateOrgan(KeypadDriver& keypad) {
    char currentKey = keypad.getCurrentKey();
    int16_t pitchBend = _pot.getPitchBendHz();

    // 1. Xử lý chuyển Octave khi bấm phím '*' hoặc '#'
    char pressedKey = keypad.getKey();
    if (pressedKey == '*') {
        if (_octaveOffset > -1) {
            _octaveOffset--;
            Serial.printf("[ORGAN] Octave Shift DOWN: %d\n", 4 + _octaveOffset);
        }
    } else if (pressedKey == '#') {
        if (_octaveOffset < 1) {
            _octaveOffset++;
            Serial.printf("[ORGAN] Octave Shift UP: %d\n", 4 + _octaveOffset);
        }
    }

    // 2. Xử lý chơi nốt nhạc (Note Playing)
    if (currentKey != '\0') {
        const NoteDefinition* note = findNote(currentKey);
        if (note != nullptr) {
            if (_activeKey != currentKey || abs(pitchBend - _lastPitchBend) >= 2) {
                _activeKey = currentKey;
                _currentNoteBaseFreq = note->baseFreq;
                _lastPitchBend = pitchBend;

                uint16_t freq = calculateActualFreq(_currentNoteBaseFreq, _octaveOffset, pitchBend);
                _audio.startTone(freq);

                uint8_t effectiveOctave = note->octave + _octaveOffset;
                _display.showNote(note->name, effectiveOctave);

                Serial.printf("[ORGAN] Playing Note %s%d | Base: %dHz | PitchBend: %+dHz | Output: %dHz\n",
                              note->name, effectiveOctave, note->baseFreq, pitchBend, freq);
            }
        }
    } else {
        // Nhả phím -> Dừng âm thanh
        if (_activeKey != '\0') {
            _audio.stopTone();
            _activeKey = '\0';
            _currentNoteBaseFreq = 0;
            _display.showMode(MODE_ORGAN);
            Serial.println(F("[ORGAN] Key Released. Tone Stopped."));
        }
    }
}

void SynthMode::updateSFX(KeypadDriver& keypad) {
    char key = keypad.getKey();
    if (key == '\0') return;

    switch (key) {
        case '1':
            playLaser();
            break;
        case '2':
            playCoin();
            break;
        case '3':
            playJump();
            break;
        case '4':
            playPowerUp();
            break;
        case '5':
            playSiren();
            break;
        default:
            break;
    }
}

void SynthMode::playLaser() {
    _display.displayString("LASE");
    Serial.println(F("[SFX] Playing: Laser Blaster Zap"));
    for (int f = 2000; f > 300; f -= 50) {
        _audio.playTone(f, 5);
        delay(5);
    }
    _audio.stopTone();
    _display.showMode(MODE_SFX);
}

void SynthMode::playCoin() {
    _display.displayString("COIn");
    Serial.println(F("[SFX] Playing: Coin Pickup Ding"));
    _audio.playTone(NOTE_B5, 80);
    delay(85);
    _audio.playTone(NOTE_E6, 250);
    delay(260);
    _audio.stopTone();
    _display.showMode(MODE_SFX);
}

void SynthMode::playJump() {
    _display.displayString("JUIP");
    Serial.println(F("[SFX] Playing: Jump Sound"));
    for (int f = 150; f < 650; f += 25) {
        _audio.playTone(f, 8);
        delay(8);
    }
    _audio.stopTone();
    _display.showMode(MODE_SFX);
}

void SynthMode::playPowerUp() {
    _display.displayString("P-UP");
    Serial.println(F("[SFX] Playing: Power-Up Fanfare"));
    uint16_t notes[] = {NOTE_G4, NOTE_C5, NOTE_E5, NOTE_G5, NOTE_C6};
    for (uint8_t i = 0; i < 5; i++) {
        _audio.playTone(notes[i], 100);
        delay(115);
    }
    _audio.stopTone();
    _display.showMode(MODE_SFX);
}

void SynthMode::playSiren() {
    _display.displayString("SIrE");
    Serial.println(F("[SFX] Playing: Police Siren Alarm"));
    for (int repeat = 0; repeat < 2; repeat++) {
        for (int f = 600; f < 1200; f += 30) {
            _audio.playTone(f, 10);
            delay(10);
        }
        for (int f = 1200; f > 600; f -= 30) {
            _audio.playTone(f, 10);
            delay(10);
        }
    }
    _audio.stopTone();
    _display.showMode(MODE_SFX);
}
