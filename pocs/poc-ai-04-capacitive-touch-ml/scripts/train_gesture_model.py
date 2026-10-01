#!/usr/bin/env python3
"""
train_gesture_model.py
Huấn luyện mô hình TinyML nhận diện cử chỉ cảm ứng điện dung cho ESP32 AI-04.
Không phụ thuộc thư viện ngoài (dùng Python standard library: math, random).
Tự động sinh mã C++ Header: include/touch_model_weights.h
"""

import math
import random
import os
import sys

# Các hằng số mô hình
INPUT_DIM = 8
HIDDEN_DIM = 16
NUM_CLASSES = 5
CLASS_NAMES = ["IDLE", "SWIPE_RIGHT", "SWIPE_LEFT", "DOUBLE_TAP", "HOLD"]

def generate_sample(class_id):
    """
    Sinh 1 mẫu vector đặc trưng 8 chiều tương ứng với cử chỉ:
    f0: peak_t0 (-1..1)
    f1: peak_t1 (-1..1)
    f2: peak_t2 (-1..1)
    f3: peak_t3 (-1..1)
    f4: energy_asym (-1..1)
    f5: swipe_gradient (-1..1)
    f6: peak_count (0..1)
    f7: hold_ratio (0..1)
    """
    noise = lambda: random.gauss(0, 0.05)
    
    if class_id == 0:  # IDLE
        return [
            -1.0 + random.uniform(0, 0.05),
            -1.0 + random.uniform(0, 0.05),
            -1.0 + random.uniform(0, 0.05),
            -1.0 + random.uniform(0, 0.05),
            0.0 + noise(),
            0.0 + noise(),
            0.0,
            0.0 + random.uniform(0, 0.05)
        ]
    elif class_id == 1:  # SWIPE_RIGHT (0 -> 1 -> 2 -> 3)
        t0 = 0.15 + random.uniform(-0.05, 0.05)
        t1 = 0.35 + random.uniform(-0.05, 0.05)
        t2 = 0.55 + random.uniform(-0.05, 0.05)
        t3 = 0.75 + random.uniform(-0.05, 0.05)
        grad = (t3 + t2 - t1 - t0) * 1.2
        return [
            t0, t1, t2, t3,
            -0.2 + noise(),  # Năng lượng dịch chuyển từ trái qua phải
            min(1.0, max(0.4, grad + noise())),
            1.0 / 3.0 + noise() * 0.1,  # 1 peak
            0.35 + noise()
        ]
    elif class_id == 2:  # SWIPE_LEFT (3 -> 2 -> 1 -> 0)
        t3 = 0.15 + random.uniform(-0.05, 0.05)
        t2 = 0.35 + random.uniform(-0.05, 0.05)
        t1 = 0.55 + random.uniform(-0.05, 0.05)
        t0 = 0.75 + random.uniform(-0.05, 0.05)
        grad = (t0 + t1 - t2 - t3) * -1.2
        return [
            t0, t1, t2, t3,
            0.2 + noise(),
            max(-1.0, min(-0.4, grad + noise())),
            1.0 / 3.0 + noise() * 0.1,
            0.35 + noise()
        ]
    elif class_id == 3:  # DOUBLE_TAP
        t = random.uniform(0.2, 0.7)
        return [
            t + noise() * 0.2,
            t + noise() * 0.2,
            -1.0 if random.random() > 0.5 else t,
            -1.0 if random.random() > 0.5 else t,
            0.0 + noise(),
            0.0 + noise() * 0.2,
            2.0 / 3.0 + noise() * 0.05,  # 2 peaks!
            0.3 + noise()
        ]
    elif class_id == 4:  # HOLD
        t = random.uniform(0.1, 0.4)
        return [
            t + noise(),
            t + noise() if random.random() > 0.4 else -1.0,
            -1.0 if random.random() > 0.6 else t,
            -1.0 if random.random() > 0.6 else t,
            random.uniform(-0.3, 0.3),
            0.0 + noise() * 0.1,
            1.0 / 3.0 + noise() * 0.05,
            0.75 + random.uniform(0.05, 0.2)  # High hold ratio!
        ]

def init_network():
    # He initialization
    w1 = [[random.gauss(0, math.sqrt(2.0 / INPUT_DIM)) for _ in range(INPUT_DIM)] for _ in range(HIDDEN_DIM)]
    b1 = [0.0 for _ in range(HIDDEN_DIM)]
    w2 = [[random.gauss(0, math.sqrt(2.0 / HIDDEN_DIM)) for _ in range(HIDDEN_DIM)] for _ in range(NUM_CLASSES)]
    b2 = [0.0 for _ in range(NUM_CLASSES)]
    return w1, b1, w2, b2

def forward(x, w1, b1, w2, b2):
    # Hidden layer: ReLU
    h = []
    for i in range(HIDDEN_DIM):
        val = b1[i]
        for j in range(INPUT_DIM):
            val += w1[i][j] * x[j]
        h.append(max(0.0, val))
        
    # Output layer: logits
    logits = []
    for k in range(NUM_CLASSES):
        val = b2[k]
        for i in range(HIDDEN_DIM):
            val += w2[k][i] * h[i]
        logits.append(val)
        
    # Softmax with numerical stability
    max_logit = max(logits)
    exp_logits = [math.exp(l - max_logit) for l in logits]
    sum_exp = sum(exp_logits)
    probs = [e / sum_exp for e in exp_logits]
    return h, logits, probs

def train():
    random.seed(42)
    w1, b1, w2, b2 = init_network()
    
    # Sinh dữ liệu huấn luyện
    samples_per_class = 400
    dataset = []
    for c in range(NUM_CLASSES):
        for _ in range(samples_per_class):
            x = generate_sample(c)
            dataset.append((x, c))
            
    # Xáo trộn
    random.shuffle(dataset)
    split = int(len(dataset) * 0.8)
    train_data = dataset[:split]
    test_data = dataset[split:]
    
    lr = 0.02
    epochs = 120
    
    print(f"Bắt đầu huấn luyện MLP TinyML ({len(train_data)} train samples, {len(test_data)} test samples)...")
    
    for epoch in range(epochs):
        total_loss = 0.0
        random.shuffle(train_data)
        for x, y in train_data:
            h, logits, probs = forward(x, w1, b1, w2, b2)
            
            # Cross-entropy loss
            loss = -math.log(max(1e-15, probs[y]))
            total_loss += loss
            
            # Gradients output layer
            d_logits = [probs[k] - (1.0 if k == y else 0.0) for k in range(NUM_CLASSES)]
            
            # Gradients w2, b2, and backprop to hidden
            d_h = [0.0 for _ in range(HIDDEN_DIM)]
            for k in range(NUM_CLASSES):
                b2[k] -= lr * d_logits[k]
                for i in range(HIDDEN_DIM):
                    w2[k][i] -= lr * d_logits[k] * h[i]
                    d_h[i] += d_logits[k] * w2[k][i]
                    
            # Gradients hidden layer (ReLU)
            for i in range(HIDDEN_DIM):
                if h[i] > 0.0:
                    b1[i] -= lr * d_h[i]
                    for j in range(INPUT_DIM):
                        w1[i][j] -= lr * d_h[i] * x[j]
                        
        if (epoch + 1) % 30 == 0:
            avg_loss = total_loss / len(train_data)
            print(f"Epoch {epoch+1:3d}/{epochs} | Train Loss: {avg_loss:.4f}")
            
    # Đánh giá trên tập test
    correct = 0
    confusion = [[0 for _ in range(NUM_CLASSES)] for _ in range(NUM_CLASSES)]
    for x, y in test_data:
        _, _, probs = forward(x, w1, b1, w2, b2)
        pred = probs.index(max(probs))
        confusion[y][pred] += 1
        if pred == y:
            correct += 1
            
    acc = correct / len(test_data) * 100.0
    print(f"\n=> Kết quả Test Accuracy: {acc:.2f}% ({correct}/{len(test_data)})")
    print("Confusion Matrix (Hàng: Thực tế, Cột: Dự đoán):")
    header = "          " + " ".join([f"{name[:6]:>7}" for name in CLASS_NAMES])
    print(header)
    for c in range(NUM_CLASSES):
        row = f"{CLASS_NAMES[c][:8]:>8}: " + " ".join([f"{confusion[c][p]:7d}" for p in range(NUM_CLASSES)])
        print(row)
        
    return w1, b1, w2, b2

def export_c_header(w1, b1, w2, b2, out_path):
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("/**\n")
        f.write(" * touch_model_weights.h\n")
        f.write(" * Tự động sinh bởi scripts/train_gesture_model.py\n")
        f.write(" * Kiến trúc mạng: MLP 8 -> 16 (ReLU) -> 5 (Softmax)\n")
        f.write(" */\n\n")
        f.write("#ifndef TOUCH_MODEL_WEIGHTS_H\n")
        f.write("#define TOUCH_MODEL_WEIGHTS_H\n\n")
        f.write("#include <Arduino.h>\n\n")
        f.write(f"#define MODEL_INPUT_DIM {INPUT_DIM}\n")
        f.write(f"#define MODEL_HIDDEN_DIM {HIDDEN_DIM}\n")
        f.write(f"#define MODEL_OUTPUT_DIM {NUM_CLASSES}\n\n")
        
        # W1
        f.write(f"static const float PROGMEM MODEL_W1[{HIDDEN_DIM}][{INPUT_DIM}] = {{\n")
        for i in range(HIDDEN_DIM):
            row = ", ".join([f"{w1[i][j]:.6f}f" for j in range(INPUT_DIM)])
            f.write(f"    {{ {row} }},\n")
        f.write("};\n\n")
        
        # B1
        f.write(f"static const float PROGMEM MODEL_B1[{HIDDEN_DIM}] = {{\n")
        f.write("    " + ", ".join([f"{b1[i]:.6f}f" for i in range(HIDDEN_DIM)]) + "\n")
        f.write("};\n\n")
        
        # W2
        f.write(f"static const float PROGMEM MODEL_W2[{NUM_CLASSES}][{HIDDEN_DIM}] = {{\n")
        for k in range(NUM_CLASSES):
            row = ", ".join([f"{w2[k][i]:.6f}f" for i in range(HIDDEN_DIM)])
            f.write(f"    {{ {row} }},\n")
        f.write("};\n\n")
        
        # B2
        f.write(f"static const float PROGMEM MODEL_B2[{NUM_CLASSES}] = {{\n")
        f.write("    " + ", ".join([f"{b2[k]:.6f}f" for k in range(NUM_CLASSES)]) + "\n")
        f.write("};\n\n")
        
        f.write("static const char* const GESTURE_NAMES[MODEL_OUTPUT_DIM] = {\n")
        for name in CLASS_NAMES:
            f.write(f'    "{name}",\n')
        f.write("};\n\n")
        f.write("#endif // TOUCH_MODEL_WEIGHTS_H\n")
        
    print(f"\n=> Đã xuất weights sang: {out_path}")

if __name__ == "__main__":
    out_file = os.path.join(os.path.dirname(os.path.dirname(__file__)), "include", "touch_model_weights.h")
    if len(sys.argv) > 1:
        out_file = sys.argv[1]
    w1, b1, w2, b2 = train()
    export_c_header(w1, b1, w2, b2, out_file)
