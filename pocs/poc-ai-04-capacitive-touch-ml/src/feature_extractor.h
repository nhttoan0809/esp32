/**
 * feature_extractor.h
 * Quản lý bộ đệm cửa sổ trượt (Sliding Window) và trích xuất vector đặc trưng 8 chiều
 */

#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include "config.h"
#include "touch_sampler.h"

struct TouchFrame {
    int deltas[NUM_TOUCH_PADS];
    bool anyTouched;
};

class FeatureExtractor {
public:
    FeatureExtractor();
    void reset();
    bool pushFrame(const TouchSampler& sampler); // Trả về true khi phát hiện kết thúc 1 cử chỉ cần suy luận
    void extractFeatures(float outFeatures[8]);
    bool isGestureActive() const { return gestureActive; }

private:
    TouchFrame window[WINDOW_SIZE];
    uint8_t writeIndex;
    uint8_t totalFrames;
    bool gestureActive;
    uint8_t quietFrames;
    uint8_t activeFramesCount;

    int countBursts() const;
};

#endif // FEATURE_EXTRACTOR_H
