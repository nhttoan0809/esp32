/**
 * touch_sampler.cpp
 * Hiện thực lớp thu thập và chuẩn hóa dữ liệu cảm ứng
 */

#include "touch_sampler.h"

TouchSampler::TouchSampler() : lastSampleTime(0) {
    for (uint8_t i = 0; i < NUM_TOUCH_PADS; i++) {
        baselines[i] = 80;
        rawValues[i] = 80;
        deltaValues[i] = 0;
        padTouched[i] = false;
    }
}

void TouchSampler::begin() {
    calibrate();
    lastSampleTime = millis();
}

void TouchSampler::calibrate() {
    Serial.println(F("[CALIB] Starting capacitive touch baseline calibration..."));
    long sums[NUM_TOUCH_PADS] = {0, 0, 0, 0};

    // Thu thập các mẫu đầu tiên khi không chạm
    for (int cycle = 0; cycle < CALIBRATION_CYCLES; cycle++) {
        for (uint8_t i = 0; i < NUM_TOUCH_PADS; i++) {
            sums[i] += touchRead(pins[i]);
        }
        delay(5);
    }

    for (uint8_t i = 0; i < NUM_TOUCH_PADS; i++) {
        int avg = sums[i] / CALIBRATION_CYCLES;
        // Nếu môi trường mô phỏng hoặc nhiễu trả về giá trị quá thấp, gán baseline an toàn
        if (avg < 30) {
            avg = 80;
        }
        baselines[i] = avg;
        Serial.printf("[CALIB] Pad %d (GPIO%2d): Baseline = %d\n", i, pins[i], baselines[i]);
    }
    Serial.println(F("[CALIB] Calibration complete. Ready for gestures."));
}

bool TouchSampler::update() {
    unsigned long now = millis();
    if (now - lastSampleTime < SAMPLING_PERIOD_MS) {
        return false;
    }
    lastSampleTime = now;

    for (uint8_t i = 0; i < NUM_TOUCH_PADS; i++) {
        int raw = touchRead(pins[i]);
        rawValues[i] = raw;

        // Tính độ sụt giảm giá trị (Delta)
        int delta = (baselines[i] > raw) ? (baselines[i] - raw) : 0;
        deltaValues[i] = delta;

        // Tiêu chuẩn phát hiện chạm: Delta vượt 30% baseline hoặc raw cực thấp (< 20)
        int threshold = (baselines[i] * 30) / 100;
        if (threshold < TOUCH_DELTA_MIN) {
            threshold = TOUCH_DELTA_MIN;
        }

        padTouched[i] = (delta >= threshold) || (raw <= 20);
    }

    return true;
}

int TouchSampler::getRaw(uint8_t padIndex) const {
    if (padIndex < NUM_TOUCH_PADS) return rawValues[padIndex];
    return 0;
}

int TouchSampler::getBaseline(uint8_t padIndex) const {
    if (padIndex < NUM_TOUCH_PADS) return baselines[padIndex];
    return 80;
}

int TouchSampler::getDelta(uint8_t padIndex) const {
    if (padIndex < NUM_TOUCH_PADS) return deltaValues[padIndex];
    return 0;
}

bool TouchSampler::isPadTouched(uint8_t padIndex) const {
    if (padIndex < NUM_TOUCH_PADS) return padTouched[padIndex];
    return false;
}

bool TouchSampler::anyPadTouched() const {
    for (uint8_t i = 0; i < NUM_TOUCH_PADS; i++) {
        if (padTouched[i]) return true;
    }
    return false;
}
