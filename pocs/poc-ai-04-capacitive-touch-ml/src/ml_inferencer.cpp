/**
 * ml_inferencer.cpp
 * Hiện thực bộ nhân ma trận Feed-forward MLP, ReLU và Softmax
 */

#include "ml_inferencer.h"
#include <math.h>

MLInferencer::MLInferencer() {}

GestureType MLInferencer::predict(const float features[MODEL_INPUT_DIM], float* outConfidence, float* outProbs) {
    float h[MODEL_HIDDEN_DIM];

    // 1. Hidden Layer: h = ReLU(W1 * x + B1)
    for (uint8_t i = 0; i < MODEL_HIDDEN_DIM; i++) {
        float sum = MODEL_B1[i];
        for (uint8_t j = 0; j < MODEL_INPUT_DIM; j++) {
            sum += MODEL_W1[i][j] * features[j];
        }
        h[i] = (sum > 0.0f) ? sum : 0.0f; // Hàm kích hoạt ReLU
    }

    // 2. Output Layer: logits = W2 * h + B2
    float logits[MODEL_OUTPUT_DIM];
    float maxLogit = -1e9f;
    for (uint8_t k = 0; k < MODEL_OUTPUT_DIM; k++) {
        float sum = MODEL_B2[k];
        for (uint8_t i = 0; i < MODEL_HIDDEN_DIM; i++) {
            sum += MODEL_W2[k][i] * h[i];
        }
        logits[k] = sum;
        if (sum > maxLogit) {
            maxLogit = sum;
        }
    }

    // 3. Softmax
    float sumExp = 0.0f;
    float probs[MODEL_OUTPUT_DIM];
    for (uint8_t k = 0; k < MODEL_OUTPUT_DIM; k++) {
        probs[k] = expf(logits[k] - maxLogit);
        sumExp += probs[k];
    }
    for (uint8_t k = 0; k < MODEL_OUTPUT_DIM; k++) {
        probs[k] /= sumExp;
        if (outProbs != nullptr) {
            outProbs[k] = probs[k];
        }
    }

    // 4. Tìm lớp có xác suất cao nhất
    uint8_t bestClass = 0;
    float bestProb = probs[0];
    for (uint8_t k = 1; k < MODEL_OUTPUT_DIM; k++) {
        if (probs[k] > bestProb) {
            bestProb = probs[k];
            bestClass = k;
        }
    }

    if (outConfidence != nullptr) {
        *outConfidence = bestProb;
    }

    // Lọc theo ngưỡng tin cậy
    if (bestProb < 0.50f) {
        return GESTURE_IDLE;
    }

    return (GestureType)bestClass;
}

const char* MLInferencer::getGestureName(GestureType gesture) const {
    if (gesture >= 0 && gesture < MODEL_OUTPUT_DIM) {
        return GESTURE_NAMES[gesture];
    }
    return "UNKNOWN";
}
