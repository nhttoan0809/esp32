/**
 * ml_inferencer.h
 * Bộ suy luận mô hình TinyML trên vi điều khiển ESP32
 */

#ifndef ML_INFERENCER_H
#define ML_INFERENCER_H

#include "config.h"
#include "touch_model_weights.h"

class MLInferencer {
public:
    MLInferencer();
    GestureType predict(const float features[MODEL_INPUT_DIM], float* outConfidence = nullptr, float* outProbs = nullptr);
    const char* getGestureName(GestureType gesture) const;
};

#endif // ML_INFERENCER_H
