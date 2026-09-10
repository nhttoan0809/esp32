#pragma once

#include "device_config.h"

class ConfigStore {
 public:
  bool load(DeviceConfig &config) const;
  bool save(const DeviceConfig &config) const;
  bool clear() const;
};
