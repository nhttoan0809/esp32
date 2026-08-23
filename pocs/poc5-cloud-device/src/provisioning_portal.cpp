#include "provisioning_portal.h"

#include <WiFi.h>

#include "app_config.h"
#include "provisioning_page.h"

using namespace app_config;

namespace {

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

bool validServerHost(const String &host) {
  if (host.length() < 3 || host.length() > 253 ||
      host.indexOf('.') < 1 || host.indexOf("..") >= 0) {
    return false;
  }
  bool labelStart = true;
  bool hasLetter = false;
  size_t labelLength = 0;
  for (size_t index = 0; index < host.length(); ++index) {
    const char character = host[index];
    if (character == '.') {
      if (labelStart || host[index - 1] == '-' || labelLength > 63) {
        return false;
      }
      labelStart = true;
      labelLength = 0;
      continue;
    }
    if (!(isalnum(static_cast<unsigned char>(character)) || character == '-')) {
      return false;
    }
    if (isalpha(static_cast<unsigned char>(character))) {
      hasLetter = true;
    }
    if (labelStart && character == '-') {
      return false;
    }
    labelStart = false;
    ++labelLength;
  }
  return !labelStart && labelLength <= 63 && host[host.length() - 1] != '-' &&
         hasLetter;
}

bool validServerPath(const String &path) {
  if (path.isEmpty() || path.length() > 96 || path[0] != '/' ||
      path.indexOf('?') >= 0 || path.indexOf('#') >= 0 ||
      containsControlByte(path)) {
    return false;
  }
  for (size_t index = 0; index < path.length(); ++index) {
    const uint8_t character = static_cast<uint8_t>(path[index]);
    if (character > 0x7e || character == ' ') {
      return false;
    }
  }
  return true;
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

}  // namespace

bool ProvisioningPortal::begin(const char *deviceId) {
  if (active_) {
    return true;
  }
  deviceId_ = deviceId;
  uint8_t mac[6] = {};
  WiFi.softAPmacAddress(mac);
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%02X%02X%02X", mac[3], mac[4], mac[5]);
  apSsid_ = String(AP_SSID_PREFIX) + suffix;

  if (!WiFi.softAP(apSsid_.c_str(), AP_PASSWORD, AP_INITIAL_CHANNEL, false,
                   AP_MAX_CLIENTS)) {
    return false;
  }
  if (!routesRegistered_) {
    registerRoutes();
    routesRegistered_ = true;
  }
  server_.begin();
  active_ = true;
  Serial.printf("PROVISIONING_AP_STARTED ssid=%s ip=%s\r\n",
                apSsid_.c_str(), WiFi.softAPIP().toString().c_str());
  Serial.println("HTTP_SERVER_STARTED port=80");
  return true;
}

void ProvisioningPortal::stop() {
  if (!active_) {
    return;
  }
  server_.stop();
  WiFi.softAPdisconnect(true);
  active_ = false;
  Serial.println("PROVISIONING_AP_STOPPED");
}

void ProvisioningPortal::loop() {
  if (active_) {
    server_.handleClient();
  }
}

bool ProvisioningPortal::active() const {
  return active_;
}

uint8_t ProvisioningPortal::clientCount() const {
  return active_ ? WiFi.softAPgetStationNum() : 0;
}

const String &ProvisioningPortal::ssid() const {
  return apSsid_;
}

void ProvisioningPortal::setRuntimeStatus(
    const PortalRuntimeStatus &status) {
  runtimeStatus_ = status;
}

bool ProvisioningPortal::takeSubmittedConfig(DeviceConfig &config) {
  if (!submitted_) {
    return false;
  }
  config = submittedConfig_;
  clearDeviceConfig(submittedConfig_);
  submitted_ = false;
  return true;
}

bool ProvisioningPortal::takeResetRequest() {
  const bool requested = resetRequested_;
  resetRequested_ = false;
  return requested;
}

void ProvisioningPortal::registerRoutes() {
  const char *headerKeys[] = {"Content-Type"};
  server_.collectHeaders(headerKeys, 1);
  server_.on("/", HTTP_GET, [this]() { handleRoot(); });
  server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/configure", HTTP_POST,
             [this]() { handleConfigure(); },
             [this]() { handleConfigureBody(); });
  server_.on("/api/reset", HTTP_POST, [this]() { handleReset(); });
  server_.onNotFound([this]() { handleNotFound(); });
}

void ProvisioningPortal::handleRoot() {
  server_.sendHeader(F("Cache-Control"), F("no-store"));
  server_.send_P(200, PSTR("text/html; charset=utf-8"), PROVISIONING_PAGE);
}

void ProvisioningPortal::handleStatus() {
  String body;
  body.reserve(420);
  body += F("{\"device_id\":\"");
  body += jsonEscape(deviceId_);
  body += F("\",\"provisioning\":\"");
  body += active_ ? (clientCount() > 0 ? F("client_connected") : F("ready"))
                  : F("disabled");
  body += F("\",\"ap\":{\"ssid\":\"");
  body += jsonEscape(apSsid_);
  body += F("\",\"clients\":");
  body += clientCount();
  body += F("},\"wifi\":{\"state\":\"");
  body += jsonEscape(runtimeStatus_.wifiState);
  body += F("\",\"ip\":\"");
  body += jsonEscape(runtimeStatus_.stationIp);
  body += F("\"},\"cloud\":{\"state\":\"");
  body += jsonEscape(runtimeStatus_.cloudState);
  body += F("\"},\"real_device\":{\"on\":");
  body += runtimeStatus_.realDeviceOn ? F("true") : F("false");
  body += '}';
  if (!runtimeStatus_.lastError.isEmpty()) {
    body += F(",\"error\":\"");
    body += jsonEscape(runtimeStatus_.lastError);
    body += '"';
  }
  body += '}';
  sendJson(200, body);
}

void ProvisioningPortal::handleConfigureBody() {
  HTTPRaw &raw = server_.raw();
  if (raw.status == RAW_START) {
    configureBody_ = "";
    bodyComplete_ = false;
    bodyTooLarge_ = server_.clientContentLength() <= 0 ||
                    server_.clientContentLength() >
                        static_cast<int>(MAX_FORM_BODY_BYTES);
    if (!bodyTooLarge_) {
      configureBody_.reserve(server_.clientContentLength());
    }
  } else if (raw.status == RAW_WRITE) {
    if (!bodyTooLarge_) {
      for (size_t index = 0; index < raw.currentSize; ++index) {
        configureBody_ += static_cast<char>(raw.buf[index]);
      }
    }
    memset(raw.buf, 0, raw.currentSize);
    if (configureBody_.length() > MAX_FORM_BODY_BYTES) {
      bodyTooLarge_ = true;
      clearSensitiveString(configureBody_);
    }
  } else if (raw.status == RAW_END) {
    bodyComplete_ = !bodyTooLarge_;
  } else if (raw.status == RAW_ABORTED) {
    bodyComplete_ = false;
    clearSensitiveString(configureBody_);
  }
}

void ProvisioningPortal::handleConfigure() {
  const int contentLength = server_.clientContentLength();
  const String contentType = server_.header(F("Content-Type"));
  if (contentLength <= 0 ||
      contentLength > static_cast<int>(MAX_FORM_BODY_BYTES) || bodyTooLarge_ ||
      !bodyComplete_ ||
      configureBody_.length() != static_cast<size_t>(contentLength)) {
    clearSensitiveString(configureBody_);
    bodyComplete_ = false;
    sendError(400, F("invalid_request_size"));
    return;
  }
  if (!contentType.startsWith(F("application/x-www-form-urlencoded"))) {
    clearSensitiveString(configureBody_);
    bodyComplete_ = false;
    sendError(400, F("unsupported_content_type"));
    return;
  }

  DeviceConfig parsed;
  const bool parsedSuccessfully = parseConfigureBody(parsed);
  clearSensitiveString(configureBody_);
  bodyComplete_ = false;
  if (!parsedSuccessfully) {
    clearDeviceConfig(parsed);
    sendError(400, F("invalid_form"));
    return;
  }
  if (parsed.wifiSsid.isEmpty() || parsed.wifiSsid.length() > 32 ||
      containsControlByte(parsed.wifiSsid)) {
    clearDeviceConfig(parsed);
    sendError(400, F("invalid_ssid"));
    return;
  }
  if (!validPassword(parsed.wifiPassword)) {
    clearDeviceConfig(parsed);
    sendError(400, F("invalid_password"));
    return;
  }
  if (!validServerHost(parsed.serverHost)) {
    clearDeviceConfig(parsed);
    sendError(400, F("invalid_server_host"));
    return;
  }
  if (parsed.serverPort != HTTPS_PORT) {
    clearDeviceConfig(parsed);
    sendError(400, F("server_port_must_be_443"));
    return;
  }
  if (!validServerPath(parsed.serverPath)) {
    clearDeviceConfig(parsed);
    sendError(400, F("invalid_server_path"));
    return;
  }

  clearDeviceConfig(submittedConfig_);
  submittedConfig_ = parsed;
  submittedConfig_.valid = true;
  clearSensitiveString(parsed.wifiPassword);
  submitted_ = true;
  Serial.printf("CONFIG_RECEIVED ssid=%s server_host=%s server_port=%u\r\n",
                submittedConfig_.wifiSsid.c_str(),
                submittedConfig_.serverHost.c_str(),
                submittedConfig_.serverPort);
  sendJson(202, F("{\"state\":\"accepted\"}"));
}

void ProvisioningPortal::handleReset() {
  resetRequested_ = true;
  sendJson(202, F("{\"state\":\"reset_pending\"}"));
}

void ProvisioningPortal::handleNotFound() {
  sendError(404, F("not_found"));
}

void ProvisioningPortal::sendJson(int statusCode, const String &body) {
  server_.sendHeader(F("Cache-Control"), F("no-store"));
  server_.sendHeader(F("Pragma"), F("no-cache"));
  server_.send(statusCode, F("application/json"), body);
}

void ProvisioningPortal::sendError(
    int statusCode, const __FlashStringHelper *error) {
  sendJson(statusCode,
           String(F("{\"error\":\"")) + String(error) + F("\"}"));
}

bool ProvisioningPortal::parseConfigureBody(DeviceConfig &config) {
  bool foundSsid = false;
  bool foundPassword = false;
  bool foundHost = false;
  bool foundPort = false;
  bool foundPath = false;
  size_t fieldStart = 0;
  while (fieldStart <= configureBody_.length()) {
    const int separator = configureBody_.indexOf('&', fieldStart);
    const size_t fieldEnd = separator < 0
                                ? configureBody_.length()
                                : static_cast<size_t>(separator);
    const String field = configureBody_.substring(fieldStart, fieldEnd);
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
      config.wifiSsid = value;
      foundSsid = true;
    } else if (name == F("password") && !foundPassword) {
      config.wifiPassword = value;
      foundPassword = true;
    } else if (name == F("server_host") && !foundHost) {
      config.serverHost = value;
      config.serverHost.toLowerCase();
      foundHost = true;
    } else if (name == F("server_port") && !foundPort) {
      if (value != F("443")) {
        return false;
      }
      config.serverPort = HTTPS_PORT;
      foundPort = true;
    } else if (name == F("server_path") && !foundPath) {
      config.serverPath = value;
      foundPath = true;
    } else {
      return false;
    }

    if (separator < 0) {
      break;
    }
    fieldStart = fieldEnd + 1;
  }
  config.valid = foundSsid && foundPassword && foundHost && foundPort &&
                 foundPath;
  return config.valid;
}
