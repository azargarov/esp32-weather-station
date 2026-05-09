#pragma once

#include "device_state.h"
#include "sensors/sensor_manager.h"
#include <Arduino.h>

struct MetricHeaderRegistry {
  static constexpr size_t kMaxNames = 32;
  String names[kMaxNames];
  size_t count = 0;

  bool contains(const String &name) const {
    for (size_t i = 0; i < count; ++i) {
      if (names[i] == name) {
        return true;
      }
    }
    return false;
  }

  void add(const String &name) {
    if (count < kMaxNames) {
      names[count++] = name;
    }
  }
};

void initMetricsFormatter();

const String &getBaseLabels();

void formatDeviceMetrics(String &out, const DeviceState &state);

void formatDeviceMetrics(String &out, const DeviceState &state,
                         uint32_t metricsBuildDurationMs,
                         uint32_t metricsLastBuildUptimeSeconds);
void appendSensorMetric(String &out, String &nameBuffer, String &labelsBuffer,
                        MetricHeaderRegistry &registry,
                        const SensorMetric &metric);
