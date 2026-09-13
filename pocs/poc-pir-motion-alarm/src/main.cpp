#include <Arduino.h>

/**
 * POC: PIR Motion Sensor Controller (Auto-Off Lighting & Security Alarm)
 * Board: ESP32 DevKit V1 (30 chân)
 * 
 * Sơ đồ chân:
 * - PIR OUT     -> GPIO 33 (Digital Input, 3.3V TTL)
 * - Mode Button -> GPIO 4  (INPUT_PULLUP, nhấn = LOW)
 * - Light LED   -> GPIO 21 (Qua trở 220 Ohm)
 * - Buzzer      -> GPIO 22 (Hỗ trợ cả Active và Passive Buzzer qua tone())
 * - PIR VCC     -> VIN (5V từ USB)
 */

static const uint8_t PIN_PIR_IN    = 33;
static const uint8_t PIN_BTN_MODE  = 4;
static const uint8_t PIN_LED_LIGHT = 21;
static const uint8_t PIN_BUZZER    = 22;

enum SystemMode {
  MODE_AUTO_LIGHT = 0,    // Chế độ đèn tự động tiết kiệm điện
  MODE_ARMED_SECURITY = 1 // Chế độ báo động an ninh
};

enum SensitivityProfile {
  SENS_NEAR_3M = 1, // Cự ly gần (~3m): Lọc nhiễu cao, yêu cầu tín hiệu >= 400ms
  SENS_MED_5M  = 2, // Cự ly tiêu chuẩn (~5m): Yêu cầu tín hiệu >= 150ms (Mặc định)
  SENS_FAR_7M  = 3  // Cự ly xa (~7m): Độ nhạy cao, tức thời >= 40ms
};

static SystemMode currentMode = MODE_AUTO_LIGHT;
static SensitivityProfile currentSens = SENS_MED_5M;

// Thời gian cấu hình
static const unsigned long WARMUP_DURATION_MS = 10000; // 10s khởi động nhiệt ban đầu
static const unsigned long HOLD_TIME_MS       = 8000;  // 8s duy trì đèn phần mềm sau khi hết người
static const unsigned long DEBOUNCE_MS        = 50;
static const unsigned long LONG_PRESS_MS      = 1200;  // Nhấn giữ > 1.2s để đổi mức độ nhạy

static unsigned long bootTime = 0;
static unsigned long lastMotionTime = 0;
static unsigned long lastLogTime = 0;
static bool isLightOn = false;

// Đo đạc thông số phần cứng PIR (Núm Tx & Sx)
static bool lastPirRawState = false;
static unsigned long pirPulseStartTime = 0;
static unsigned long lastMeasuredHardwareTxMs = 0;
static unsigned long motionEventCounter = 0;
static bool motionVerified = false;

// Quản lý nút bấm (Hỗ trợ cả Nhấn Nhả và Nhấn Giữ)
static int lastButtonReading = HIGH;
static int buttonState = HIGH;
static unsigned long lastDebounceTime = 0;
static unsigned long buttonPressStartTime = 0;
static bool longPressHandled = false;

// Biến chớp còi báo động
static unsigned long lastAlarmToggleTime = 0;
static bool alarmToggleState = false;

// -------------------------------------------------------------
// ĐIỀU KHIỂN CÒI BUZZER (Tương thích cả Passive và Active Buzzer)
// -------------------------------------------------------------
void buzzerOn(uint16_t freq = 2500) {
  tone(PIN_BUZZER, freq);
}

void buzzerOff() {
  noTone(PIN_BUZZER);
  digitalWrite(PIN_BUZZER, LOW);
}

void chirpBuzzer(uint8_t count, uint16_t onMs, uint16_t offMs, uint16_t freq = 2500) {
  for (uint8_t i = 0; i < count; i++) {
    buzzerOn(freq);
    delay(onMs);
    buzzerOff();
    if (i + 1 < count) delay(offMs);
  }
}

// Kiểm tra toàn diện còi với nhiều tần số khác nhau
void runBuzzerDiagnostics() {
  Serial.println(F("\n🔊 [BUZZER DIAGNOSTIC] Bắt đầu phát chuỗi âm thanh kiểm tra còi..."));
  Serial.println(F("   -> Tone 1: 1500 Hz (Âm trầm)"));
  chirpBuzzer(1, 100, 50, 1500);
  delay(100);

  Serial.println(F("   -> Tone 2: 2400 Hz (Cộng hưởng chuẩn)"));
  chirpBuzzer(2, 80, 60, 2400);
  delay(100);

  Serial.println(F("   -> Tone 3: 3200 Hz (Âm bổng cảnh báo)"));
  chirpBuzzer(3, 50, 40, 3200);

  Serial.println(F("✅ [BUZZER DIAGNOSTIC] Hoàn tất kiểm tra còi."));
  Serial.println(F("   Nếu bạn vẫn KHÔNG nghe thấy âm thanh nào:"));
  Serial.println(F("   1. Kiểm tra cực tính: Chân (+) nối GPIO 22, Chân (-) nối GND."));
  Serial.println(F("   2. Kiểm tra tiếp xúc breadboard hoặc dây jumper có bị đứt ngầm không."));
  Serial.println(F("   3. Nếu dùng module còi 3 chân có transistor, cần cấp VCC=5V (VIN).\n"));
}

// -------------------------------------------------------------
// QUẢN LÝ ĐỘ NHẠY & PHẠM VI (SENSITIVITY / RANGE PROFILE)
// -------------------------------------------------------------
unsigned long getMinMotionDurationMs(SensitivityProfile prof) {
  switch (prof) {
    case SENS_NEAR_3M: return 400; // 400ms: Cự ly gần ~3m, lọc nhiễu tối đa
    case SENS_MED_5M:  return 150; // 150ms: Tiêu chuẩn ~5m
    case SENS_FAR_7M:  return 40;  // 40ms:  Cự ly xa ~7m, nhạy tối đa
    default:           return 150;
  }
}

const char* getSensitivityName(SensitivityProfile prof) {
  switch (prof) {
    case SENS_NEAR_3M: return "MỨC 1: CỰ LY GẦN (~3m - Chống báo giả)";
    case SENS_MED_5M:  return "MỨC 2: TIÊU CHUẨN (~5m - Cân bằng)";
    case SENS_FAR_7M:  return "MỨC 3: CỰ LY XA (~7m - Nhạy tối đa)";
    default:           return "UNKNOWN";
  }
}

void setSensitivityProfile(SensitivityProfile newSens) {
  currentSens = newSens;
  Serial.println(F("\n============================================================"));
  Serial.printf("🎯 [CẬP NHẬT PHẠM VI] ĐÃ THAY ĐỔI ĐỘ NHẠY SANG:\n");
  Serial.printf("   >>> %s <<<\n", getSensitivityName(currentSens));
  Serial.printf("   - Thời gian kích hoạt tối thiểu: %lu ms (Software Filter)\n", getMinMotionDurationMs(currentSens));
  
  if (currentSens == SENS_NEAR_3M) {
    Serial.println(F("   - Hướng dẫn vặn núm Sx vật lý: Xoay NGƯỢC chiều kim đồng hồ (CCW) để giảm cự ly."));
    chirpBuzzer(1, 100, 0, 2000); // 1 bíp xác nhận Mức 1
  } else if (currentSens == SENS_MED_5M) {
    Serial.println(F("   - Hướng dẫn vặn núm Sx vật lý: Để ở vị trí GIỮA (12 giờ)."));
    chirpBuzzer(2, 70, 70, 2400); // 2 bíp xác nhận Mức 2
  } else {
    Serial.println(F("   - Hướng dẫn vặn núm Sx vật lý: Xoay CÙNG chiều kim đồng hồ (CW) để tăng tối đa cự ly."));
    chirpBuzzer(3, 60, 50, 2800); // 3 bíp xác nhận Mức 3
  }
  Serial.println(F("============================================================\n"));
}

void printHelpMenu() {
  Serial.println(F("------------------------------------------------------------"));
  Serial.println(F("⌨️  DANH MỤC PHÍM ĐIỀU KHIỂN QUA SERIAL MONITOR:"));
  Serial.println(F("   [1] : Chọn Phạm vi GẦN  (~3m) - Lọc nhiễu cao, còi bíp 1 tiếng"));
  Serial.println(F("   [2] : Chọn Phạm vi VỪA  (~5m) - Tiêu chuẩn, còi bíp 2 tiếng"));
  Serial.println(F("   [3] : Chọn Phạm vi XA   (~7m) - Nhạy tối đa, còi bíp 3 tiếng"));
  Serial.println(F("   [t] : Kiểm tra còi Buzzer ngay lập tức (Phát 3 dải tần số)"));
  Serial.println(F("   [m] : Chuyển đổi chế độ (AUTO-LIGHT <---> ARMED SECURITY)"));
  Serial.println(F("   [?] : In lại hướng dẫn này"));
  Serial.println(F("   * Nút bấm GPIO 4: Nhấn nhanh = Đổi Mode | Nhấn giữ > 1.2s = Đổi Độ nhạy"));
  Serial.println(F("------------------------------------------------------------"));
}

void printPirKnobGuide() {
  Serial.println(F("------------------------------------------------------------"));
  Serial.println(F("📋 THÔNG SỐ VÀ CÁCH CĂN CHỈNH 2 NÚM VẶN TRÊN CẢM BIẾN PIR HC-SR501"));
  Serial.println(F("------------------------------------------------------------"));
  Serial.println(F("1. NÚM CHỈNH ĐỘ NHẠY (SENSITIVITY - Sx):"));
  Serial.println(F("   - Về phần cứng: Là biến trở analog nối vào IC BISS0001 (không đưa dây ra ngoài)."));
  Serial.println(F("   - Dải khoảng cách: 3 mét ~ 7 mét (Góc quét hồng ngoại: ~120°)."));
  Serial.println(F("   - Xoay cùng chiều kim đồng hồ (CW):  TĂNG cự ly nhận diện (tối đa ~7m)."));
  Serial.println(F("   - Xoay ngược chiều kim đồng hồ (CCW): GIẢM cự ly nhận diện (tối thiểu ~3m)."));
  Serial.println(F("   - Về phần mềm: Bạn có thể gõ phím 1, 2, 3 hoặc nhấn giữ nút để chuyển dải cự ly!"));
  Serial.println(F(""));
  Serial.println(F("2. NÚM CHỈNH THỜI GIAN TRÌ HOÃN (TIME DELAY - Tx):"));
  Serial.println(F("   - Dải thời gian giữ mức HIGH: ~0.5 giây đến ~300 giây (5 phút)."));
  Serial.println(F("   - Công thức BISS0001: Tx ≈ 24576 × R10 × C6."));
  Serial.println(F("   - Xoay cùng chiều kim đồng hồ (CW):  TĂNG thời gian trễ (tối đa ~5 phút)."));
  Serial.println(F("   - Xoay ngược chiều kim đồng hồ (CCW): GIẢM thời gian trễ (ngắn nhất ~0.5s - 3s)."));
  Serial.println(F("   - Hệ thống sẽ tự động đo thời gian thực tế mỗi khi có xung kết thúc!"));
  Serial.println(F("------------------------------------------------------------"));
}

void setup() {
  Serial.begin(115200);
  delay(500);

  bootTime = millis();

  pinMode(PIN_PIR_IN, INPUT);
  pinMode(PIN_BTN_MODE, INPUT_PULLUP);
  pinMode(PIN_LED_LIGHT, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  digitalWrite(PIN_LED_LIGHT, LOW);
  buzzerOff();

  Serial.println();
  Serial.println(F("============================================================"));
  Serial.println(F("🚀 [SYSTEM] POC PIR Motion Alarm Initialized"));
  Serial.println(F("[CONFIG] PIR Input: GPIO 33 | PIR Power: VIN (5V)"));
  Serial.println(F("[CONFIG] Mode Button: GPIO 4 | LED: GPIO 21 | Buzzer: GPIO 22"));
  Serial.println(F("[CONFIG] Default Mode: AUTO-LIGHT | Default Range: MED (~5m)"));
  Serial.println(F("============================================================"));

  // Kiểm tra còi tự động lúc khởi động (Power-on Self Test)
  Serial.println(F("🔔 [BUZZER TEST] Đang phát tiếng bíp kiểm tra còi (POST)..."));
  chirpBuzzer(2, 70, 50, 2400); // 2 tiếng bíp khởi động
  Serial.println(F("   -> Nếu không nghe thấy tiếng còi, hãy kiểm tra dây còi GPIO 22 & GND!"));

  // In hướng dẫn thông số 2 núm chỉnh & menu phím tắt
  printPirKnobGuide();
  printHelpMenu();
}

void handleSerialCommands() {
  while (Serial.available() > 0) {
    char ch = Serial.read();
    if (ch == '\r' || ch == '\n' || ch == ' ') continue;

    switch (ch) {
      case '1':
        setSensitivityProfile(SENS_NEAR_3M);
        break;
      case '2':
        setSensitivityProfile(SENS_MED_5M);
        break;
      case '3':
        setSensitivityProfile(SENS_FAR_7M);
        break;
      case 't':
      case 'b':
      case 'T':
      case 'B':
        runBuzzerDiagnostics();
        break;
      case 'm':
      case 'M':
        if (currentMode == MODE_AUTO_LIGHT) {
          currentMode = MODE_ARMED_SECURITY;
          Serial.println(F("\n[MODE] >>> Chuyển sang [ARMED SECURITY 🚨] (Báo động an ninh)"));
          chirpBuzzer(2, 60, 60, 2600);
        } else {
          currentMode = MODE_AUTO_LIGHT;
          Serial.println(F("\n[MODE] >>> Chuyển sang [AUTO-LIGHT 💡] (Đèn tự động chiếu sáng)"));
          chirpBuzzer(1, 150, 0, 1800);
        }
        break;
      case '?':
      case 'h':
      case 'H':
        printHelpMenu();
        break;
      default:
        Serial.printf("[SERIAL] Lệnh '%c' không hợp lệ. Gõ '?' để xem menu.\n", ch);
        break;
    }
  }
}

void loop() {
  unsigned long now = millis();

  // Đọc lệnh điều khiển từ Serial
  handleSerialCommands();

  // 1. Xử lý giai đoạn làm nóng quang học (Warm-up Period)
  if (now - bootTime < WARMUP_DURATION_MS) {
    unsigned long remainingSec = (WARMUP_DURATION_MS - (now - bootTime)) / 1000 + 1;
    if (now - lastLogTime >= 1000) {
      lastLogTime = now;
      Serial.printf("[%6lu ms] [WARMUP] Cảm biến PIR đang ổn định quang học... (%lu s còn lại)\n", now, remainingSec);
    }
    return;
  }

  // 2. Xử lý nút bấm (Hỗ trợ Nhấn Nhanh: Đổi Mode | Nhấn Giữ: Đổi Độ Nhạy)
  int reading = digitalRead(PIN_BTN_MODE);
  if (reading != lastButtonReading) {
    lastDebounceTime = now;
  }

  if ((now - lastDebounceTime) > DEBOUNCE_MS) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        // Nút vừa được nhấn xuống
        buttonPressStartTime = now;
        longPressHandled = false;
      } else {
        // Nút vừa được nhả ra
        if (!longPressHandled) {
          // Nhấn nhanh (< 1.2s): Chuyển đổi Mode
          if (currentMode == MODE_AUTO_LIGHT) {
            currentMode = MODE_ARMED_SECURITY;
            Serial.printf("\n[EVENT %6lu ms] >>> NÚT BẤM (Nhấn nhanh): Chuyển sang [ARMED SECURITY 🚨]\n", now);
            chirpBuzzer(2, 60, 60, 2600); // 2 tiếng bíp báo Arm
          } else {
            currentMode = MODE_AUTO_LIGHT;
            Serial.printf("\n[EVENT %6lu ms] >>> NÚT BẤM (Nhấn nhanh): Chuyển sang [AUTO-LIGHT 💡]\n", now);
            chirpBuzzer(1, 150, 0, 1800); // 1 tiếng bíp dài báo Disarm
          }
        }
      }
    } else if (buttonState == LOW && !longPressHandled) {
      // Đang giữ nút: kiểm tra thời gian nhấn giữ
      if (now - buttonPressStartTime >= LONG_PRESS_MS) {
        longPressHandled = true;
        // Xoay vòng độ nhạy: 1 -> 2 -> 3 -> 1
        SensitivityProfile nextSens = (currentSens == SENS_NEAR_3M) ? SENS_MED_5M :
                                      (currentSens == SENS_MED_5M)  ? SENS_FAR_7M : SENS_NEAR_3M;
        Serial.printf("\n[EVENT %6lu ms] >>> NÚT BẤM (Nhấn giữ > 1.2s): Đổi mức độ nhạy!\n", now);
        setSensitivityProfile(nextSens);
      }
    }
  }
  lastButtonReading = reading;

  // 3. Đọc và phân tích xung từ cảm biến PIR (Đo lường thời gian trễ thực tế của núm Tx)
  bool pirCurrentState = (digitalRead(PIN_PIR_IN) == HIGH);

  // Phát hiện sườn lên (Rising Edge: Bắt đầu có chuyển động)
  if (pirCurrentState && !lastPirRawState) {
    motionEventCounter++;
    pirPulseStartTime = now;
    motionVerified = false;
    Serial.printf("\n[PIR EVENT #%lu | %6lu ms] >>> BẮT ĐẦU CÓ CHUYỂN ĐỘNG (PIR OUT: HIGH)\n", motionEventCounter, now);
    Serial.println(F("   [+] Đang lọc nhiễu theo phạm vi độ nhạy và tính thời gian giữ mức HIGH của núm Tx..."));
  }

  // Kiểm tra bộ lọc thời lượng chuyển động (Software Sensitivity Filter)
  if (pirCurrentState) {
    unsigned long activeDuration = now - pirPulseStartTime;
    unsigned long requiredDuration = getMinMotionDurationMs(currentSens);

    if (!motionVerified && activeDuration >= requiredDuration) {
      motionVerified = true;
      lastMotionTime = now;
      Serial.printf("🎯 [XÁC NHẬN CHUYỂN ĐỘNG] Đạt ngưỡng cự ly %s (Duy trì >= %lu ms)!\n",
                    getSensitivityName(currentSens), requiredDuration);
    }
  }

  // Phát hiện sườn xuống (Falling Edge: Hết chuyển động / Kết thúc xung trễ Tx)
  if (!pirCurrentState && lastPirRawState) {
    lastMeasuredHardwareTxMs = now - pirPulseStartTime;
    float txSeconds = lastMeasuredHardwareTxMs / 1000.0f;
    Serial.printf("[PIR EVENT #%lu | %6lu ms] <<< KẾT THÚC CHUYỂN ĐỘNG (PIR OUT: LOW)\n", motionEventCounter, now);
    Serial.println(F("------------------------------------------------------------"));
    Serial.printf("⏱️  KẾT QUẢ ĐO THỜI GIAN TRÌ HOÃN NÚM PHẦN CỨNG (Tx Delay Knob):\n");
    Serial.printf("   - Thời gian xung HIGH đo được: %.2f giây (%lu ms)\n", txSeconds, lastMeasuredHardwareTxMs);
    Serial.printf("   - Trạng thái núm Tx: %s\n", 
                  (txSeconds < 5.0f) ? "Đang vặn gần MIN (~0.5s - 5s, chuẩn nhất cho ESP32)" :
                  (txSeconds < 30.0f) ? "Đang vặn ở mức TRUNG BÌNH (5s - 30s)" :
                                        "Đang vặn ở mức CAO (> 30s, giữ sáng phần cứng rất lâu)");
    Serial.printf("   - Cấu hình độ nhạy hiện tại: %s\n", getSensitivityName(currentSens));
    Serial.println(F("------------------------------------------------------------\n"));
    motionVerified = false;
  }

  lastPirRawState = pirCurrentState;

  // 4. Máy trạng thái điều khiển theo chế độ (FSM)
  if (currentMode == MODE_AUTO_LIGHT) {
    // Chế độ đèn tự động: còi luôn ngắt để đảm bảo êm ái
    buzzerOff();

    if (motionVerified) {
      lastMotionTime = now;
      if (!isLightOn) {
        isLightOn = true;
        digitalWrite(PIN_LED_LIGHT, HIGH);
        Serial.printf("[ACTION %6lu ms] >>> ĐÈN BẬT SÁNG! (Bắt đầu bộ đếm phần mềm Keep-Alive %lu ms).\n", now, HOLD_TIME_MS);
      }
    } else {
      // Khi không còn chuyển động, kiểm tra khoảng thời gian Hold Time phần mềm
      if (isLightOn) {
        if (now - lastMotionTime >= HOLD_TIME_MS) {
          isLightOn = false;
          digitalWrite(PIN_LED_LIGHT, LOW);
          Serial.printf("[ACTION %6lu ms] >>> HẾT THỜI GIAN CHỜ (%lu ms). ĐÈN ĐÃ TẮT TIẾT KIỆM ĐIỆN.\n", now, HOLD_TIME_MS);
        }
      }
    }
  } else { // MODE_ARMED_SECURITY
    if (motionVerified) {
      // Nhấp nháy còi và đèn cảnh báo liên tục (120ms nhịp, phát âm tần 2500Hz)
      if (now - lastAlarmToggleTime >= 120) {
        lastAlarmToggleTime = now;
        alarmToggleState = !alarmToggleState;
        digitalWrite(PIN_LED_LIGHT, alarmToggleState ? HIGH : LOW);
        if (alarmToggleState) {
          buzzerOn(2500); // Kêu còi ở tần số cộng hưởng 2500Hz
        } else {
          buzzerOff();
        }
      }
    } else {
      digitalWrite(PIN_LED_LIGHT, LOW);
      buzzerOff();
      alarmToggleState = false;
    }
  }

  // 5. Định kỳ in log trạng thái ra Serial Monitor
  if (now - lastLogTime >= 1500) {
    lastLogTime = now;
    unsigned long timeSinceLastMotion = (lastMotionTime > 0) ? (now - lastMotionTime) : 99999;
    char txStr[16];
    if (lastMeasuredHardwareTxMs > 0) {
      snprintf(txStr, sizeof(txStr), "%.1fs", lastMeasuredHardwareTxMs / 1000.0f);
    } else {
      snprintf(txStr, sizeof(txStr), "N/A");
    }

    const char* sensShort = (currentSens == SENS_NEAR_3M) ? "NEAR(3m)" :
                            (currentSens == SENS_MED_5M)  ? "MED(5m)"  : "FAR(7m)";

    Serial.printf("[%6lu ms] Mode:%-11s | Range:%-8s | PIR:%-6s | Light:%-3s | Tx:%-5s | Idle:%5lu ms\n",
                  now,
                  (currentMode == MODE_AUTO_LIGHT ? "AUTO-LIGHT" : "ARMED"),
                  sensShort,
                  (pirCurrentState ? "ACTIVE" : "QUIET"),
                  (digitalRead(PIN_LED_LIGHT) ? "ON" : "OFF"),
                  txStr,
                  timeSinceLastMotion);
  }

  delay(10);
}
