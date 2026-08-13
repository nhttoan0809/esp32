#include <Arduino.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

#include "provisioning_config.h"
#include "provisioning_page.h"

using namespace provisioning_config;

namespace {

enum class ProvisioningState {
  Provisioning,
  Connecting,
  Verifying,
  Connected,
  Failed,
};

enum class CredentialSource {
  None,
  Pending,
  Stored,
};

struct Credentials {
  String ssid;
  String password;
  bool valid = false;
};

WebServer server(80);
ProvisioningState state = ProvisioningState::Provisioning;
CredentialSource activeSource = CredentialSource::None;
Credentials storedCredentials;
Credentials pendingCredentials;
String apSsid;
String lastError;
uint32_t connectionStartedAt = 0;
uint32_t fallbackAt = 0;
bool fallbackScheduled = false;
bool upstreamReachable = false;
int upstreamStatus = 0;
uint8_t previousApClients = 0;
bool attemptStartScheduled = false;
String configureBody;
bool configureBodyComplete = false;
bool configureBodyTooLarge = false;

void clearSensitiveString(String &value) {
  for (size_t index = 0; index < value.length(); ++index) {
    value.setCharAt(index, '\0');
  }
  value = "";
}

void clearCredentials(Credentials &credentials) {
  clearSensitiveString(credentials.password);
  credentials.ssid = "";
  credentials.valid = false;
}

bool deadlineReached(uint32_t now, uint32_t deadline) {
  return static_cast<int32_t>(now - deadline) >= 0;
}

const char *stateName() {
  switch (state) {
    case ProvisioningState::Connecting:
      return "connecting";
    case ProvisioningState::Verifying:
      return "verifying";
    case ProvisioningState::Connected:
      return "connected";
    case ProvisioningState::Failed:
      return "failed";
    case ProvisioningState::Provisioning:
    default:
      return "provisioning";
  }
}

void clearPendingCredentials() {
  clearCredentials(pendingCredentials);
}

String jsonEscape(const String &value) {
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t index = 0; index < value.length(); ++index) {
    const uint8_t character = static_cast<uint8_t>(value[index]);
    switch (character) {
      case '"':
        escaped += F("\\\"");
        break;
      case '\\':
        escaped += F("\\\\");
        break;
      case '\b':
        escaped += F("\\b");
        break;
      case '\f':
        escaped += F("\\f");
        break;
      case '\n':
        escaped += F("\\n");
        break;
      case '\r':
        escaped += F("\\r");
        break;
      case '\t':
        escaped += F("\\t");
        break;
      default:
        if (character < 0x20) {
          char encoded[7];
          snprintf(encoded, sizeof(encoded), "\\u%04x", character);
          escaped += encoded;
        } else {
          escaped += static_cast<char>(character);
        }
    }
  }
  return escaped;
}

void addNoStoreHeaders() {
  server.sendHeader(F("Cache-Control"), F("no-store"));
  server.sendHeader(F("Pragma"), F("no-cache"));
}

void sendJson(int statusCode, const String &body) {
  addNoStoreHeaders();
  server.send(statusCode, F("application/json"), body);
}

void sendError(int statusCode, const __FlashStringHelper *message) {
  sendJson(statusCode,
           String(F("{\"error\":\"")) + String(message) + F("\"}"));
}

bool containsControlByte(const String &value) {
  for (size_t index = 0; index < value.length(); ++index) {
    const uint8_t character = static_cast<uint8_t>(value[index]);
    if (character < 0x20 || character == 0x7f) {
      return true;
    }
  }
  return false;
}

bool validPassword(const String &password) {
  if (password.isEmpty()) {
    return true;
  }
  if (password.length() < 8 || password.length() > 63) {
    return false;
  }
  for (size_t index = 0; index < password.length(); ++index) {
    const uint8_t character = static_cast<uint8_t>(password[index]);
    if (character < 0x20 || character > 0x7e) {
      return false;
    }
  }
  return true;
}

bool loadStoredCredentials() {
  Preferences preferences;
  if (!preferences.begin(NVS_NAMESPACE, true)) {
    return false;
  }

  const bool valid = preferences.getBool(NVS_VALID_KEY, false);
  const String ssid = valid ? preferences.getString(NVS_SSID_KEY, "") : "";
  String password =
      valid ? preferences.getString(NVS_PASSWORD_KEY, "") : "";
  preferences.end();

  const bool credentialsValid =
      valid && !ssid.isEmpty() && ssid.length() <= 32 &&
      !containsControlByte(ssid) && validPassword(password);
  if (!credentialsValid) {
    clearSensitiveString(password);
    return false;
  }

  storedCredentials.ssid = ssid;
  storedCredentials.password = password;
  storedCredentials.valid = true;
  clearSensitiveString(password);
  return true;
}

bool commitPendingCredentials() {
  Preferences preferences;
  if (!preferences.begin(NVS_NAMESPACE, false)) {
    return false;
  }

  // The marker is invalidated only after the pending network has passed both
  // DHCP and the upstream probe. valid=true is always written last.
  const bool markerInvalidated =
      preferences.putBool(NVS_VALID_KEY, false) == 1;
  const bool ssidWritten =
      preferences.putString(NVS_SSID_KEY, pendingCredentials.ssid) ==
      pendingCredentials.ssid.length();
  const size_t passwordBytes =
      preferences.putString(NVS_PASSWORD_KEY, pendingCredentials.password);
  // Preferences::putString() returns strlen(value), so a successful empty
  // password write reports zero. Readback below disambiguates that case.
  const bool passwordWriteReported =
      pendingCredentials.password.isEmpty() ||
      passwordBytes == pendingCredentials.password.length();
  const bool valuesReadBack =
      preferences.getString(NVS_SSID_KEY, "") == pendingCredentials.ssid &&
      preferences.getString(NVS_PASSWORD_KEY, "") ==
          pendingCredentials.password;
  const bool valuesWritten = markerInvalidated && ssidWritten &&
                             passwordWriteReported && valuesReadBack;
  const bool markerCommitted =
      valuesWritten && preferences.putBool(NVS_VALID_KEY, true) == 1;
  preferences.end();
  return markerCommitted;
}

bool resetStoredCredentials() {
  Preferences preferences;
  if (!preferences.begin(NVS_NAMESPACE, false)) {
    return false;
  }
  const bool markerCleared =
      preferences.putBool(NVS_VALID_KEY, false) == 1;
  preferences.remove(NVS_SSID_KEY);
  preferences.remove(NVS_PASSWORD_KEY);
  preferences.end();
  return markerCleared;
}

const Credentials *credentialsFor(CredentialSource source) {
  if (source == CredentialSource::Pending) {
    return &pendingCredentials;
  }
  if (source == CredentialSource::Stored) {
    return &storedCredentials;
  }
  return nullptr;
}

void queueStationAttempt(CredentialSource source) {
  activeSource = source;
  state = ProvisioningState::Connecting;
  attemptStartScheduled = true;
  fallbackScheduled = false;
  upstreamReachable = false;
  upstreamStatus = 0;
  lastError = "";
}

bool startStationAttempt() {
  const CredentialSource source = activeSource;
  const Credentials *credentials = credentialsFor(source);
  if (credentials == nullptr || !credentials->valid) {
    return false;
  }

  WiFi.disconnect(false, false);
  const wl_status_t initialStatus =
      WiFi.begin(credentials->ssid.c_str(), credentials->password.c_str());
  if (initialStatus == WL_CONNECT_FAILED) {
    return false;
  }

  connectionStartedAt = millis();
  Serial.printf("STA_CONNECTING ssid=%s source=%s timeout_ms=%lu\n",
                credentials->ssid.c_str(),
                source == CredentialSource::Pending ? "pending" : "stored",
                static_cast<unsigned long>(STA_CONNECT_TIMEOUT_MS));
  return true;
}

void failAttempt(const __FlashStringHelper *reason) {
  const bool pendingFailed = activeSource == CredentialSource::Pending;
  activeSource = CredentialSource::None;
  attemptStartScheduled = false;
  state = ProvisioningState::Failed;
  lastError = String(reason);
  upstreamReachable = false;
  WiFi.disconnect(false, false);

  Serial.printf("STA_CONNECTION_FAILED reason=%s status=%d\n",
                lastError.c_str(), WiFi.status());
  Serial.println("PROVISIONING_STATE failed ap_available=true");

  if (pendingFailed) {
    clearPendingCredentials();
    if (storedCredentials.valid) {
      fallbackScheduled = true;
      fallbackAt = millis() + FALLBACK_DELAY_MS;
      Serial.printf("STORED_CONFIG_FALLBACK_IN_MS %lu\n",
                    static_cast<unsigned long>(FALLBACK_DELAY_MS));
    }
  }
}

bool probeUpstream() {
  WiFiClient client;
  HTTPClient http;
  http.setConnectTimeout(UPSTREAM_TIMEOUT_MS);
  http.setTimeout(UPSTREAM_TIMEOUT_MS);

  if (!http.begin(client, UPSTREAM_URL)) {
    upstreamStatus = -1000;
    http.end();
    Serial.println("UPSTREAM_PROBE status=-1000");
    return false;
  }

  upstreamStatus = http.GET();
  http.end();
  Serial.printf("UPSTREAM_PROBE status=%d\n", upstreamStatus);
  return upstreamStatus == HTTP_CODE_OK;
}

void handleStateMachine() {
  const uint32_t now = millis();

  if (attemptStartScheduled) {
    attemptStartScheduled = false;
    if (!startStationAttempt()) {
      failAttempt(F("connect_start_failed"));
    }
    return;
  }

  if (fallbackScheduled && deadlineReached(now, fallbackAt)) {
    fallbackScheduled = false;
    queueStationAttempt(CredentialSource::Stored);
    return;
  }

  if (state == ProvisioningState::Connecting) {
    if (WiFi.status() == WL_CONNECTED &&
        WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
      const Credentials *credentials = credentialsFor(activeSource);
      state = ProvisioningState::Verifying;
      Serial.printf("STA_GOT_IP ssid=%s ip=%s rssi=%d\n",
                    credentials == nullptr ? "" : credentials->ssid.c_str(),
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
      return;
    }

    const wl_status_t wifiStatus = WiFi.status();
    if (wifiStatus == WL_CONNECT_FAILED || wifiStatus == WL_NO_SSID_AVAIL) {
      failAttempt(F("connect_error"));
      return;
    }
    if (now - connectionStartedAt >= STA_CONNECT_TIMEOUT_MS) {
      failAttempt(F("timeout"));
      return;
    }
  }

  if (state == ProvisioningState::Verifying) {
    if (WiFi.status() != WL_CONNECTED) {
      failAttempt(F("disconnected_before_probe"));
      return;
    }

    upstreamReachable = probeUpstream();
    if (!upstreamReachable) {
      failAttempt(F("upstream_unreachable"));
      return;
    }

    if (activeSource == CredentialSource::Pending) {
      if (!commitPendingCredentials()) {
        failAttempt(F("nvs_write_failed"));
        return;
      }
      storedCredentials = pendingCredentials;
      clearPendingCredentials();
      Serial.println("CREDENTIALS_SAVED namespace=wifi_cfg");
    }

    activeSource = CredentialSource::Stored;
    state = ProvisioningState::Connected;
    lastError = "";
    Serial.println("PROVISIONING_STATE connected");
    return;
  }

  if (state == ProvisioningState::Connected &&
      WiFi.status() != WL_CONNECTED) {
    state = ProvisioningState::Failed;
    lastError = F("sta_disconnected");
    upstreamReachable = false;
    upstreamStatus = 0;
    fallbackScheduled = storedCredentials.valid;
    fallbackAt = now + FALLBACK_DELAY_MS;
    Serial.println("PROVISIONING_STATE failed ap_available=true");
  }
}

void handleRoot() {
  addNoStoreHeaders();
  server.send_P(200, PSTR("text/html; charset=utf-8"), PROVISIONING_PAGE);
}

void handleStatus() {
  const bool stationConnected = WiFi.status() == WL_CONNECTED;
  const Credentials *active = credentialsFor(activeSource);
  String body;
  body.reserve(360);
  body += F("{\"state\":\"");
  body += stateName();
  body += F("\",\"ap\":{\"ssid\":\"");
  body += jsonEscape(apSsid);
  body += F("\",\"ip\":\"");
  body += WiFi.softAPIP().toString();
  body += F("\",\"clients\":");
  body += WiFi.softAPgetStationNum();
  body += F("},\"sta\":{\"ssid\":\"");
  body += active == nullptr ? "" : jsonEscape(active->ssid);
  body += F("\",\"ip\":\"");
  body += stationConnected ? WiFi.localIP().toString() : String();
  body += F("\",\"rssi\":");
  body += stationConnected ? String(WiFi.RSSI()) : F("null");
  body += F("},\"upstream\":{\"reachable\":");
  body += upstreamReachable ? F("true") : F("false");
  body += F(",\"status\":");
  body += upstreamStatus;
  body += F("}");
  if (!lastError.isEmpty()) {
    body += F(",\"error\":\"");
    body += jsonEscape(lastError);
    body += '"';
  }
  body += '}';
  sendJson(200, body);
}

bool hexValue(char character, uint8_t &value) {
  if (character >= '0' && character <= '9') {
    value = character - '0';
    return true;
  }
  if (character >= 'a' && character <= 'f') {
    value = character - 'a' + 10;
    return true;
  }
  if (character >= 'A' && character <= 'F') {
    value = character - 'A' + 10;
    return true;
  }
  return false;
}

bool decodeFormComponent(const String &encoded, String &decoded) {
  decoded = "";
  decoded.reserve(encoded.length());
  for (size_t index = 0; index < encoded.length(); ++index) {
    char character = encoded[index];
    if (character == '+') {
      decoded += ' ';
      continue;
    }
    if (character != '%') {
      decoded += character;
      continue;
    }
    if (index + 2 >= encoded.length()) {
      return false;
    }
    uint8_t high = 0;
    uint8_t low = 0;
    if (!hexValue(encoded[index + 1], high) ||
        !hexValue(encoded[index + 2], low)) {
      return false;
    }
    character = static_cast<char>((high << 4) | low);
    if (character == '\0') {
      return false;
    }
    decoded += character;
    index += 2;
  }
  return true;
}

bool parseConfigureBody(String &ssid, String &password) {
  bool foundSsid = false;
  bool foundPassword = false;
  size_t fieldStart = 0;
  while (fieldStart <= configureBody.length()) {
    int separator = configureBody.indexOf('&', fieldStart);
    const size_t fieldEnd =
        separator < 0 ? configureBody.length() : static_cast<size_t>(separator);
    const String field = configureBody.substring(fieldStart, fieldEnd);
    const int equals = field.indexOf('=');
    if (equals < 0) {
      return false;
    }

    String name;
    String value;
    if (!decodeFormComponent(field.substring(0, equals), name) ||
        !decodeFormComponent(field.substring(equals + 1), value)) {
      return false;
    }
    if (name == F("ssid") && !foundSsid) {
      ssid = value;
      foundSsid = true;
    } else if (name == F("password") && !foundPassword) {
      password = value;
      foundPassword = true;
    } else {
      return false;
    }

    if (separator < 0) {
      break;
    }
    fieldStart = fieldEnd + 1;
  }
  return foundSsid && foundPassword;
}

void handleConfigureBody() {
  HTTPRaw &raw = server.raw();
  if (raw.status == RAW_START) {
    configureBody = "";
    configureBodyComplete = false;
    configureBodyTooLarge =
        server.clientContentLength() <= 0 ||
        server.clientContentLength() >
            static_cast<int>(MAX_FORM_BODY_BYTES);
    if (!configureBodyTooLarge) {
      configureBody.reserve(server.clientContentLength());
    }
    return;
  }
  if (raw.status == RAW_WRITE) {
    if (!configureBodyTooLarge) {
      for (size_t index = 0; index < raw.currentSize; ++index) {
        configureBody += static_cast<char>(raw.buf[index]);
      }
    }
    memset(raw.buf, 0, sizeof(raw.buf));
    if (configureBody.length() > MAX_FORM_BODY_BYTES) {
      configureBodyTooLarge = true;
      clearSensitiveString(configureBody);
    }
    return;
  }
  if (raw.status == RAW_END) {
    memset(raw.buf, 0, sizeof(raw.buf));
    configureBodyComplete = !configureBodyTooLarge;
  } else if (raw.status == RAW_ABORTED) {
    memset(raw.buf, 0, sizeof(raw.buf));
    configureBodyComplete = false;
    clearSensitiveString(configureBody);
  }
}

void handleConfigure() {
  if (state == ProvisioningState::Connecting ||
      state == ProvisioningState::Verifying || fallbackScheduled) {
    clearSensitiveString(configureBody);
    configureBodyComplete = false;
    sendError(409, F("connection_attempt_in_progress"));
    return;
  }

  const int contentLength = server.clientContentLength();
  const String contentType = server.header(F("Content-Type"));
  if (contentLength <= 0 ||
      contentLength > static_cast<int>(MAX_FORM_BODY_BYTES) ||
      configureBodyTooLarge || !configureBodyComplete ||
      configureBody.length() != static_cast<size_t>(contentLength)) {
    clearSensitiveString(configureBody);
    configureBodyComplete = false;
    sendError(400, F("invalid_request_size"));
    return;
  }
  if (!contentType.startsWith(F("application/x-www-form-urlencoded"))) {
    clearSensitiveString(configureBody);
    configureBodyComplete = false;
    sendError(400, F("unsupported_content_type"));
    return;
  }
  String ssid;
  String password;
  const bool parsed = parseConfigureBody(ssid, password);
  clearSensitiveString(configureBody);
  configureBodyComplete = false;
  if (!parsed) {
    clearSensitiveString(password);
    sendError(400, F("invalid_form"));
    return;
  }
  if (ssid.isEmpty() || ssid.length() > 32 || containsControlByte(ssid)) {
    clearSensitiveString(password);
    sendError(400, F("invalid_ssid"));
    return;
  }
  if (!validPassword(password)) {
    clearSensitiveString(password);
    sendError(400, F("invalid_password"));
    return;
  }

  pendingCredentials.ssid = ssid;
  pendingCredentials.password = password;
  pendingCredentials.valid = true;
  clearSensitiveString(password);
  lastError = "";
  upstreamReachable = false;
  upstreamStatus = 0;
  Serial.printf("WIFI_CONFIG_RECEIVED ssid=%s password=<redacted>\n",
                pendingCredentials.ssid.c_str());

  // The connection is deliberately queued. No Wi-Fi wait or upstream request
  // runs in this HTTP handler, so the client receives 202 immediately.
  queueStationAttempt(CredentialSource::Pending);
  sendJson(202, F("{\"state\":\"connecting\"}"));
}

void handleReset() {
  if (!resetStoredCredentials()) {
    sendError(500, F("nvs_reset_failed"));
    return;
  }

  WiFi.disconnect(false, true);
  clearCredentials(storedCredentials);
  clearPendingCredentials();
  activeSource = CredentialSource::None;
  state = ProvisioningState::Provisioning;
  attemptStartScheduled = false;
  fallbackScheduled = false;
  upstreamReachable = false;
  upstreamStatus = 0;
  lastError = "";
  Serial.println("CREDENTIALS_RESET namespace=wifi_cfg");
  Serial.println("PROVISIONING_STATE provisioning");
  sendJson(200, F("{\"state\":\"provisioning\"}"));
}

void handleNotFound() {
  sendError(404, F("not_found"));
}

bool startAccessPoint() {
  uint8_t mac[6] = {};
  WiFi.softAPmacAddress(mac);
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%02X%02X%02X", mac[3], mac[4], mac[5]);
  apSsid = String(AP_SSID_PREFIX) + suffix;

  if (!WiFi.softAP(apSsid.c_str(), AP_PASSWORD, AP_INITIAL_CHANNEL, false,
                   AP_MAX_CLIENTS)) {
    return false;
  }

  Serial.printf("PROVISIONING_AP_STARTED ssid=%s ip=%s\n", apSsid.c_str(),
                WiFi.softAPIP().toString().c_str());
  return true;
}

void startHttpServer() {
  const char *headerKeys[] = {"Content-Type"};
  server.collectHeaders(headerKeys, 1);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/wifi/configure", HTTP_POST, handleConfigure,
            handleConfigureBody);
  server.on("/api/wifi/reset", HTTP_POST, handleReset);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP_SERVER_STARTED port=80");
}

void reportApClients() {
  const uint8_t currentClients = WiFi.softAPgetStationNum();
  if (currentClients == previousApClients) {
    return;
  }
  Serial.printf(currentClients > previousApClients
                    ? "AP_CLIENT_CONNECTED clients=%u\n"
                    : "AP_CLIENT_DISCONNECTED clients=%u\n",
                currentClients);
  previousApClients = currentClients;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  WiFi.persistent(false);

  if (!WiFi.mode(WIFI_AP_STA)) {
    Serial.println("FATAL wifi_mode_failed");
    return;
  }
  if (!startAccessPoint()) {
    Serial.println("FATAL softap_start_failed");
    return;
  }

  startHttpServer();
  if (loadStoredCredentials()) {
    queueStationAttempt(CredentialSource::Stored);
  } else {
    Serial.println("PROVISIONING_STATE provisioning");
  }
}

void loop() {
  server.handleClient();
  reportApClients();
  handleStateMachine();
  delay(2);
}
