#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>
#include "config.h"

class AuthManager {
public:
  bool begin();
  bool pollRfidCard(String &outUid);
  bool isCardAuthorized(const String &uid);

  bool verifyPin(const String &inputPin);
  bool changePin(const String &oldPin, const String &newPin);

  String getMasterUid() const;
  void addAuthorizedCard(const String &uid);

private:
  MFRC522 _mfrc522{PIN_RFID_SS, PIN_RFID_RST};
  bool _rfidAvailable = false;
  Preferences _prefs;

  String _pinCode = DEFAULT_PIN;
  String _masterUid = DEFAULT_MASTER_UID;
  String _whitelist[5];
  int _whitelistCount = 0;

  void loadCredentialsFromNvs();
  void saveCredentialsToNvs();
};
