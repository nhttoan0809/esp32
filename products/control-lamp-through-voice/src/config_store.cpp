#include "config_store.h"

#include <Preferences.h>

#include "app_config.h"

using namespace app_config;

bool ConfigStore::load(DeviceConfig &config) const {
  Preferences preferences;
  if (!preferences.begin(NVS_NAMESPACE, true)) {
    return false;
  }

  const bool valid = preferences.getBool(NVS_VALID_KEY, false);
  DeviceConfig loaded;
  if (valid) {
    loaded.wifiSsid = preferences.getString(NVS_SSID_KEY, "");
    loaded.wifiPassword = preferences.getString(NVS_PASSWORD_KEY, "");
    loaded.serverHost = preferences.getString(NVS_SERVER_HOST_KEY, "");
    loaded.serverPort = preferences.getUShort(NVS_SERVER_PORT_KEY, HTTPS_PORT);
    loaded.serverPath =
        preferences.getString(NVS_SERVER_PATH_KEY, DEFAULT_SERVER_PATH);
    loaded.valid = true;
  }
  preferences.end();

  if (!valid || loaded.wifiSsid.isEmpty() || loaded.serverHost.isEmpty() ||
      loaded.serverPort != HTTPS_PORT || loaded.serverPath.isEmpty()) {
    clearDeviceConfig(loaded);
    return false;
  }

  config = loaded;
  clearSensitiveString(loaded.wifiPassword);
  return true;
}

bool ConfigStore::save(const DeviceConfig &config) const {
  if (!config.valid) {
    return false;
  }

  Preferences preferences;
  if (!preferences.begin(NVS_NAMESPACE, false)) {
    return false;
  }

  const bool markerInvalidated =
      preferences.putBool(NVS_VALID_KEY, false) == 1;
  const bool ssidWritten =
      preferences.putString(NVS_SSID_KEY, config.wifiSsid) ==
      config.wifiSsid.length();
  const size_t passwordBytes =
      preferences.putString(NVS_PASSWORD_KEY, config.wifiPassword);
  const bool passwordWritten = config.wifiPassword.isEmpty() ||
                               passwordBytes == config.wifiPassword.length();
  const bool hostWritten =
      preferences.putString(NVS_SERVER_HOST_KEY, config.serverHost) ==
      config.serverHost.length();
  const bool portWritten =
      preferences.putUShort(NVS_SERVER_PORT_KEY, config.serverPort) ==
      sizeof(uint16_t);
  const bool pathWritten =
      preferences.putString(NVS_SERVER_PATH_KEY, config.serverPath) ==
      config.serverPath.length();

  const bool readBack =
      preferences.getString(NVS_SSID_KEY, "") == config.wifiSsid &&
      preferences.getString(NVS_PASSWORD_KEY, "") == config.wifiPassword &&
      preferences.getString(NVS_SERVER_HOST_KEY, "") == config.serverHost &&
      preferences.getUShort(NVS_SERVER_PORT_KEY, 0) == config.serverPort &&
      preferences.getString(NVS_SERVER_PATH_KEY, "") == config.serverPath;

  const bool valuesWritten = markerInvalidated && ssidWritten &&
                             passwordWritten && hostWritten && portWritten &&
                             pathWritten && readBack;
  const bool committed =
      valuesWritten && preferences.putBool(NVS_VALID_KEY, true) == 1;
  preferences.end();
  return committed;
}

bool ConfigStore::clear() const {
  Preferences preferences;
  if (!preferences.begin(NVS_NAMESPACE, false)) {
    return false;
  }
  const bool markerCleared =
      preferences.putBool(NVS_VALID_KEY, false) == 1;
  preferences.remove(NVS_SSID_KEY);
  preferences.remove(NVS_PASSWORD_KEY);
  preferences.remove(NVS_SERVER_HOST_KEY);
  preferences.remove(NVS_SERVER_PORT_KEY);
  preferences.remove(NVS_SERVER_PATH_KEY);
  preferences.end();
  return markerCleared;
}
