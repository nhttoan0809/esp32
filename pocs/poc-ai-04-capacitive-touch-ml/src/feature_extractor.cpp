/**
 * feature_extractor.cpp
 * Hiện thực bộ đệm cửa sổ trượt và thuật toán trích xuất đặc trưng
 */

#include "feature_extractor.h"

FeatureExtractor::FeatureExtractor() {
    reset();
}

void FeatureExtractor::reset() {
    writeIndex = 0;
    totalFrames = 0;
    gestureActive = false;
    quietFrames = 0;
    activeFramesCount = 0;
    for (uint8_t i = 0; i < WINDOW_SIZE; i++) {
        for (uint8_t p = 0; p < NUM_TOUCH_PADS; p++) {
            window[i].deltas[p] = 0;
        }
        window[i].anyTouched = false;
    }
}

bool FeatureExtractor::pushFrame(const TouchSampler& sampler) {
    // Lưu khung hình hiện tại vào buffer
    TouchFrame& frame = window[writeIndex];
    for (uint8_t p = 0; p < NUM_TOUCH_PADS; p++) {
        frame.deltas[p] = sampler.getDelta(p);
    }
    frame.anyTouched = sampler.anyPadTouched();

    writeIndex = (writeIndex + 1) % WINDOW_SIZE;
    if (totalFrames < WINDOW_SIZE) {
        totalFrames++;
    }

    bool touchedNow = frame.anyTouched;

    if (touchedNow) {
        if (!gestureActive) {
            gestureActive = true;
            activeFramesCount = 1;
            quietFrames = 0;
        } else {
            activeFramesCount++;
            quietFrames = 0;
        }

        // Nếu chạm giữ liên tục gần đầy cửa sổ (Hold gesture)
        if (activeFramesCount >= (WINDOW_SIZE - 2)) {
            gestureActive = false;
            return true;
        }
    } else {
        if (gestureActive) {
            quietFrames++;
            // Nếu người dùng đã nhấc tay (yên lặng 4 khung = 80ms) và đã có đủ mẫu chạm
            if (quietFrames >= 4 && activeFramesCount >= 3) {
                gestureActive = false;
                return true;
            }
            if (quietFrames >= 6) {
                gestureActive = false;
                activeFramesCount = 0;
            }
        }
    }

    return false;
}

int FeatureExtractor::countBursts() const {
    if (totalFrames < 2) return 0;
    int bursts = 0;
    bool inTouch = false;

    for (uint8_t i = 0; i < totalFrames; i++) {
        uint8_t idx = (totalFrames == WINDOW_SIZE) ? ((writeIndex + i) % WINDOW_SIZE) : i;
        bool touched = window[idx].anyTouched;
        if (touched && !inTouch) {
            bursts++;
            inTouch = true;
        } else if (!touched) {
            inTouch = false;
        }
    }
    return bursts;
}

void FeatureExtractor::extractFeatures(float outFeatures[8]) {
    uint8_t N = totalFrames;
    if (N == 0) {
        for (int i = 0; i < 8; i++) outFeatures[i] = 0.0f;
        return;
    }

    // 1. Tìm đỉnh của từng pad (f0 .. f3)
    long energyLeft = 0;
    long energyRight = 0;

    for (uint8_t p = 0; p < NUM_TOUCH_PADS; p++) {
        int maxVal = 0;
        int maxIdx = -1;

        for (uint8_t i = 0; i < N; i++) {
            uint8_t idx = (N == WINDOW_SIZE) ? ((writeIndex + i) % WINDOW_SIZE) : i;
            int delta = window[idx].deltas[p];
            if (delta > maxVal) {
                maxVal = delta;
                maxIdx = i;
            }
            if (p < 2) {
                energyLeft += delta;
            } else {
                energyRight += delta;
            }
        }

        if (maxVal < TOUCH_DELTA_MIN) {
            outFeatures[p] = -1.0f;
        } else {
            outFeatures[p] = (float)maxIdx / (float)(WINDOW_SIZE - 1);
        }
    }

    // 2. f4: Bất đối xứng năng lượng trái - phải (-1.0 .. 1.0)
    float totalEnergy = (float)(energyLeft + energyRight);
    outFeatures[4] = (float)(energyLeft - energyRight) / (totalEnergy + 1.0f);

    // 3. f5: Hướng lan truyền vận tốc đỉnh (Swipe gradient)
    float t0 = outFeatures[0];
    float t1 = outFeatures[1];
    float t2 = outFeatures[2];
    float t3 = outFeatures[3];
    float gradSum = 0.0f;
    int pairs = 0;

    if (t0 >= 0 && t3 >= 0) { gradSum += (t3 - t0); pairs++; }
    if (t1 >= 0 && t2 >= 0) { gradSum += (t2 - t1); pairs++; }
    if (t0 >= 0 && t2 >= 0) { gradSum += (t2 - t0); pairs++; }
    if (t1 >= 0 && t3 >= 0) { gradSum += (t3 - t1); pairs++; }

    if (pairs > 0) {
        float rawGrad = (gradSum / pairs) * 1.5f;
        if (rawGrad > 1.0f) rawGrad = 1.0f;
        if (rawGrad < -1.0f) rawGrad = -1.0f;
        outFeatures[5] = rawGrad;
    } else {
        outFeatures[5] = 0.0f;
    }

    // 4. f6: Số xung nhấp nhả (Peak count / Double-Tap detection)
    int bursts = countBursts();
    outFeatures[6] = (float)bursts / 3.0f;

    // 5. f7: Tỷ lệ giữ chạm (Hold ratio)
    outFeatures[7] = (float)activeFramesCount / (float)WINDOW_SIZE;
}
