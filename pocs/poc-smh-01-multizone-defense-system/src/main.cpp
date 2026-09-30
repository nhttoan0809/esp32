#include <Arduino.h>
#include "config.h"
#include "types.h"
#include "sensor_manager.h"
#include "actuator_manager.h"
#include "rtc_manager.h"
#include "display_manager.h"
#include "auth_manager.h"
#include "network_manager.h"

// ============================================================================
// ĐỐI TƯỢNG QUẢN LÝ CÁC PHÂN HỆ
// ============================================================================
SensorManager   sensors;
ActuatorManager actuators;
RtcManager      rtc;
DisplayManager  display;
AuthManager     auth;
NetworkManager  network;

// ============================================================================
// BIẾN QUẢN LÝ MÁY TRẠNG THÁI (FSM)
// ============================================================================
SystemState   currentState = STATE_INIT;
unsigned long stateStartTime = 0;
SecurityZone  breachedZone = ZONE_NONE;

// Buffer nhận lệnh từ Serial Terminal
String serialBuffer = "";

// ============================================================================
// CÁC HÀM HỖ TRỢ CHUYỂN TRẠNG THÁI
// ============================================================================
void transitionTo(SystemState newState, const char* reason = "") {
  SystemState oldState = currentState;
  currentState = newState;
  stateStartTime = millis();

  Serial.println("--------------------------------------------------");
  Serial.printf("[FSM CHUYEN TRANG THAI] %s -> %s\n", getStateName(oldState), getStateName(newState));
  if (strlen(reason) > 0) {
    Serial.printf("  Ly do: %s\n", reason);
  }
  Serial.println("--------------------------------------------------");

  if (newState == STATE_DISARMED) {
    actuators.silenceAll();
    actuators.playConfirmDisarm();
    breachedZone = ZONE_NONE;
  } else if (newState == STATE_EXIT_DELAY) {
    actuators.playConfirmArm();
  } else if (newState == STATE_ARMED) {
    actuators.playConfirmArm();
    breachedZone = ZONE_NONE;
  } else if (newState == STATE_ALARM) {
    Serial.println("🚨 🚨 🚨 [CANH BAO AN NINH TOAN DIEN] 🚨 🚨 🚨");
    Serial.printf("Vi pham tai: %s\n", getZoneName(breachedZone));
    Serial.println("Dang kich hoat: Den pha Relay 1, Coi hu Relay 2, Strobe LED, Siren Buzzer!");
    rtc.logEvent(breachedZone, "SENSOR", "Xam nhap vung bao ve");
    network.sendTelegramAlert(breachedZone, rtc.getFormattedTime());
  }
}

void printHelpMenu() {
  Serial.println("\n==================================================");
  Serial.println("   MENU DIEU KHIEN DONG LENH SERIAL CLI (SMH-01)");
  Serial.println("==================================================");
  Serial.println("  a       : Kich hoat che do Bao Ve (ARM)");
  Serial.println("  d [pin] : Giai tru bao ve (DISARM, mac dinh PIN: 1234)");
  Serial.println("  1       : Gia lap vi pham Vung 1 (PIR Hanh lang)");
  Serial.println("  2       : Gia lap vi pham Vung 2 (IR Cua so/Hang rao)");
  Serial.println("  3       : Gia lap vi pham Vung 3 (Am thanh vo kinh)");
  Serial.println("  l       : Xem danh sach Nhat ky su kien (Audit Logs)");
  Serial.println("  c       : Xoa toan bo nhat ky trong NVS");
  Serial.println("  s       : Xem trang thai tong the he thong");
  Serial.println("  ?       : In menu tro giup nay");
  Serial.println("==================================================\n");
}

void printStatus() {
  Serial.println("\n==================================================");
  Serial.println("          THONG SO HE THONG SMH-01");
  Serial.println("==================================================");
  Serial.printf("  Trang thai hien tai: %s\n", getStateName(currentState));
  Serial.printf("  Thoi gian hien tai : %s\n", rtc.getFormattedTime().c_str());
  Serial.printf("  Ket noi Wi-Fi      : %s\n", network.isConnected() ? "DA KET NOI" : "OFFLINE");
  Serial.printf("  Vung 1 (PIR)       : %s\n", sensors.isZone1Active() ? "CO CHUYEN DONG!" : "BINH THUONG");
  Serial.printf("  Vung 2 (IR)        : %s\n", sensors.isZone2Active() ? "CAT TIA!" : "BINH THUONG");
  Serial.printf("  Vung 3 (Sound)     : %s\n", sensors.isZone3Active() ? "CO TIENG DONG!" : "BINH THUONG");
  Serial.printf("  So log da luu      : %d / %d\n", rtc.getLogCount(), MAX_LOG_ENTRIES);
  Serial.println("==================================================\n");
}

void handleSerialCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  char firstChar = cmd.charAt(0);

  if (cmd.equalsIgnoreCase("a") || cmd.equalsIgnoreCase("arm")) {
    if (currentState == STATE_DISARMED) {
      transitionTo(STATE_EXIT_DELAY, "Lenh ARM tu Serial");
    } else {
      Serial.println("[CLI] He thong da o che do ARMED hoac dang bao dong!");
    }
  } else if (firstChar == 'd' || cmd.startsWith("disarm")) {
    String pin = DEFAULT_PIN;
    int spaceIdx = cmd.indexOf(' ');
    if (spaceIdx > 0) {
      pin = cmd.substring(spaceIdx + 1);
      pin.trim();
    }
    if (auth.verifyPin(pin)) {
      transitionTo(STATE_DISARMED, "Lenh DISARM hop le tu Serial");
    } else {
      Serial.println("[CLI] SAI MA PIN! Tu choi giai tru he thong.");
      actuators.playErrorBeep();
    }
  } else if (cmd == "1") {
    Serial.println("[CLI] Gia lap vi pham Vung 1 (PIR)...");
    sensors.injectTrigger(ZONE_1_PIR);
  } else if (cmd == "2") {
    Serial.println("[CLI] Gia lap vi pham Vung 2 (IR)...");
    sensors.injectTrigger(ZONE_2_IR);
  } else if (cmd == "3") {
    Serial.println("[CLI] Gia lap vi pham Vung 3 (Sound)...");
    sensors.injectTrigger(ZONE_3_SOUND);
  } else if (cmd.equalsIgnoreCase("l") || cmd.equalsIgnoreCase("logs")) {
    rtc.printAllLogs();
  } else if (cmd.equalsIgnoreCase("c") || cmd.equalsIgnoreCase("clear")) {
    rtc.clearLogs();
  } else if (cmd.equalsIgnoreCase("s") || cmd.equalsIgnoreCase("status")) {
    printStatus();
  } else if (cmd == "?") {
    printHelpMenu();
  } else {
    Serial.printf("[CLI] Lenh khong hop le: '%s'. Go '?' de xem huong dan.\n", cmd.c_str());
  }
}

// ============================================================================
// ARDUINO SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n\n==================================================");
  Serial.println("  POC SMH-01: MULTI-ZONE DEFENSE SYSTEM");
  Serial.println("  ESP32 DevKit V1 (30-pin) | PlatformIO CLI");
  Serial.println("==================================================");

  // 1. Khởi tạo cơ cấu chấp hành trước để đưa chân về trạng thái an toàn
  actuators.begin();

  // 2. Khởi tạo cảm biến
  sensors.begin();

  // 3. Khởi tạo RTC
  rtc.begin();

  // 4. Khởi tạo màn hình OLED
  display.begin();

  // 5. Khởi tạo RFID RC522
  auth.begin();

  // 6. Khởi tạo mạng Wi-Fi
  network.begin();

  // 7. Chạy kiểm tra Power-on Self Test (POST)
  Serial.println("[POST] Chay kiem tra phan cung khoi dong...");
  actuators.beep(2000, 100);
  delay(150);
  actuators.beep(2500, 100);

  // Chuyển sang trạng thái ban đầu: AN TOÀN (DISARMED)
  transitionTo(STATE_DISARMED, "Khoi dong he thong thanh cong");

  Serial.println("\n[SMH-01] SYSTEM READY! Multi-Zone Defense Active.");
  printHelpMenu();
}

// ============================================================================
// ARDUINO MAIN LOOP
// ============================================================================
void loop() {
  unsigned long now = millis();

  // 1. Cập nhật các phân hệ phần cứng
  sensors.update();
  actuators.update(currentState);
  network.update();

  // 2. Quét thẻ từ RFID RC522
  String scannedUid = "";
  if (auth.pollRfidCard(scannedUid)) {
    Serial.printf("\n[RFID] Phat hien the tu: %s\n", scannedUid.c_str());
    if (auth.isCardAuthorized(scannedUid)) {
      Serial.println("[RFID] THE HOP LE! Thao tac Arm/Disarm he thong.");
      if (currentState == STATE_DISARMED) {
        transitionTo(STATE_EXIT_DELAY, "Quet the Master ARM he thong");
      } else {
        transitionTo(STATE_DISARMED, "Quet the Master DISARM he thong");
      }
    } else {
      Serial.println("[RFID] CANH BAO: THE KHONG HOP LE!");
      actuators.playErrorBeep();
    }
  }

  // 3. Xử lý nút bấm Arm/Disarm vật lý (GPIO 13)
  if (sensors.isArmButtonPressed()) {
    Serial.println("\n[BUTTON] Nhan nut Arm/Disarm toggle...");
    if (currentState == STATE_DISARMED) {
      transitionTo(STATE_EXIT_DELAY, "Nhan nut chuyen che do ARM");
    } else {
      transitionTo(STATE_DISARMED, "Nhan nut chuyen che do DISARM");
    }
  }

  // 4. Đọc lệnh từ Serial Monitor
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        handleSerialCommand(serialBuffer);
        serialBuffer = "";
      }
    } else {
      serialBuffer += c;
    }
  }

  // 5. Logic xử lý máy trạng thái (FSM)
  uint8_t remainingSeconds = 0;

  switch (currentState) {
    case STATE_INIT:
      // Đã xử lý trong setup()
      break;

    case STATE_DISARMED:
      // Ở chế độ an toàn, chỉ hiển thị trạng thái cảm biến, không kích hoạt báo động
      break;

    case STATE_EXIT_DELAY: {
      unsigned long elapsed = now - stateStartTime;
      if (elapsed >= EXIT_DELAY_MS) {
        transitionTo(STATE_ARMED, "Het thoi gian Exit Delay 15s");
      } else {
        remainingSeconds = (EXIT_DELAY_MS - elapsed) / 1000 + 1;
      }
      break;
    }

    case STATE_ARMED: {
      SecurityZone intrusion = sensors.checkIntrusion();
      if (intrusion != ZONE_NONE) {
        breachedZone = intrusion;
        Serial.printf("\n⚠️ [CANH BAO] PHAT HIEN XAM NHAP TAI: %s!\n", getZoneName(intrusion));

        // Vùng 3 (Âm thanh vỡ kính / cậy cửa): Kích hoạt báo động ALARM tức thời!
        if (intrusion == ZONE_3_SOUND) {
          transitionTo(STATE_ALARM, "Phat hien am thanh vo kinh - Kich hoat bao dong tuc thi!");
        } else {
          // Vùng 1 (PIR) hoặc Vùng 2 (IR): Kích hoạt thời gian trễ vào nhà (Entry Delay)
          transitionTo(STATE_ENTRY_DELAY, "Phat hien chuyen dong - Kich hoat Entry Delay 15s");
        }
      }
      break;
    }

    case STATE_ENTRY_DELAY: {
      unsigned long elapsed = now - stateStartTime;
      if (elapsed >= ENTRY_DELAY_MS) {
        transitionTo(STATE_ALARM, "Het thoi gian Entry Delay 15s khong giai tru!");
      } else {
        remainingSeconds = (ENTRY_DELAY_MS - elapsed) / 1000 + 1;
      }
      break;
    }

    case STATE_ALARM: {
      // Báo động duy trì đến khi quẹt thẻ hoặc sau thời gian tối đa ALARM_DURATION_MS
      if (now - stateStartTime >= ALARM_DURATION_MS) {
        Serial.println("[ALARM] Da het thoi gian bao dong 60s, tam ngung coi de tranh chay loa...");
        actuators.silenceAll();
      }
      break;
    }
  }

  // 6. Cập nhật giao diện màn hình OLED SSD1306
  display.update(currentState,
                 rtc.getFormattedTime(),
                 sensors.isZone1Active(),
                 sensors.isZone2Active(),
                 sensors.isZone3Active(),
                 remainingSeconds,
                 breachedZone);

  delay(10);
}
