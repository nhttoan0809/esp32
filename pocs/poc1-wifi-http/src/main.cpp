#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

#if __has_include("secrets.h")
#include "secrets.h"
constexpr int32_t WIFI_CHANNEL = 0;
#else
constexpr char WIFI_SSID[] = "Wokwi-GUEST";
constexpr char WIFI_PASSWORD[] = "";
constexpr int32_t WIFI_CHANNEL = 6;
#endif

namespace {

constexpr char API_URL[] =
    "http://jsonplaceholder.typicode.com/todos/1";
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t RETRY_INTERVAL_MS = 5000;
constexpr uint32_t REQUEST_INTERVAL_MS = 15000;
constexpr uint16_t HTTP_TIMEOUT_MS = 5000;

bool wifiAttemptActive = false;
bool wifiWasConnected = false;
uint32_t wifiAttemptStartedAt = 0;
uint32_t nextWifiAttemptAt = 0;
uint32_t nextHttpRequestAt = 0;

bool deadlineReached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

void startWifiConnection(uint32_t now) {
  Serial.printf("WIFI_CONNECTING ssid=%s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  wifiAttemptStartedAt = now;
  wifiAttemptActive = true;
}

void handleWifi(uint32_t now) {
  const bool connected = WiFi.status() == WL_CONNECTED;

  if (connected) {
    wifiAttemptActive = false;
    if (!wifiWasConnected) {
      wifiWasConnected = true;
      Serial.printf("WIFI_CONNECTED ip=%s rssi=%d\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
      nextHttpRequestAt = now;
    }
    return;
  }

  if (wifiWasConnected) {
    wifiWasConnected = false;
    wifiAttemptActive = false;
    nextWifiAttemptAt = now + RETRY_INTERVAL_MS;
    Serial.printf("WIFI_DISCONNECTED status=%d\n", WiFi.status());
    Serial.printf("WIFI_RETRY_IN_MS %lu\n",
                  static_cast<unsigned long>(RETRY_INTERVAL_MS));
  }

  if (wifiAttemptActive) {
    if (now - wifiAttemptStartedAt >= WIFI_CONNECT_TIMEOUT_MS) {
      Serial.printf("WIFI_CONNECT_TIMEOUT status=%d\n", WiFi.status());
      WiFi.disconnect();
      wifiAttemptActive = false;
      nextWifiAttemptAt = now + RETRY_INTERVAL_MS;
      Serial.printf("WIFI_RETRY_IN_MS %lu\n",
                    static_cast<unsigned long>(RETRY_INTERVAL_MS));
    }
    return;
  }

  if (deadlineReached(now, nextWifiAttemptAt)) {
    startWifiConnection(now);
  }
}

bool performHttpGet() {
  WiFiClient client;
  HTTPClient http;
  const char *responseHeaders[] = {"Content-Type"};

  http.setConnectTimeout(HTTP_TIMEOUT_MS);
  http.setTimeout(HTTP_TIMEOUT_MS);
  http.collectHeaders(responseHeaders, 1);

  Serial.printf("HTTP_REQUEST method=GET url=%s\n", API_URL);
  if (!http.begin(client, API_URL)) {
    Serial.println("HTTP_ERROR code=-1000 message=begin failed");
    http.end();
    return false;
  }

  http.addHeader("Accept", "application/json");
  const int status = http.GET();
  bool succeeded = false;

  if (status > 0) {
    Serial.printf("HTTP_RESPONSE status=%d content_type=%s\n", status,
                  http.header("Content-Type").c_str());
    Serial.printf("HTTP_BODY %s\n", http.getString().c_str());
    succeeded = status == HTTP_CODE_OK;
  } else {
    Serial.printf("HTTP_ERROR code=%d message=%s\n", status,
                  HTTPClient::errorToString(status).c_str());
  }

  http.end();
  return succeeded;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  startWifiConnection(millis());
}

void loop() {
  const uint32_t now = millis();
  handleWifi(now);

  if (WiFi.status() == WL_CONNECTED &&
      deadlineReached(now, nextHttpRequestAt)) {
    const bool succeeded = performHttpGet();
    const uint32_t interval =
        succeeded ? REQUEST_INTERVAL_MS : RETRY_INTERVAL_MS;
    nextHttpRequestAt = millis() + interval;
    Serial.printf(succeeded ? "HTTP_NEXT_IN_MS %lu\n"
                            : "HTTP_RETRY_IN_MS %lu\n",
                  static_cast<unsigned long>(interval));
  }

  delay(10);
}
