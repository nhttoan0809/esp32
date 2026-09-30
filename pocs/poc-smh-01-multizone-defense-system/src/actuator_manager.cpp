#include "actuator_manager.h"

void ActuatorManager::begin() {
  pinMode(PIN_RELAY_FLOODLIGHT, OUTPUT);
  pinMode(PIN_RELAY_SIREN, OUTPUT);
  pinMode(PIN_LED_ALARM, OUTPUT);
  pinMode(PIN_LED_DISARMED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // Mặc định ngắt toàn bộ tải
  silenceAll();
}

void ActuatorManager::setRelayFloodlight(bool on) {
  _floodlightOn = on;
  digitalWrite(PIN_RELAY_FLOODLIGHT, on ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL);
}

void ActuatorManager::setRelaySiren(bool on) {
  _sirenRelayOn = on;
  digitalWrite(PIN_RELAY_SIREN, on ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL);
}

void ActuatorManager::setAllRelays(bool on) {
  setRelayFloodlight(on);
  setRelaySiren(on);
}

void ActuatorManager::beep(uint16_t freq, uint16_t durationMs) {
  tone(PIN_BUZZER, freq);
  _beepEndTime = millis() + durationMs;
  _isBeeping = true;
}

void ActuatorManager::playConfirmArm() {
  // 2 tiếng bíp đanh gọn
  beep(2400, 100);
}

void ActuatorManager::playConfirmDisarm() {
  // 1 tiếng bíp dài êm ái
  beep(1800, 300);
}

void ActuatorManager::playErrorBeep() {
  // Tiếng bíp trầm báo lỗi
  beep(800, 400);
}

void ActuatorManager::silenceAll() {
  setAllRelays(false);
  noTone(PIN_BUZZER);
  digitalWrite(PIN_BUZZER, LOW);
  _isBeeping = false;
  digitalWrite(PIN_LED_ALARM, LOW);
  digitalWrite(PIN_LED_DISARMED, LOW);
}

void ActuatorManager::update(SystemState state) {
  unsigned long now = millis();

  // 1. Quản lý thời gian dừng tiếng bíp đơn lẻ
  if (_isBeeping && now >= _beepEndTime && state != STATE_ALARM) {
    noTone(PIN_BUZZER);
    digitalWrite(PIN_BUZZER, LOW);
    _isBeeping = false;
  }

  // 2. Điều khiển trạng thái LED & Ngoại vi theo FSM
  switch (state) {
    case STATE_INIT:
    case STATE_DISARMED:
      digitalWrite(PIN_LED_DISARMED, HIGH); // Đèn xanh sáng tĩnh
      digitalWrite(PIN_LED_ALARM, LOW);     // Đèn đỏ tắt
      if (_floodlightOn || _sirenRelayOn) {
        setAllRelays(false);
      }
      break;

    case STATE_EXIT_DELAY:
      digitalWrite(PIN_LED_DISARMED, LOW);
      setAllRelays(false);

      // Nhấp nháy đèn đỏ chậm 1Hz & bíp mỗi 1 giây
      if (now - _lastChirpTime >= 1000) {
        _lastChirpTime = now;
        beep(2200, 80);
      }
      if (now - _lastStrobeTime >= 500) {
        _lastStrobeTime = now;
        _strobeState = !_strobeState;
        digitalWrite(PIN_LED_ALARM, _strobeState ? HIGH : LOW);
      }
      break;

    case STATE_ARMED:
      digitalWrite(PIN_LED_DISARMED, LOW);
      digitalWrite(PIN_LED_ALARM, HIGH); // Đèn đỏ sáng tĩnh
      if (_floodlightOn || _sirenRelayOn) {
        setAllRelays(false);
      }
      break;

    case STATE_ENTRY_DELAY:
      digitalWrite(PIN_LED_DISARMED, LOW);
      setAllRelays(false);

      // Bíp dồn dập 3 tiếng/giây nhắc quẹt thẻ
      if (now - _lastChirpTime >= 330) {
        _lastChirpTime = now;
        beep(2800, 60);
      }
      // Đèn đỏ chớp nhanh
      if (now - _lastStrobeTime >= 165) {
        _lastStrobeTime = now;
        _strobeState = !_strobeState;
        digitalWrite(PIN_LED_ALARM, _strobeState ? HIGH : LOW);
      }
      break;

    case STATE_ALARM:
      digitalWrite(PIN_LED_DISARMED, LOW);

      // Bật 2 kênh Relay: Đèn pha & Còi hú công suất lớn
      if (!_floodlightOn || !_sirenRelayOn) {
        setAllRelays(true);
      }

      // Đèn đỏ nhấp nháy Strobe cường độ cao nhịp 80ms
      if (now - _lastStrobeTime >= 80) {
        _lastStrobeTime = now;
        _strobeState = !_strobeState;
        digitalWrite(PIN_LED_ALARM, _strobeState ? HIGH : LOW);
      }

      // Hú còi âm điệu cảnh sát (Siren sweep 1500Hz -> 3000Hz)
      if (now - _lastSirenStepTime >= 15) {
        _lastSirenStepTime = now;
        if (_sirenAscending) {
          _sirenFreq += 40;
          if (_sirenFreq >= 3000) _sirenAscending = false;
        } else {
          _sirenFreq -= 40;
          if (_sirenFreq <= 1500) _sirenAscending = true;
        }
        tone(PIN_BUZZER, _sirenFreq);
      }
      break;
  }
}
