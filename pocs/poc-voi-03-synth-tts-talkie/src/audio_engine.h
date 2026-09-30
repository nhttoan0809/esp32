#ifndef VOI03_AUDIO_ENGINE_H
#define VOI03_AUDIO_ENGINE_H

#include <Arduino.h>
#include "config.h"

enum AudioOutputMode {
    AUDIO_MODE_PWM_TONE = 0, // Đang dùng xung vuông LEDC Tone
    AUDIO_MODE_DAC_SPEECH    // Đang nhường chân GPIO 25 cho DAC Talkie
};

class AudioEngine {
public:
    AudioEngine(uint8_t pin = PIN_BUZZER);

    void begin();
    void update(); // Quản lý thời lượng nốt phi khóa

    // Các hàm phát âm thanh Tone PWM
    void startTone(uint16_t frequency);
    void playTone(uint16_t frequency, uint32_t durationMs);
    void stopTone();

    bool isPlaying() const;
    uint16_t getCurrentFrequency() const;

    // Cơ chế chuyển giao tài nguyên phần cứng GPIO 25 (LEDC <-> DAC1)
    void prepareForDAC();
    void resumePWM();
    AudioOutputMode getOutputMode() const;

private:
    uint8_t _pin;
    AudioOutputMode _mode;
    uint16_t _currentFreq;
    bool _isPlaying;
    uint32_t _toneEndTime;
};

#endif // VOI03_AUDIO_ENGINE_H
