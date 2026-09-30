#include "audio_engine.h"

AudioEngine::AudioEngine(uint8_t pin)
    : _pin(pin), _mode(AUDIO_MODE_PWM_TONE), _currentFreq(0),
      _isPlaying(false), _toneEndTime(0) {}

void AudioEngine::begin() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    _mode = AUDIO_MODE_PWM_TONE;
    _isPlaying = false;
    _currentFreq = 0;
    _toneEndTime = 0;
}

void AudioEngine::update() {
    if (_mode != AUDIO_MODE_PWM_TONE) {
        return;
    }

    if (_isPlaying && _toneEndTime > 0) {
        if (millis() >= _toneEndTime) {
            stopTone();
        }
    }
}

void AudioEngine::startTone(uint16_t frequency) {
    if (_mode != AUDIO_MODE_PWM_TONE) {
        resumePWM();
    }

    if (frequency == 0) {
        stopTone();
        return;
    }

    _currentFreq = frequency;
    _isPlaying = true;
    _toneEndTime = 0; // Phát liên tục cho đến khi có lệnh dừng

    tone(_pin, frequency);
}

void AudioEngine::playTone(uint16_t frequency, uint32_t durationMs) {
    if (_mode != AUDIO_MODE_PWM_TONE) {
        resumePWM();
    }

    if (frequency == 0 || durationMs == 0) {
        stopTone();
        return;
    }

    _currentFreq = frequency;
    _isPlaying = true;
    _toneEndTime = millis() + durationMs;

    tone(_pin, frequency);
}

void AudioEngine::stopTone() {
    if (_isPlaying) {
        noTone(_pin);
        digitalWrite(_pin, LOW);
        _isPlaying = false;
        _currentFreq = 0;
        _toneEndTime = 0;
    }
}

bool AudioEngine::isPlaying() const {
    return _isPlaying;
}

uint16_t AudioEngine::getCurrentFrequency() const {
    return _currentFreq;
}

void AudioEngine::prepareForDAC() {
    stopTone();
    // Giải phóng kênh LEDC PWM trên chân GPIO 25
    pinMode(_pin, INPUT);
    _mode = AUDIO_MODE_DAC_SPEECH;
    delay(10); // Đảm bảo phần cứng chuyển đổi ổn định
}

void AudioEngine::resumePWM() {
    if (_mode == AUDIO_MODE_DAC_SPEECH) {
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
        _mode = AUDIO_MODE_PWM_TONE;
        delay(5);
    }
}

AudioOutputMode AudioEngine::getOutputMode() const {
    return _mode;
}
