#pragma once

#include "sensor_data.h"

#include <Adafruit_BME280.h>
#include <Arduino.h>

struct Bme280Reading {
  bool valid = false;

  float temperatureC = NAN;
  float humidityPercent = NAN;
  float pressureHpa = NAN;

  uint32_t timestampMs = 0;
};

class Bme280Sensor {
public:
  bool begin(uint8_t i2cAddress);
  bool read();

  bool available() const;
  bool lastReadOk() const;

  Bme280Reading lastReading() const;
  SensorSample temperatureSample() const;
  SensorSample humiditySample() const;
  SensorSample pressureSample() const;

private:
  Adafruit_BME280 driver_;

  bool available_ = false;
  bool lastReadOk_ = false;
  uint32_t lastReadMs_ = 0;

  float temperatureRawC_ = NAN;
  float humidityRawPercent_ = NAN;
  float pressureRawHpa_ = NAN;
};
