#pragma once

#include <stdint.h>
#include "sensor_types.h"

struct SensorSample {
  SensorType sensor{SensorType::Unknown};
  SensorField field{SensorField::Unknown};
  bool valid{false};
  float value{0.0f};
  uint32_t timestampMs{0};

  constexpr SensorSample() = default;

  constexpr SensorSample(SensorType sensorType,
                         SensorField sensorField,
                         bool isValid,
                         float sampleValue,
                         uint32_t ts)
      : sensor(sensorType),
        field(sensorField),
        valid(isValid),
        value(sampleValue),
        timestampMs(ts) {}
};

struct SensorSnapshot {
  bool bme280Available = false;
  bool bme280ReadOk = false;
};