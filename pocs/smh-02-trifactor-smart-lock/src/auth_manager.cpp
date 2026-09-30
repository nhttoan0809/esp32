#include "auth_manager.h"
#include "rtc_manager.h"
#include "feedback.h"
#include <ArduinoJson.h>

AuthManager authManager;

AuthManager::AuthManager()
    : userPin(DEFAULT_USER_PIN),
      webPassword(DEFAULT_ADMIN_PASSWORD),
      masterCardUid(DEFAULT_MASTER_CARD_UID),
      cardCount(0),
      logCount(0),
      logHead(0),
      consecutiveFailures(0),
      lockoutActive(false),
      lockoutStartTime(0) {}

void AuthManager::begin() {
    loadConfiguration();
}

void AuthManager::loadConfiguration() {
    prefs.begin("smh02_auth", false);

    userPin = prefs.getString("pin", DEFAULT_USER_PIN);
    webPassword = prefs.getString("web_pass", DEFAULT_ADMIN_PASSWORD);
    masterCardUid = prefs.getString("master_uid", DEFAULT_MASTER_CARD_UID);

    cardCount = prefs.getInt("card_cnt", 0);
    if (cardCount > MAX_CARDS) cardCount = MAX_CARDS;

    for (int i = 0; i < cardCount; i++) {
        String key = "c_" + String(i);
        authorizedCards[i] = prefs.getString(key.c_str(), "");
    }

    // If no card is present, seed with DEFAULT_MASTER_CARD_UID
    if (cardCount == 0) {
        authorizedCards[0] = DEFAULT_MASTER_CARD_UID;
        cardCount = 1;
        saveCards();
    }

    prefs.end();
}

void AuthManager::saveCards() {
    prefs.begin("smh02_auth", false);
    prefs.putInt("card_cnt", cardCount);
    for (int i = 0; i < cardCount; i++) {
        String key = "c_" + String(i);
        prefs.putString(key.c_str(), authorizedCards[i]);
    }
    prefs.end();
}

bool AuthManager::verifyPin(const String& pin) {
    if (lockoutActive) return false;

    if (pin.length() > 0 && pin == userPin) {
        consecutiveFailures = 0;
        logEvent(AUTH_PIN, "****", true);
        return true;
    }

    consecutiveFailures++;
    logEvent(AUTH_PIN, pin, false);
    if (consecutiveFailures >= MAX_FAILED_ATTEMPTS) {
        triggerLockout();
    }
    return false;
}

bool AuthManager::verifyRfid(const String& uid) {
    if (lockoutActive) return false;

    if (uid.length() == 0) return false;

    if (uid.equalsIgnoreCase(masterCardUid) || isCardAuthorized(uid)) {
        consecutiveFailures = 0;
        logEvent(AUTH_RFID, uid, true);
        return true;
    }

    consecutiveFailures++;
    logEvent(AUTH_RFID, uid, false);
    if (consecutiveFailures >= MAX_FAILED_ATTEMPTS) {
        triggerLockout();
    }
    return false;
}

bool AuthManager::verifyWebPassword(const String& password) {
    if (lockoutActive) return false;

    if (password.length() > 0 && password == webPassword) {
        consecutiveFailures = 0;
        logEvent(AUTH_WEB, "Web Portal", true);
        return true;
    }

    consecutiveFailures++;
    logEvent(AUTH_WEB, "Web Portal", false);
    if (consecutiveFailures >= MAX_FAILED_ATTEMPTS) {
        triggerLockout();
    }
    return false;
}

void AuthManager::triggerLockout() {
    lockoutActive = true;
    lockoutStartTime = millis();
    feedback.startAlarm();
    logEvent(AUTH_NONE, "LOCKOUT TRIGGERED", false);
    Serial.println(F("[SECURITY] 5 failed attempts reached! Lockout activated for 60s!"));
}

bool AuthManager::isLockoutActive() const {
    return lockoutActive;
}

int AuthManager::getRemainingLockoutSeconds() const {
    if (!lockoutActive) return 0;
    unsigned long elapsed = millis() - lockoutStartTime;
    if (elapsed >= LOCKOUT_DURATION_MS) return 0;
    return (int)((LOCKOUT_DURATION_MS - elapsed + 999) / 1000);
}

int AuthManager::getFailedAttempts() const {
    return consecutiveFailures;
}

void AuthManager::logEvent(AuthMethod method, const String& detail, bool success) {
    LogEntry& entry = logs[logHead];
    String ts = rtcManager.getFormattedDateTime();
    strncpy(entry.timestamp, ts.c_str(), sizeof(entry.timestamp) - 1);
    entry.timestamp[sizeof(entry.timestamp) - 1] = '\0';
    entry.method = method;
    strncpy(entry.identifier, detail.c_str(), sizeof(entry.identifier) - 1);
    entry.identifier[sizeof(entry.identifier) - 1] = '\0';
    entry.granted = success;

    logHead = (logHead + 1) % MAX_LOG_ENTRIES;
    if (logCount < MAX_LOG_ENTRIES) {
        logCount++;
    }

    Serial.print(F("[ACCESS LOG] "));
    Serial.print(entry.timestamp);
    Serial.print(F(" | Method: "));
    switch (method) {
        case AUTH_RFID: Serial.print(F("RFID")); break;
        case AUTH_PIN:  Serial.print(F("PIN")); break;
        case AUTH_WEB:  Serial.print(F("WEB")); break;
        default:        Serial.print(F("SYS")); break;
    }
    Serial.print(F(" | Detail: "));
    Serial.print(entry.identifier);
    Serial.print(F(" | Result: "));
    Serial.println(success ? F("GRANTED") : F("DENIED"));
}

bool AuthManager::setUserPin(const String& newPin) {
    if (newPin.length() < 4 || newPin.length() > 8) return false;
    userPin = newPin;
    prefs.begin("smh02_auth", false);
    prefs.putString("pin", userPin);
    prefs.end();
    return true;
}

String AuthManager::getUserPin() const {
    return userPin;
}

bool AuthManager::setWebPassword(const String& newPassword) {
    if (newPassword.length() < 4) return false;
    webPassword = newPassword;
    prefs.begin("smh02_auth", false);
    prefs.putString("web_pass", webPassword);
    prefs.end();
    return true;
}

String AuthManager::getWebPassword() const {
    return webPassword;
}

bool AuthManager::isCardAuthorized(const String& uid) const {
    for (int i = 0; i < cardCount; i++) {
        if (authorizedCards[i].equalsIgnoreCase(uid)) {
            return true;
        }
    }
    return false;
}

bool AuthManager::addAuthorizedCard(const String& uid) {
    if (uid.length() == 0) return false;
    if (isCardAuthorized(uid)) return true;
    if (cardCount >= MAX_CARDS) return false;

    authorizedCards[cardCount++] = uid;
    saveCards();
    return true;
}

bool AuthManager::removeAuthorizedCard(const String& uid) {
    int foundIdx = -1;
    for (int i = 0; i < cardCount; i++) {
        if (authorizedCards[i].equalsIgnoreCase(uid)) {
            foundIdx = i;
            break;
        }
    }
    if (foundIdx == -1) return false;

    for (int i = foundIdx; i < cardCount - 1; i++) {
        authorizedCards[i] = authorizedCards[i + 1];
    }
    cardCount--;
    saveCards();
    return true;
}

String AuthManager::getCardsJson() const {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < cardCount; i++) {
        JsonObject item = arr.add<JsonObject>();
        item["uid"] = authorizedCards[i];
        item["is_master"] = authorizedCards[i].equalsIgnoreCase(masterCardUid);
    }
    String output;
    serializeJson(doc, output);
    return output;
}

String AuthManager::getLogsJson() const {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    // Export in reverse chronological order (newest first)
    for (int i = 0; i < logCount; i++) {
        int idx = (logHead - 1 - i + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;
        const LogEntry& entry = logs[idx];

        JsonObject item = arr.add<JsonObject>();
        item["timestamp"] = entry.timestamp;
        switch (entry.method) {
            case AUTH_RFID: item["method"] = "RFID"; break;
            case AUTH_PIN:  item["method"] = "PIN"; break;
            case AUTH_WEB:  item["method"] = "WEB"; break;
            default:        item["method"] = "SYSTEM"; break;
        }
        item["detail"] = entry.identifier;
        item["status"] = entry.granted ? "GRANTED" : "DENIED";
    }

    String output;
    serializeJson(doc, output);
    return output;
}

void AuthManager::update() {
    if (lockoutActive) {
        if (millis() - lockoutStartTime >= LOCKOUT_DURATION_MS) {
            lockoutActive = false;
            consecutiveFailures = 0;
            feedback.stopAlarm();
            feedback.beepSuccess();
            Serial.println(F("[SECURITY] Lockout duration expired. System normal."));
        }
    }
}
