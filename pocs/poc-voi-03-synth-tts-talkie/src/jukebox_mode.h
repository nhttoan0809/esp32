#ifndef VOI03_JUKEBOX_MODE_H
#define VOI03_JUKEBOX_MODE_H

#include <Arduino.h>
#include "config.h"
#include "audio_engine.h"
#include "tm1637_driver.h"
#include "pot_driver.h"
#include "keypad_driver.h"
#include "songs_data.h"

enum JukeboxState {
    JUKEBOX_IDLE = 0,
    JUKEBOX_PLAYING,
    JUKEBOX_PAUSED
};

class JukeboxMode {
public:
    JukeboxMode(AudioEngine& audio, TM1637Driver& display, PotentiometerDriver& pot);

    void enter();
    void update(KeypadDriver& keypad);

    void playSong(uint8_t songIndex);
    void pauseResume();
    void stop();

    bool isPlaying() const;
    uint8_t getCurrentSong() const;
    uint16_t getCurrentBpm() const;

private:
    AudioEngine& _audio;
    TM1637Driver& _display;
    PotentiometerDriver& _pot;

    JukeboxState _state;
    uint8_t _currentSongIndex;
    uint16_t _currentNoteIndex;
    uint16_t _currentBpm;
    uint32_t _noteEndTime;
    uint32_t _pauseEndTime;
    bool _inNotePause;

    void processPlayback();
    uint32_t calculateNoteDuration(int8_t durationValue, uint16_t bpm);
};

#endif // VOI03_JUKEBOX_MODE_H
