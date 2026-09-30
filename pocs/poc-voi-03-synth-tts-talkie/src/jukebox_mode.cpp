#include "jukebox_mode.h"

JukeboxMode::JukeboxMode(AudioEngine& audio, TM1637Driver& display, PotentiometerDriver& pot)
    : _audio(audio), _display(display), _pot(pot),
      _state(JUKEBOX_IDLE), _currentSongIndex(0), _currentNoteIndex(0),
      _currentBpm(TEMPO_DEFAULT_BPM), _noteEndTime(0), _pauseEndTime(0), _inNotePause(false) {}

void JukeboxMode::enter() {
    _state = JUKEBOX_IDLE;
    _audio.stopTone();
    _display.showMode(MODE_JUKEBOX);
    Serial.println(F("[MODE] Entered JUKEBOX Mode. Press 1-4 to choose songs, Potentiometer adjusts Tempo BPM."));
}

uint32_t JukeboxMode::calculateNoteDuration(int8_t durationValue, uint16_t bpm) {
    if (bpm == 0) bpm = 120;
    // Thời lượng của một nốt tròn (whole note) tính bằng mili giây
    // wholenote = (60000 * 4) / BPM
    uint32_t wholenote = (60000UL * 4UL) / bpm;

    if (durationValue > 0) {
        // Nốt thường (4: quarter, 8: eighth, 16: sixteenth, 2: half...)
        return wholenote / durationValue;
    } else if (durationValue < 0) {
        // Nốt chấm đôi (dotted note): tăng 1.5 lần độ dài
        uint32_t base = wholenote / (-durationValue);
        return base + (base / 2);
    }
    return 200;
}

void JukeboxMode::playSong(uint8_t songIndex) {
    if (songIndex >= TOTAL_SONGS) return;

    _currentSongIndex = songIndex;
    _currentNoteIndex = 0;
    _currentBpm = _pot.getTempoBpm(); // Lấy BPM tức thời từ biến trở
    _inNotePause = false;
    _state = JUKEBOX_PLAYING;

    _display.showSong(_currentSongIndex + 1);
    Serial.printf("[JUKEBOX] Started Song #%d: \"%s\" at %d BPM (%d notes)\n",
                  _currentSongIndex + 1, SONG_CATALOG[_currentSongIndex].title,
                  _currentBpm, SONG_CATALOG[_currentSongIndex].length);

    // Kích hoạt ngay nốt đầu tiên
    _noteEndTime = millis();
}

void JukeboxMode::pauseResume() {
    if (_state == JUKEBOX_PLAYING) {
        _state = JUKEBOX_PAUSED;
        _audio.stopTone();
        _display.displayString("PAUS");
        Serial.println(F("[JUKEBOX] Playback PAUSED."));
    } else if (_state == JUKEBOX_PAUSED) {
        _state = JUKEBOX_PLAYING;
        _display.showSong(_currentSongIndex + 1);
        _noteEndTime = millis(); // Tiếp tục phát
        Serial.println(F("[JUKEBOX] Playback RESUMED."));
    }
}

void JukeboxMode::stop() {
    _state = JUKEBOX_IDLE;
    _audio.stopTone();
    _currentNoteIndex = 0;
    _display.showMode(MODE_JUKEBOX);
    Serial.println(F("[JUKEBOX] Playback STOPPED."));
}

void JukeboxMode::processPlayback() {
    if (_state != JUKEBOX_PLAYING) return;

    uint32_t now = millis();

    // 1. Kiểm tra nếu đang trong khoảng nghỉ siêu ngắn giữa 2 nốt (Articulated Pause 10%)
    if (_inNotePause) {
        if (now >= _pauseEndTime) {
            _inNotePause = false;
            _currentNoteIndex++;

            if (_currentNoteIndex >= SONG_CATALOG[_currentSongIndex].length) {
                // Đã phát hết bài
                Serial.printf("[JUKEBOX] Finished Song #%d!\n", _currentSongIndex + 1);
                stop();
                return;
            }
        } else {
            return;
        }
    }

    // 2. Kiểm tra nếu nốt hiện tại đã ngân xong
    if (now >= _noteEndTime) {
        const MelodyNote& note = SONG_CATALOG[_currentSongIndex].notes[_currentNoteIndex];
        uint32_t fullDuration = calculateNoteDuration(note.duration, _currentBpm);

        // 90% thời lượng là phát âm, 10% thời lượng ngắt âm để tách bạch các nốt liền kề
        uint32_t playDuration = (fullDuration * 9) / 10;
        uint32_t pauseDuration = fullDuration - playDuration;

        if (note.freq > 0) {
            _audio.playTone(note.freq, playDuration);
        } else {
            _audio.stopTone(); // Dấu lặng
        }

        _noteEndTime = now + playDuration;
        _pauseEndTime = now + fullDuration;
        _inNotePause = true;
    }
}

void JukeboxMode::update(KeypadDriver& keypad) {
    // 1. Cập nhật Tempo real-time từ biến trở khi đang chơi
    if (_pot.hasChanged()) {
        uint16_t newBpm = _pot.getTempoBpm();
        if (abs((int)newBpm - (int)_currentBpm) >= 2) {
            _currentBpm = newBpm;
            _display.showBpm(_currentBpm);
            Serial.printf("[JUKEBOX] Tempo adjusted to: %d BPM\n", _currentBpm);
        }
    }

    // 2. Xử lý phím bấm điều khiển
    char key = keypad.getKey();
    if (key != '\0') {
        if (key >= '1' && key <= '4') {
            uint8_t songIdx = key - '1';
            playSong(songIdx);
        } else if (key == '*') {
            pauseResume();
        } else if (key == '#') {
            stop();
        }
    }

    // 3. Tiến trình phát nhạc Non-blocking
    processPlayback();
}

bool JukeboxMode::isPlaying() const {
    return (_state == JUKEBOX_PLAYING);
}

uint8_t JukeboxMode::getCurrentSong() const {
    return _currentSongIndex;
}

uint16_t JukeboxMode::getCurrentBpm() const {
    return _currentBpm;
}
