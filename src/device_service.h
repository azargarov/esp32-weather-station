
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <stdint.h>
#include <esp_system.h>

#include "sensors/sensor_manager.h"

struct BootInfo {
  esp_reset_reason_t resetReason;

};

class DeviceService {
public:
  struct Result {
    bool ok;
    uint16_t statusCode;
    const char *error;
  };
  DeviceService(SensorManager& sensorManager, BootInfo bootInfo);

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
  const BootInfo bootInfo_;

  bool rebootRequested_ = false;
  uint32_t rebootAtMs_ = 0;
  uint32_t metricsBuildDurationMs_ = 0;
  uint32_t metricsLastBuildUptimeSeconds_ = 0;

  String cachedMetrics_;
  String metricNameBuffer_;
  String metricLabelsBuffer_;
};
