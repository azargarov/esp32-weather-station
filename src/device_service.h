
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <stdint.h>

#include "sensors/sensor_manager.h"

class DeviceService {
public:
  struct Result {
    bool ok;
    uint16_t statusCode;
    const char *error;
  };

  explicit DeviceService(SensorManager &sensorManager);

  void updateMetricsCache();
  const String &getMetrics() const;

  String getTextStatus();
  void getJSONStatus(JsonDocument &doc);
  void getDeviceInfo(JsonDocument &doc);

  Result provisionDevice(const String &newId, const String &newHostname,
                         JsonDocument &response);
  Result setHostname(const String &newHostname, JsonDocument &response);

  Result requestReboot(JsonDocument &response);
  void handlePendingReboot();

private:
  SensorManager &sensorManager_;

  bool rebootRequested_ = false;
  uint32_t rebootAtMs_ = 0;

  String cachedMetrics_;
  String metricNameBuffer_;
  String metricLabelsBuffer_;
};