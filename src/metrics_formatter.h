#pragma once

#include "device_state.h"
#include "sensors/sensor_manager.h"
#include <Arduino.h>

void initMetricsFormatter();

const String &getBaseLabels();

void formatDeviceMetrics(String &out, const DeviceState &state);

void formatDeviceMetrics(String &out, const DeviceState &state,
                         uint32_t metricsBuildDurationMs,
                         uint32_t metricsLastBuildUptimeSeconds);

void appendSensorMetric(String &out, String &nameBuffer, String &labelsBuffer,
                        const SensorMetric &metric);
