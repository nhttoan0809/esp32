#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

#include "web_ui.h"

namespace {

constexpr uint8_t LED_PIN = 2;
constexpr uint16_t HTTP_PORT = 80;

WebServer server(HTTP_PORT);
bool ledOn = false;

const char *ledWord() {
  return ledOn ? "on" : "off";
}

String ledStateJson() {
  String response;
  response.reserve(64);
  response = F("{\"device\":\"esp32dev\",\"led\":{\"pin\":2,\"on\":");
  response += ledOn ? F("true") : F("false");
  response += F("}}");
  return response;
}

void logRequest(const char *method, int status) {
  Serial.printf("HTTP_REQUEST method=%s path=%s status=%d led=%s\r\n", method,
                server.uri().c_str(), status, ledWord());
}

void sendLedState(const char *method) {
  logRequest(method, 200);
  server.send(200, "application/json", ledStateJson());
}

void setLed(bool on) {
  digitalWrite(LED_PIN, on ? HIGH : LOW);
  ledOn = on;
}

void registerRoutes() {
  server.on("/", HTTP_GET, []() {
    logRequest("GET", 200);
    server.send_P(200, PSTR("text/html; charset=utf-8"), WEB_UI);
  });

  server.on("/api/led", HTTP_GET, []() { sendLedState("GET"); });

  server.on("/api/led/on", HTTP_POST, []() {
    setLed(true);
    sendLedState("POST");
  });

  server.on("/api/led/off", HTTP_POST, []() {
    setLed(false);
    sendLedState("POST");
  });

  server.onNotFound([]() {
    logRequest(server.method() == HTTP_GET ? "GET" :
               server.method() == HTTP_POST ? "POST" : "OTHER",
               404);
    server.send(404, "application/json", "{\"error\":\"not_found\"}");
  });
}

}  // namespace

void setup() {
  pinMode(LED_PIN, OUTPUT);
  setLed(false);

  Serial.begin(115200);
  Serial.println();
  Serial.printf("WIFI_CONNECTING ssid=%s\r\n", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }

  Serial.printf("WIFI_CONNECTED ip=%s\r\n",
                WiFi.localIP().toString().c_str());

  registerRoutes();
  server.begin();
  Serial.printf("HTTP_SERVER_STARTED port=%u\r\n", HTTP_PORT);
}

void loop() {
  server.handleClient();
  delay(2);
}
