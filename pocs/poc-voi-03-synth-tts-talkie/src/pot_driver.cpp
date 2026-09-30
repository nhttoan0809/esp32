#include "pot_driver.h"

PotentiometerDriver::PotentiometerDriver(uint8_t pin)
    : _pin(pin), _sampleIndex(0), _sampleSum(0),
      _filteredValue(0), _lastReportedValue(0), _changed(false), _lastReadTime(0) {}

void PotentiometerDriver::begin() {
    pinMode(_pin, INPUT);
    analogReadResolution(POT_ADC_RESOLUTION);
    analogSetAttenuation(ADC_11db); // Dải đo 0 - 3.3V

    // Khởi tạo ban đầu toàn bộ mảng mẫu với giá trị đọc thực tế
    uint16_t initial = analogRead(_pin);
    for (uint8_t i = 0; i < POT_SMA_SAMPLES; i++) {
        _samples[i] = initial;
    }
    _sampleSum = (uint32_t)initial * POT_SMA_SAMPLES;
    _filteredValue = initial;
    _lastReportedValue = initial;
    _changed = false;
}

void PotentiometerDriver::update() {
    uint32_t now = millis();
    // Đọc mỗi 20ms (tần số lấy mẫu 50Hz, vừa đủ cho biến trở)
    if (now - _lastReadTime < 20) {
        _changed = false;
        return;
    }
    _lastReadTime = now;

    uint16_t raw = analogRead(_pin);

    // Cập nhật bộ lọc trung bình trượt SMA
    _sampleSum -= _samples[_sampleIndex];
    _samples[_sampleIndex] = raw;
    _sampleSum += raw;
    _sampleIndex = (_sampleIndex + 1) % POT_SMA_SAMPLES;

    _filteredValue = _sampleSum / POT_SMA_SAMPLES;

    // Kiểm tra biến động với ngưỡng Deadband chống rung
    int diff = abs((int)_filteredValue - (int)_lastReportedValue);
    if (diff >= POT_DEADBAND) {
        _lastReportedValue = _filteredValue;
        _changed = true;
    } else {
        _changed = false;
    }
}

uint16_t PotentiometerDriver::getRaw() const {
    return analogRead(_pin);
}

uint16_t PotentiometerDriver::getFiltered() const {
    return _filteredValue;
}

int16_t PotentiometerDriver::getPitchBendHz() const {
    // 0 -> -50Hz, 2048 -> 0Hz, 4095 -> +50Hz
    // Vùng chết trung tâm [1950, 2150] trả về 0Hz để dễ trả về nốt chuẩn
    if (_filteredValue >= 1950 && _filteredValue <= 2150) {
        return 0;
    }

    if (_filteredValue < 1950) {
        return map(_filteredValue, 0, 1950, PITCH_BEND_MIN, 0);
    } else {
        return map(_filteredValue, 2150, 4095, 0, PITCH_BEND_MAX);
    }
}

uint16_t PotentiometerDriver::getTempoBpm() const {
    // Ánh xạ dải ADC (0 - 4095) thành nhịp Tempo BPM (60 - 240)
    return map(_filteredValue, 0, 4095, TEMPO_MIN_BPM, TEMPO_MAX_BPM);
}

bool PotentiometerDriver::hasChanged() const {
    return _changed;
}
