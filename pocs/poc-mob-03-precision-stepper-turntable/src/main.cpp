/**
 * @file main.cpp
 * @brief POC MOB-03: Bàn Xoay Quét 3D & Chụp Ảnh Sản Phẩm Chính Xác (Precision Stepper Turntable)
 * @details Điều khiển bàn xoay chính xác bằng động cơ bước 28BYJ-48 + ULN2003, bàn phím ma trận 4x4,
 *          màn hình OLED SSD1306, đồng hồ RTC DS1307/DS3231 và Relay kích màn trập máy ảnh.
 * 
 * Nền tảng: ESP32 DevKit V1 (30 chân)
 * Tuân thủ quy chuẩn AGENTS.md:
 * - Boot-Safe Pinout (Không dùng GPIO 12, không dùng GPIO 6-11, không dùng ADC2 khi WiFi)
 * - Tự ngắt dòng cuộn dây motor (Coil De-energization) khi dừng để chống quá nhiệt
 * - Chu trình kích Shutter chống rung nhòe hình ảnh (Settling -> Trigger -> Cooldown -> RTC Log)
 * - Giao diện OLED 3 vùng và phím khẩn cấp Emergency Stop (*)
 */

#include <Arduino.h>
#include <Wire.h>
#include <AccelStepper.h>
#include <Keypad.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

// ============================================================================
// 1. ĐỊNH NGHĨA CHÂN PHẦN CỨNG (BOOT-SAFE PINOUT MAPPING)
// ============================================================================

// I2C Bus (Dùng chung cho OLED SSD1306 và RTC DS1307/DS3231)
constexpr uint8_t PIN_I2C_SDA = 21;
constexpr uint8_t PIN_I2C_SCL = 22;

// ULN2003 Stepper Driver (Động cơ bước 28BYJ-48 5V)
constexpr uint8_t PIN_STEPPER_IN1 = 19;
constexpr uint8_t PIN_STEPPER_IN2 = 18;
constexpr uint8_t PIN_STEPPER_IN3 = 5;
constexpr uint8_t PIN_STEPPER_IN4 = 17;

// Shutter Relay Module (Kích màn trập máy ảnh - Active LOW)
constexpr uint8_t PIN_RELAY_SHUTTER = 23;

// Còi báo trạng thái / LED tín hiệu
constexpr uint8_t PIN_BUZZER = 2;

// Bàn phím ma trận 4x4 (4 Rows x 4 Cols)
// Toàn bộ các chân Cột đều dùng nhóm GPIO có điện trở kéo lên nội bộ (INPUT_PULLUP)
// Tránh hoàn toàn GPIO 12 (Strapping) và GPIO 34-39 (Input-Only)
constexpr uint8_t KEYPAD_ROWS = 4;
constexpr uint8_t KEYPAD_COLS = 4;
const uint8_t PIN_KEYPAD_ROWS[KEYPAD_ROWS] = {13, 14, 27, 4};
const uint8_t PIN_KEYPAD_COLS[KEYPAD_COLS] = {26, 25, 33, 32};

// ============================================================================
// 2. THÔNG SỐ VẬN HÀNH & HẰNG SỐ CƠ KHÍ
// ============================================================================

// 28BYJ-48 với tỉ số truyền 1:64
// Chế độ Half-step: 4096 bước trên 1 vòng trục ra (360 độ)
constexpr float STEPS_PER_REV_HALFSTEP = 4096.0f;
constexpr float STEPS_PER_DEGREE = STEPS_PER_REV_HALFSTEP / 360.0f; // ~11.3778 steps/deg

// Tốc độ và gia tốc động cơ bước
constexpr float STEPPER_MAX_SPEED = 800.0f;     // steps/sec
constexpr float STEPPER_ACCELERATION = 400.0f;  // steps/sec^2
constexpr float CONTINUOUS_DEFAULT_SPEED = 400.0f;

// Thời gian điều khiển kích màn trập máy ảnh (Shutter Timing)
constexpr uint32_t SHUTTER_SETTLING_MS = 400;   // Chờ ổn định rung cơ khí sau khi dừng xoay
constexpr uint32_t SHUTTER_PULSE_MS = 250;      // Độ rộng xung kích relay đóng tiếp điểm màn trập
constexpr uint32_t SHUTTER_COOLDOWN_MS = 300;   // Chờ máy ảnh phơi sáng và lưu ảnh vào thẻ

// Kích thước màn hình OLED SSD1306
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr uint8_t OLED_I2C_ADDR = 0x3C;

// ============================================================================
// 3. KHỞI TẠO ĐỐI TƯỢNG NGOẠI VI
// ============================================================================

// Thứ tự chân kích nửa bước của 28BYJ-48 qua AccelStepper: IN1, IN3, IN2, IN4
AccelStepper stepper(AccelStepper::HALF4WIRE, PIN_STEPPER_IN1, PIN_STEPPER_IN3, PIN_STEPPER_IN2, PIN_STEPPER_IN4);

// Cấu hình ma trận bàn phím 4x4
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
Keypad keypad = Keypad(makeKeymap(keys), (byte*)PIN_KEYPAD_ROWS, (byte*)PIN_KEYPAD_COLS, KEYPAD_ROWS, KEYPAD_COLS);

// Màn hình OLED SSD1306
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Đồng hồ RTC
RTC_DS1307 rtc;
bool g_rtcAvailable = false;
uint32_t g_bootTimeOffset = 0;

// ============================================================================
// 4. MÁY TRẠNG THÁI (FINITE STATE MACHINE - FSM)
// ============================================================================

enum SystemState {
  STATE_IDLE,                 // Màn hình chính, chờ chọn chế độ
  STATE_AUTO_SELECT_PRESET,   // Chọn preset số khung hình chụp 360
  STATE_AUTO_INPUT_CUSTOM,    // Nhập số khung hình tùy ý
  STATE_AUTO_RUNNING,         // Đang thực hiện chu trình chụp 360 tự động
  STATE_MANUAL_INPUT_ANGLE,   // Đang nhập góc quay mục tiêu
  STATE_MANUAL_RUNNING,       // Đang quay đến góc chỉ định
  STATE_CONTINUOUS_RUNNING,   // Bàn xoay trưng bày xoay liên tục
  STATE_DIAGNOSTICS,          // Chế độ chẩn đoán & thông tin hệ thống
  STATE_EMERGENCY_STOP        // Dừng khẩn cấp
};

enum AutoCapturePhase {
  AUTO_PHASE_MOVE,            // Đang xoay tới góc của frame kế tiếp
  AUTO_PHASE_SETTLE,          // Dừng, ngắt dòng cuộn dây, chờ tắt rung
  AUTO_PHASE_TRIGGER,         // Kích Relay Shutter ON
  AUTO_PHASE_COOLDOWN,        // Tắt Relay Shutter, chờ hoàn tất phơi sáng
  AUTO_PHASE_DONE             // Hoàn thành toàn bộ 360 độ
};

// Biến trạng thái toàn cục
SystemState g_systemState = STATE_IDLE;
AutoCapturePhase g_autoPhase = AUTO_PHASE_MOVE;

int g_totalFrames = 12;         // Số khung hình mặc định (30 độ / frame)
int g_currentFrame = 0;         // Khung hình hiện tại
float g_currentAngle = 0.0f;    // Góc hiện tại (độ)
float g_targetAngle = 0.0f;     // Góc mục tiêu (độ)
float g_angleIncrement = 30.0f; // Bước góc cho mỗi frame
uint32_t g_phaseTimer = 0;      // Timer đếm chuyển phase non-blocking
uint32_t g_autoStartTime = 0;   // Mốc thời gian bắt đầu quét 360
uint32_t g_autoTotalDuration = 0;

// Biến nhập liệu bàn phím
String g_inputBuffer = "";
bool g_continuousCW = true;
float g_continuousSpeed = CONTINUOUS_DEFAULT_SPEED;

// ============================================================================
// 5. CÁC HÀM TIỆN ÍCH THỜI GIAN & RTC
// ============================================================================

/**
 * @brief Lấy chuỗi định dạng thời gian thực "YYYY-MM-DD HH:MM:SS"
 */
String getFormattedDateTime() {
  if (g_rtcAvailable) {
    DateTime now = rtc.now();
    char buf[24];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
    return String(buf);
  } else {
    // Fallback: Sử dụng millis() tính thời gian tương đối
    uint32_t sec = millis() / 1000;
    uint32_t m = sec / 60;
    uint32_t s = sec % 60;
    uint32_t h = m / 60;
    m = m % 60;
    char buf[24];
    snprintf(buf, sizeof(buf), "UPTIME %02d:%02d:%02d", h, m, s);
    return String(buf);
  }
}

/**
 * @brief Lấy chuỗi giờ ngắn "HH:MM:SS" cho thanh Header OLED
 */
String getShortTime() {
  if (g_rtcAvailable) {
    DateTime now = rtc.now();
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    return String(buf);
  } else {
    uint32_t sec = millis() / 1000;
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d", (sec / 60) % 60, sec % 60);
    return String(buf);
  }
}

// ============================================================================
// 6. ĐIỀU KHIỂN SHUTTER & ĐỘNG CƠ BƯỚC
// ============================================================================

/**
 * @brief Ngắt điện 4 cuộn dây động cơ bước (De-energize coils)
 * Giúp triệt tiêu dòng tiêu thụ tĩnh ~300mA và tránh phát nhiệt khi motor đang đứng yên.
 */
void deenergizeStepper() {
  stepper.disableOutputs();
  digitalWrite(PIN_STEPPER_IN1, LOW);
  digitalWrite(PIN_STEPPER_IN2, LOW);
  digitalWrite(PIN_STEPPER_IN3, LOW);
  digitalWrite(PIN_STEPPER_IN4, LOW);
}

/**
 * @brief Kích hoạt ngõ ra Relay điều khiển màn trập máy ảnh
 * @param active true = Kích chụp (Active LOW), false = Nhả chụp
 */
void setShutter(bool active) {
  if (active) {
    digitalWrite(PIN_RELAY_SHUTTER, LOW); // Relay ON
    digitalWrite(PIN_BUZZER, HIGH);       // Buzzer bíp đồng bộ
  } else {
    digitalWrite(PIN_RELAY_SHUTTER, HIGH); // Relay OFF
    digitalWrite(PIN_BUZZER, LOW);
  }
}

/**
 * @brief Ra lệnh quay motor tới góc cụ thể (độ)
 */
void rotateToAngle(float angleDeg) {
  long targetSteps = lround(angleDeg * STEPS_PER_DEGREE);
  stepper.enableOutputs();
  stepper.moveTo(targetSteps);
}

/**
 * @brief Dừng khẩn cấp motor và đưa toàn bộ cơ cấu về trạng thái an toàn
 */
void emergencyStop() {
  stepper.stop();
  deenergizeStepper();
  setShutter(false);
  g_systemState = STATE_EMERGENCY_STOP;
  Serial.println(F("[SAFETY] !!! EMERGENCY STOP TRIGGERED !!!"));
}

// ============================================================================
// 7. GIAO DIỆN MÀN HÌNH OLED SSD1306 (UI RENDERER)
// ============================================================================

void drawHeader(const char* title) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(title);
  
  String timeStr = getShortTime();
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(SCREEN_WIDTH - w, 0);
  display.print(timeStr);
  display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
}

void renderUI() {
  switch (g_systemState) {
    case STATE_IDLE: {
      drawHeader("TURNTABLE 3D");
      display.setCursor(0, 14);
      display.print("A: Auto 360 Photo");
      display.setCursor(0, 24);
      display.print("B: Manual Angle");
      display.setCursor(0, 34);
      display.print("C: Continuous Spin");
      display.setCursor(0, 44);
      display.print("D: Diagnostics");
      display.setCursor(0, 54);
      display.print("#: Shoot | *: Home 0");
      break;
    }

    case STATE_AUTO_SELECT_PRESET: {
      drawHeader("SELECT PRESET");
      display.setCursor(0, 14);
      display.print("1: 8 shots  (45 deg)");
      display.setCursor(0, 24);
      display.print("2: 12 shots (30 deg)");
      display.setCursor(0, 34);
      display.print("3: 24 shots (15 deg)");
      display.setCursor(0, 44);
      display.print("4: 36 shots (10 deg)");
      display.setCursor(0, 54);
      display.print("0: Custom | *: Back");
      break;
    }

    case STATE_AUTO_INPUT_CUSTOM: {
      drawHeader("CUSTOM FRAMES");
      display.setCursor(0, 16);
      display.print("Enter frames (4-360):");
      display.setTextSize(2);
      display.setCursor(20, 30);
      display.print(g_inputBuffer + "_");
      display.setTextSize(1);
      display.setCursor(0, 54);
      display.print("*: Clear | #: Start");
      break;
    }

    case STATE_AUTO_RUNNING: {
      drawHeader("AUTO 360 SCAN");
      
      // Khung hình hiện tại
      display.setCursor(0, 14);
      display.printf("Frame: %02d / %02d", g_currentFrame, g_totalFrames);

      // Góc hiện tại
      display.setCursor(0, 25);
      display.printf("Angle: %.1f deg", g_currentAngle);

      // Trạng thái Shutter
      display.setCursor(0, 36);
      display.print("Status: ");
      if (g_autoPhase == AUTO_PHASE_MOVE) display.print("ROTATING...");
      else if (g_autoPhase == AUTO_PHASE_SETTLE) display.print("SETTLING...");
      else if (g_autoPhase == AUTO_PHASE_TRIGGER) display.print("[SHUTTER ON]");
      else if (g_autoPhase == AUTO_PHASE_COOLDOWN) display.print("SAVING...");
      else display.print("DONE");

      // Thanh tiến trình (Progress Bar)
      int progressWidth = (SCREEN_WIDTH - 4) * g_currentFrame / g_totalFrames;
      display.drawRect(0, 48, SCREEN_WIDTH, 7, SSD1306_WHITE);
      if (progressWidth > 0) {
        display.fillRect(2, 50, progressWidth, 3, SSD1306_WHITE);
      }

      display.setCursor(0, 56);
      display.print("Press * to STOP");
      break;
    }

    case STATE_MANUAL_INPUT_ANGLE: {
      drawHeader("MANUAL ANGLE");
      display.setCursor(0, 16);
      display.print("Enter angle (0-360):");
      display.setTextSize(2);
      display.setCursor(20, 30);
      display.print(g_inputBuffer + (char)247); // Ký tự độ
      display.setTextSize(1);
      display.setCursor(0, 54);
      display.print("*: Clear | #: Rotate");
      break;
    }

    case STATE_MANUAL_RUNNING: {
      drawHeader("MANUAL MOVE");
      display.setCursor(0, 16);
      display.printf("Target: %.1f deg", g_targetAngle);
      display.setCursor(0, 28);
      display.printf("Current: %.1f deg", (float)stepper.currentPosition() / STEPS_PER_DEGREE);
      display.setCursor(0, 42);
      if (stepper.distanceToGo() != 0) {
        display.print("Moving...");
      } else {
        display.print("AT TARGET (Holding)");
      }
      display.setCursor(0, 54);
      display.print("*: Cancel | #: Shoot");
      break;
    }

    case STATE_CONTINUOUS_RUNNING: {
      drawHeader("SHOWCASE SPIN");
      display.setCursor(0, 16);
      display.printf("Dir: %s", g_continuousCW ? "CLOCKWISE (CW)" : "COUNTER-CW");
      display.setCursor(0, 28);
      display.printf("Speed: %.0f sps", g_continuousSpeed);
      display.setCursor(0, 40);
      display.print("1: Dir | 2: + | 3: -");
      display.setCursor(0, 54);
      display.print("Press * to STOP");
      break;
    }

    case STATE_DIAGNOSTICS: {
      drawHeader("SYSTEM DIAG");
      display.setCursor(0, 14);
      display.printf("RTC: %s", g_rtcAvailable ? "DS1307 OK" : "NO RTC (Uptime)");
      display.setCursor(0, 24);
      display.print("Time: " + getShortTime());
      display.setCursor(0, 34);
      display.printf("Pos: %ld steps", stepper.currentPosition());
      display.setCursor(0, 44);
      display.print("1: Relay Click Test");
      display.setCursor(0, 54);
      display.print("*: Return to Menu");
      break;
    }

    case STATE_EMERGENCY_STOP: {
      drawHeader("! E-STOP ACTIVE !");
      display.setTextSize(1);
      display.setCursor(0, 16);
      display.print("MOTOR DISENGAGED");
      display.setCursor(0, 28);
      display.print("Coils de-energized.");
      display.setCursor(0, 40);
      display.print("Relay OFF.");
      display.setCursor(0, 54);
      display.print("Press # to Reset");
      break;
    }
  }

  display.display();
}

// ============================================================================
// 8. TIẾN TRÌNH QUÉT ẢNH 360 TỰ ĐỘNG (AUTO PHOTOGRAMMETRY ENGINE)
// ============================================================================

void startAutoCapture(int frames) {
  g_totalFrames = frames;
  g_currentFrame = 1;
  g_angleIncrement = 360.0f / (float)frames;
  g_currentAngle = 0.0f;
  g_autoStartTime = millis();

  // Đặt lại tọa độ stepper về 0
  stepper.setCurrentPosition(0);
  
  Serial.println(F("=================================================="));
  Serial.printf("[AUTO 360] STARTING PHOTOGRAMMETRY: %d FRAMES\n", g_totalFrames);
  Serial.printf("[AUTO 360] Step Angle: %.2f deg per frame\n", g_angleIncrement);
  Serial.println(F("=================================================="));

  // Frame 1 chụp tại vạch gốc 0 độ
  g_autoPhase = AUTO_PHASE_SETTLE;
  g_phaseTimer = millis();
  g_systemState = STATE_AUTO_RUNNING;
}

void processAutoCapture() {
  uint32_t now = millis();

  switch (g_autoPhase) {
    case AUTO_PHASE_MOVE: {
      // Động cơ bước đang quay đến góc tiếp theo
      if (stepper.distanceToGo() == 0) {
        // Đã tới góc đích
        deenergizeStepper(); // Ngắt điện cuộn dây lập tức để chống nhiệt
        g_autoPhase = AUTO_PHASE_SETTLE;
        g_phaseTimer = now;
      }
      break;
    }

    case AUTO_PHASE_SETTLE: {
      // Chờ bàn xoay tắt hẳn rung cơ học trước khi bấm máy
      if (now - g_phaseTimer >= SHUTTER_SETTLING_MS) {
        setShutter(true); // Đóng tiếp điểm Relay
        g_autoPhase = AUTO_PHASE_TRIGGER;
        g_phaseTimer = now;
      }
      break;
    }

    case AUTO_PHASE_TRIGGER: {
      // Kích Shutter trong khoảng thời gian PULSE_WIDTH
      if (now - g_phaseTimer >= SHUTTER_PULSE_MS) {
        setShutter(false); // Ngắt tiếp điểm Relay
        g_autoPhase = AUTO_PHASE_COOLDOWN;
        g_phaseTimer = now;

        // Ghi log Serial với mốc RTC thời gian thực
        String dt = getFormattedDateTime();
        uint32_t elapsed = (now - g_autoStartTime) / 1000;
        Serial.printf("[CAPTURE] [%s] Frame %02d/%02d @ %.1f deg | Shutter Triggered | Elapsed: %ds\n",
                      dt.c_str(), g_currentFrame, g_totalFrames, g_currentAngle, elapsed);
      }
      break;
    }

    case AUTO_PHASE_COOLDOWN: {
      // Chờ máy ảnh phơi sáng và lưu ảnh vào thẻ nhớ
      if (now - g_phaseTimer >= SHUTTER_COOLDOWN_MS) {
        if (g_currentFrame >= g_totalFrames) {
          // Đã hoàn tất toàn bộ 360 độ
          g_autoTotalDuration = (now - g_autoStartTime) / 1000;
          Serial.println(F("=================================================="));
          Serial.printf("[AUTO 360] COMPLETED! Total %d frames in %d seconds.\n", g_totalFrames, g_autoTotalDuration);
          Serial.println(F("=================================================="));

          // Phát bíp dài thông báo thành công
          digitalWrite(PIN_BUZZER, HIGH);
          delay(150);
          digitalWrite(PIN_BUZZER, LOW);
          delay(100);
          digitalWrite(PIN_BUZZER, HIGH);
          delay(250);
          digitalWrite(PIN_BUZZER, LOW);

          g_systemState = STATE_IDLE;
        } else {
          // Chuyển sang khung hình kế tiếp
          g_currentFrame++;
          g_currentAngle = (float)(g_currentFrame - 1) * g_angleIncrement;
          rotateToAngle(g_currentAngle);
          g_autoPhase = AUTO_PHASE_MOVE;
        }
      }
      break;
    }

    case AUTO_PHASE_DONE:
      g_systemState = STATE_IDLE;
      break;
  }
}

// ============================================================================
// 9. XỬ LÝ SỰ KIỆN BÀN PHÍM MA TRẬN 4x4
// ============================================================================

void handleKeypad(char key) {
  // Phím '*' luôn là CANCEL hoặc EMERGENCY STOP trong mọi chế độ
  if (key == '*') {
    if (g_systemState == STATE_AUTO_RUNNING || 
        g_systemState == STATE_MANUAL_RUNNING || 
        g_systemState == STATE_CONTINUOUS_RUNNING) {
      emergencyStop();
      return;
    } else if (g_systemState != STATE_IDLE) {
      // Quay về menu chính nếu đang ở các màn hình chọn
      g_systemState = STATE_IDLE;
      g_inputBuffer = "";
      return;
    } else {
      // Ở IDLE, nhấn '*' đưa motor về vạch 0 (Home)
      Serial.println(F("[ACTION] Homing to 0 degrees..."));
      rotateToAngle(0.0f);
      return;
    }
  }

  switch (g_systemState) {
    case STATE_IDLE: {
      if (key == 'A') {
        g_systemState = STATE_AUTO_SELECT_PRESET;
      } else if (key == 'B') {
        g_inputBuffer = "";
        g_systemState = STATE_MANUAL_INPUT_ANGLE;
      } else if (key == 'C') {
        g_systemState = STATE_CONTINUOUS_RUNNING;
        stepper.enableOutputs();
        stepper.setSpeed(g_continuousCW ? g_continuousSpeed : -g_continuousSpeed);
        Serial.println(F("[ACTION] Continuous Spin Started"));
      } else if (key == 'D') {
        g_systemState = STATE_DIAGNOSTICS;
      } else if (key == '#') {
        // Nhấn '#' ở chế độ IDLE: Kích chụp thủ công 1 phát
        Serial.println(F("[MANUAL SHOT] Shutter Trigger Pulse"));
        setShutter(true);
        delay(SHUTTER_PULSE_MS);
        setShutter(false);
      }
      break;
    }

    case STATE_AUTO_SELECT_PRESET: {
      if (key == '1') startAutoCapture(8);
      else if (key == '2') startAutoCapture(12);
      else if (key == '3') startAutoCapture(24);
      else if (key == '4') startAutoCapture(36);
      else if (key == '0') {
        g_inputBuffer = "";
        g_systemState = STATE_AUTO_INPUT_CUSTOM;
      }
      break;
    }

    case STATE_AUTO_INPUT_CUSTOM: {
      if (key >= '0' && key <= '9' && g_inputBuffer.length() < 3) {
        g_inputBuffer += key;
      } else if (key == '#') {
        int val = g_inputBuffer.toInt();
        if (val >= 4 && val <= 360) {
          startAutoCapture(val);
        } else {
          Serial.println(F("[ERROR] Invalid frame count! Must be between 4 and 360."));
          g_inputBuffer = "";
        }
      }
      break;
    }

    case STATE_MANUAL_INPUT_ANGLE: {
      if (key >= '0' && key <= '9' && g_inputBuffer.length() < 3) {
        g_inputBuffer += key;
      } else if (key == '#') {
        float angle = g_inputBuffer.toFloat();
        if (angle >= 0.0f && angle <= 360.0f) {
          g_targetAngle = angle;
          rotateToAngle(g_targetAngle);
          g_systemState = STATE_MANUAL_RUNNING;
          Serial.printf("[MANUAL] Rotating to target angle: %.1f deg\n", g_targetAngle);
        } else {
          Serial.println(F("[ERROR] Angle must be between 0 and 360 degrees."));
          g_inputBuffer = "";
        }
      }
      break;
    }

    case STATE_MANUAL_RUNNING: {
      if (key == '#') {
        // Bấm chụp tại góc hiện tại
        Serial.printf("[MANUAL SHOT] Captured at %.1f deg\n", (float)stepper.currentPosition() / STEPS_PER_DEGREE);
        setShutter(true);
        delay(SHUTTER_PULSE_MS);
        setShutter(false);
      }
      break;
    }

    case STATE_CONTINUOUS_RUNNING: {
      if (key == '1') {
        g_continuousCW = !g_continuousCW;
        stepper.setSpeed(g_continuousCW ? g_continuousSpeed : -g_continuousSpeed);
        Serial.printf("[CONT] Direction: %s\n", g_continuousCW ? "CW" : "CCW");
      } else if (key == '2') {
        g_continuousSpeed = min(g_continuousSpeed + 100.0f, STEPPER_MAX_SPEED);
        stepper.setSpeed(g_continuousCW ? g_continuousSpeed : -g_continuousSpeed);
        Serial.printf("[CONT] Speed+: %.0f sps\n", g_continuousSpeed);
      } else if (key == '3') {
        g_continuousSpeed = max(g_continuousSpeed - 100.0f, 100.0f);
        stepper.setSpeed(g_continuousCW ? g_continuousSpeed : -g_continuousSpeed);
        Serial.printf("[CONT] Speed-: %.0f sps\n", g_continuousSpeed);
      }
      break;
    }

    case STATE_DIAGNOSTICS: {
      if (key == '1') {
        Serial.println(F("[DIAG] Testing Shutter Relay (Click test)..."));
        setShutter(true);
        delay(200);
        setShutter(false);
      }
      break;
    }

    case STATE_EMERGENCY_STOP: {
      if (key == '#') {
        // Reset sau dừng khẩn cấp
        g_systemState = STATE_IDLE;
        stepper.setCurrentPosition(0);
        g_currentAngle = 0.0f;
        Serial.println(F("[SAFETY] E-Stop cleared. Returned to IDLE."));
      }
      break;
    }

    default:
      break;
  }
}

// ============================================================================
// 10. SETUP & MAIN LOOP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("\n========================================================"));
  Serial.println(F("   POC MOB-03: PRECISION STEPPER TURNTABLE CONTROLLER   "));
  Serial.println(F("   ESP32 DevKit V1 (30-pin) | PlatformIO & Wokwi CLI    "));
  Serial.println(F("========================================================"));

  // Cấu hình chân Relay và Buzzer
  pinMode(PIN_RELAY_SHUTTER, OUTPUT);
  digitalWrite(PIN_RELAY_SHUTTER, HIGH); // Mặc định mở tiếp điểm (Active LOW)

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  // Khởi tạo I2C Bus trên GPIO 21 (SDA) và GPIO 22 (SCL)
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Khởi tạo màn hình OLED SSD1306
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println(F("[WARN] OLED SSD1306 initialization failed! Check wiring."));
  } else {
    Serial.println(F("[INIT] OLED SSD1306 (0x3C) initialized successfully."));
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.println(F("MOB-03 TURNTABLE"));
    display.setCursor(10, 35);
    display.println(F("Booting System..."));
    display.display();
  }

  // Khởi tạo RTC DS1307
  if (!rtc.begin()) {
    Serial.println(F("[WARN] RTC DS1307 not detected on I2C (0x68). Fallback to software timer."));
    g_rtcAvailable = false;
  } else {
    Serial.println(F("[INIT] RTC DS1307 initialized successfully."));
    g_rtcAvailable = true;
    if (!rtc.isrunning()) {
      Serial.println(F("[WARN] RTC is NOT running! Setting RTC time to build time."));
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  // Cấu hình Stepper motor
  stepper.setMaxSpeed(STEPPER_MAX_SPEED);
  stepper.setAcceleration(STEPPER_ACCELERATION);
  deenergizeStepper(); // Giữ motor ngắt điện ban đầu

  // Âm thanh chào mừng khởi động
  digitalWrite(PIN_BUZZER, HIGH);
  delay(100);
  digitalWrite(PIN_BUZZER, LOW);

  Serial.println(F("[INIT] System Ready. Waiting for keypad command..."));
  delay(500);
}

void loop() {
  // 1. Quét phím ma trận 4x4
  char key = keypad.getKey();
  if (key != NO_KEY) {
    Serial.printf("[KEYPAD] Key pressed: '%c'\n", key);
    handleKeypad(key);
  }

  // 2. Xử lý logic vận hành theo trạng thái
  if (g_systemState == STATE_AUTO_RUNNING) {
    if (g_autoPhase == AUTO_PHASE_MOVE) {
      stepper.run();
    }
    processAutoCapture();
  } else if (g_systemState == STATE_MANUAL_RUNNING) {
    stepper.run();
    if (stepper.distanceToGo() == 0) {
      deenergizeStepper(); // Ngắt điện khi đã tới góc chỉ định
    }
  } else if (g_systemState == STATE_CONTINUOUS_RUNNING) {
    stepper.runSpeed();
  }

  // 3. Cập nhật màn hình OLED định kỳ (mỗi 100ms)
  static uint32_t lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate >= 100) {
    lastDisplayUpdate = millis();
    renderUI();
  }
}
