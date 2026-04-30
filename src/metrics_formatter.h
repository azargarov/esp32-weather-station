#pragma once

#include <Arduino.h>

#include "device_state.h"
#include "sensors/sensor_manager.h"

String formatPrometheusMetrics(const DeviceState& state,
                               const SensorManager& sensorManager);

void initMetricsFormatter();