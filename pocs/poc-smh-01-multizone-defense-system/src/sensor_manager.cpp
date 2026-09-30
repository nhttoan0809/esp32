#include "sensor_manager.h"

void SensorManager::begin() {
  // GPIO 34, 35, 39 là các chân Input-Only, không có pullup nội bộ
  pinMode(PIN_ZONE1_PIR, INPUT);
  pinMode(PIN_ZONE2_IR, INPUT);
  pinMode(PIN_ZONE3_SOUND, INPUT);

  // Nút bấm Arm/Disarm có thể dùng INPUT_PULLUP
  pinMode(PIN_BTN_ARM, INPUT_PULLUP);
}

void SensorManager::update() {
  unsigned long now = millis();

  // 1. Đọc PIR HC-SR501 (Vùng 1: Active HIGH)
  int pirVal = digitalRead(PIN_ZONE1_PIR);
  if (pirVal == HIGH) {
    _z1Active = true;
    _lastZ1Time = now;
  } else if (now - _lastZ1Time > 200) {
    _z1Active = false;
  }

  // 2. Đọc LM393 IR Obstacle (Vùng 2: Active LOW khi bị che / cắt tia)
  int irVal = digitalRead(PIN_ZONE2_IR);
  if (irVal == LOW) {
    _z2Active = true;
    _lastZ2Time = now;
  } else if (now - _lastZ2Time > 200) {
    _z2Active = false;
  }

  // 3. Đọc Sound Sensor HW-484 (Vùng 3: Active LOW khi có âm thanh lớn)
  int soundVal = digitalRead(PIN_ZONE3_SOUND);
  if (soundVal == LOW) {
    _z3Active = true;
    _lastZ3Time = now;
  } else if (now - _lastZ3Time > 200) {
    _z3Active = false;
  }

  // 4. Đọc Nút Bấm Arm/Disarm (Debounce non-blocking)
  int reading = digitalRead(PIN_BTN_ARM);
  if (reading != _lastBtnState) {
    _lastDebounceTime = now;
  }

  if ((now - _lastDebounceTime) > SENSOR_DEBOUNCE_MS) {
    if (reading != _btnState) {
      _btnState = reading;
      if (_btnState == LOW) {
        _buttonPressedEvent = true;
      }
    }
  }
  _lastBtnState = reading;
}

bool SensorManager::isZone1Active() const {
  return _z1Active;
}

bool SensorManager::isZone2Active() const {
  return _z2Active;
}

bool SensorManager::isZone3Active() const {
  return _z3Active;
}

bool SensorManager::isArmButtonPressed() {
  if (_buttonPressedEvent) {
    _buttonPressedEvent = false;
    return true;
  }
  return false;
}

SecurityZone SensorManager::checkIntrusion() {
  // Ưu tiên trigger được kích hoạt từ Serial CLI (kiểm thử)
  if (_injectedZone != ZONE_NONE) {
    SecurityZone z = _injectedZone;
    _injectedZone = ZONE_NONE;
    return z;
  }

  // Kiểm tra 3 vùng vật lý theo mức độ ưu tiên
  if (_z3Active) return ZONE_3_SOUND;
  if (_z2Active) return ZONE_2_IR;
  if (_z1Active) return ZONE_1_PIR;

  return ZONE_NONE;
}

void SensorManager::injectTrigger(SecurityZone zone) {
  _injectedZone = zone;
}
