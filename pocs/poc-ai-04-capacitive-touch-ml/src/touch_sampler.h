/**
 * touch_sampler.h
 * Thu thập dữ liệu cảm ứng điện dung 4 kênh, tự động hiệu chuẩn baseline
 */

#ifndef TOUCH_SAMPLER_H
#define TOUCH_SAMPLER_H

#include "config.h"

class TouchSampler {
public:
    TouchSampler();
    void begin();
    bool update(); // Trả về true nếu đã lấy mẫu mới (đúng chu kỳ 20ms)

    int getRaw(uint8_t padIndex) const;
    int getBaseline(uint8_t padIndex) const;
    int getDelta(uint8_t padIndex) const;
    bool isPadTouched(uint8_t padIndex) const;
    bool anyPadTouched() const;

private:
    const uint8_t pins[NUM_TOUCH_PADS] = {
        TOUCH_PIN_0, TOUCH_PIN_1, TOUCH_PIN_2, TOUCH_PIN_3
    };
    int baselines[NUM_TOUCH_PADS];
    int rawValues[NUM_TOUCH_PADS];
    int deltaValues[NUM_TOUCH_PADS];
    bool padTouched[NUM_TOUCH_PADS];
    unsigned long lastSampleTime;

    void calibrate();
};

#endif // TOUCH_SAMPLER_H
