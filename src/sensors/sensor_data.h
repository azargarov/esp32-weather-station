#pragma once

#include "sensor_types.h"
#include <stdint.h>

struct SensorSample {
  SensorType sensor{SensorType::Unknown};
  SensorField field{SensorField::Unknown};
  bool valid{false};
  float value{0.0f};
  uint32_t timestampMs{0};

  constexpr SensorSample() = default;

  constexpr SensorSample(SensorType sensorType, SensorField sensorField,
                         bool isValid, float sampleValue, uint32_t ts)
      : sensor(sensorType), field(sensorField), valid(isValid),
        value(sampleValue), timestampMs(ts) {}
};

struct SensorStatus {
  bool available = false;
  bool readOk = false;
};

struct SensorSnapshot {
  SensorStatus bme280;
  SensorStatus bh1750;
};