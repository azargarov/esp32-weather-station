#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include "sensor_data.h"
#include "data_serialization.h"

class Bme280Sensor : public SerializableSensor {
public:
  bool begin(uint8_t i2cAddress);
  bool read();
  bool available() const;
  bool lastReadOk() const;

  void walkFields(FieldVisitor visitor) const override;

private:
  Adafruit_BME280 bme_;
  bool available_ = false;
  bool lastReadOk_ = false;

  float temperatureC_ = 0.0f;
  float humidityPct_ = 0.0f;
  float pressureHpa_ = 0.0f;
};
