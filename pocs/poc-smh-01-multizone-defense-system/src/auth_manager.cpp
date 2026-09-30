#include "auth_manager.h"

bool AuthManager::begin() {
  SPI.begin(PIN_RFID_SCK, PIN_RFID_MISO, PIN_RFID_MOSI, PIN_RFID_SS);
  _mfrc522.PCD_Init();

  byte version = _mfrc522.PCD_ReadRegister(_mfrc522.VersionReg);
  Serial.printf("[RFID] MFRC522 Firmware Version: 0x%02X\n", version);

  if (version == 0x00 || version == 0xFF) {
    Serial.println("[RFID] CANH BAO: Khong tim thay module RC522. Kiem tra day noi SPI!");
    _rfidAvailable = false;
  } else {
    Serial.println("[RFID] Khoi tao thanh cong! San sang quet the tu 13.56MHz.");
    _rfidAvailable = true;
  }

  loadCredentialsFromNvs();
  return _rfidAvailable;
}

bool AuthManager::pollRfidCard(String &outUid) {
  if (!_rfidAvailable) return false;

  if (!_mfrc522.PICC_IsNewCardPresent()) return false;
  if (!_mfrc522.PICC_ReadCardSerial()) return false;

  char uidBuf[32] = "";
  for (byte i = 0; i < _mfrc522.uid.size; i++) {
    char byteStr[6];
    snprintf(byteStr, sizeof(byteStr), "%s%02X", (i > 0 ? ":" : ""), _mfrc522.uid.uidByte[i]);
    strcat(uidBuf, byteStr);
  }
  outUid = String(uidBuf);

  _mfrc522.PICC_HaltA();
  _mfrc522.PCD_StopCrypto1();

  return true;
}

bool AuthManager::isCardAuthorized(const String &uid) {
  // So khớp với thẻ Master
  if (uid.equalsIgnoreCase(_masterUid)) {
    return true;
  }
  // So khớp với danh sách Whitelist
  for (int i = 0; i < _whitelistCount; i++) {
    if (uid.equalsIgnoreCase(_whitelist[i])) {
      return true;
    }
  }
  return false;
}

bool AuthManager::verifyPin(const String &inputPin) {
  return inputPin.equals(_pinCode);
}

bool AuthManager::changePin(const String &oldPin, const String &newPin) {
  if (verifyPin(oldPin)) {
    if (newPin.length() >= 4) {
      _pinCode = newPin;
      saveCredentialsToNvs();
      Serial.printf("[AUTH] Doi ma PIN thanh cong sang: %s\n", _pinCode.c_str());
      return true;
    }
  }
  return false;
}

String AuthManager::getMasterUid() const {
  return _masterUid;
}

void AuthManager::addAuthorizedCard(const String &uid) {
  if (_whitelistCount < 5) {
    _whitelist[_whitelistCount++] = uid;
    saveCredentialsToNvs();
    Serial.printf("[AUTH] Da them the vao whitelist: %s\n", uid.c_str());
  }
}

void AuthManager::loadCredentialsFromNvs() {
  _prefs.begin("smh_auth", true);
  _pinCode = _prefs.getString("pin", DEFAULT_PIN);
  _masterUid = _prefs.getString("master_uid", DEFAULT_MASTER_UID);
  _whitelistCount = _prefs.getInt("wl_count", 0);
  for (int i = 0; i < _whitelistCount; i++) {
    char key[16];
    snprintf(key, sizeof(key), "wl_%d", i);
    _whitelist[i] = _prefs.getString(key, "");
  }
  _prefs.end();

  Serial.printf("[AUTH] PIN hien tai: %s | Master UID: %s\n", _pinCode.c_str(), _masterUid.c_str());
}

void AuthManager::saveCredentialsToNvs() {
  _prefs.begin("smh_auth", false);
  _prefs.putString("pin", _pinCode);
  _prefs.putString("master_uid", _masterUid);
  _prefs.putInt("wl_count", _whitelistCount);
  for (int i = 0; i < _whitelistCount; i++) {
    char key[16];
    snprintf(key, sizeof(key), "wl_%d", i);
    _prefs.putString(key, _whitelist[i]);
  }
  _prefs.end();
}
