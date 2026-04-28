#pragma once

#include "data_serialization.h"
#include "sensor_data.h"
#include "aggregated_metric.h"

#include <Adafruit_BME280.h>
#include <Arduino.h>
#include <Wire.h>

struct Bme280Reading {
  bool valid = false;
  float temperatureC = NAN;
  float humidityPercent = NAN;
  float pressureHpa = NAN;
  uint32_t timestampMs = 0;
};

struct Bme280Metrics {
  MetricStats temperature;
  MetricStats humidity;
  MetricStats pressure;

  void add(const Bme280Reading& reading) {
    if (!reading.valid) {
      return;
    }

    temperature.add(reading.temperatureC);
    humidity.add(reading.humidityPercent);
    pressure.add(reading.pressureHpa);
  }

  bool hasValue() const {
    return temperature.hasValue() || humidity.hasValue() || pressure.hasValue();
  }

  uint32_t sampleCount() const {
    return temperature.count();
  }

  void reset() {
    temperature.reset();
    humidity.reset();
    pressure.reset();
  }
};

class Bme280Sensor : public SerializableSensor {
public:
  bool begin(uint8_t i2cAddress);
  bool read();
  bool available() const;
  bool lastReadOk() const;

  Bme280Reading lastReading() const;

  void walkFields(FieldVisitor visitor) const override;

private:
  Adafruit_BME280 bme_;
  bool available_ = false;
  bool lastReadOk_ = false;

  float temperatureC_ = NAN;
  float humidityPct_ = NAN;
  float pressureHpa_ = NAN;
  uint32_t lastReadMs_ = 0;
};
