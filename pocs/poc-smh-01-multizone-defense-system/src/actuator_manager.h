#pragma once
#include <Arduino.h>
#include "config.h"
#include "types.h"

class ActuatorManager {
public:
  void begin();
  void update(SystemState state);

  void setRelayFloodlight(bool on);
  void setRelaySiren(bool on);
  void setAllRelays(bool on);

  void beep(uint16_t freq, uint16_t durationMs);
  void playConfirmArm();
  void playConfirmDisarm();
  void playErrorBeep();

  void silenceAll();

private:
  bool _floodlightOn = false;
  bool _sirenRelayOn = false;

  // Trạng thái còi Buzzer non-blocking
  unsigned long _beepEndTime = 0;
  bool _isBeeping = false;

  // Siren sweep (chế độ cảnh sát trong ALARM)
  uint16_t _sirenFreq = 1500;
  bool _sirenAscending = true;
  unsigned long _lastSirenStepTime = 0;

  // Nhịp đèn LED & Còi trong ENTRY/EXIT DELAY
  unsigned long _lastChirpTime = 0;
  unsigned long _lastStrobeTime = 0;
  bool _strobeState = false;
};
