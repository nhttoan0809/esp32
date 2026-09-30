#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

class WebPortal {
public:
    WebPortal();
    void begin();
    void update();

    bool isConnected() const;
    String getIpAddress() const;

private:
    WebServer server;
    bool wifiConnected;
    unsigned long lastWifiCheck;

    void setupRoutes();
    void handleRoot();
    void handleApiStatus();
    void handleApiUnlock();
    void handleApiLogs();
    void handleApiCards();
    void handleApiAddCard();
    void handleApiRemoveCard();
    void handleApiChangePin();
};

extern WebPortal webPortal;

#endif // WEB_PORTAL_H
