#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "types.h"
#include "config.h"

#define MAX_CARDS       10
#define MAX_LOG_ENTRIES 15

class AuthManager {
public:
    AuthManager();
    void begin();
    void update();

    bool verifyPin(const String& pin);
    bool verifyRfid(const String& uid);
    bool verifyWebPassword(const String& password);

    bool isLockoutActive() const;
    int getRemainingLockoutSeconds() const;
    int getFailedAttempts() const;

    void logEvent(AuthMethod method, const String& detail, bool success);

    bool setUserPin(const String& newPin);
    String getUserPin() const;

    bool setWebPassword(const String& newPassword);
    String getWebPassword() const;

    bool addAuthorizedCard(const String& uid);
    bool removeAuthorizedCard(const String& uid);
    bool isCardAuthorized(const String& uid) const;

    String getLogsJson() const;
    String getCardsJson() const;

private:
    Preferences prefs;

    String userPin;
    String webPassword;
    String masterCardUid;

    String authorizedCards[MAX_CARDS];
    int cardCount;

    LogEntry logs[MAX_LOG_ENTRIES];
    int logCount;
    int logHead;

    int consecutiveFailures;
    bool lockoutActive;
    unsigned long lockoutStartTime;

    void loadConfiguration();
    void saveCards();
    void triggerLockout();
};

extern AuthManager authManager;

#endif // AUTH_MANAGER_H
