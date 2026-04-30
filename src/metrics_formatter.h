#pragma once

#include "device_state.h"
#include "sensors/sensor_manager.h"
#include <Arduino.h>

void formatPrometheusMetrics(String& out, const DeviceState &state,
                               const SensorManager &sensorManager);

void initMetricsFormatter();
const String& getBaseLabels();