#pragma once

#include "aggregated_metric.h"
#include "sensor_data.h"
#include "sensor_module.h"
#include <Arduino.h>
#include <BH1750.h>

struct Bh1750Reading {
  bool valid = false;

  float lux = NAN;

  uint32_t timestampMs = 0;
};

struct Bh1750Metrics {
  MetricStats illuminance;

  bool hasValue() const { return illuminance.hasValue(); }
};

class Bh1750Sensor {
public:
  bool begin(uint8_t i2cAddress);
  bool read();

  bool available() const;
  bool lastReadOk() const;

  Bh1750Reading lastReading() const;
  SensorSample luxSample() const;

private:
  BH1750 driver_;

  bool available_ = false;
  bool lastReadOk_ = false;
  uint32_t lastReadMs_ = 0;

  float luxRaw_ = NAN;
};
