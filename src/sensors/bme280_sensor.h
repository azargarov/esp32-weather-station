#pragma once

#include "data_serialization.h"
#include "aggregated_metric.h"

#include <Adafruit_BME280.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "sensor.h"


enum class Bme280Field {
  Temperature,
  Humidity,
  Pressure,
  Unknown
};

struct Bme280Reading {
  bool valid = false;

  float temperatureC = NAN;
  float temperatureRawC = NAN;
  float temperatureOffsetC = 0.0f;

  float humidityPercent = NAN;
  float humidityRawPercent = NAN;
  float humidityOffsetPercent = 0.0f;

  float pressureHpa = NAN;
  float pressureRawHpa = NAN;
  float pressureOffsetHpa = 0.0f;

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

  bool loadCalibration();
  bool saveCalibration();
  void clearCalibration();

  void setTemperatureOffset(float offset);
  void setHumidityOffset(float offset);
  void setPressureOffset(float offset);
  Bme280Field parseBme280Field(const char* field);
  bool setCalibrationFromReference(Bme280Field f, float reference);
  bool getCalibration(JsonDocument & doc) const;

private:
  Adafruit_BME280 driver_;

  CalibratedField temperature_{"temperature", "celsius"};
  CalibratedField pressure_{"pressure", "hpa"};
  CalibratedField humidity_{"humidity", "percent"};

  bool available_ = false;
  bool lastReadOk_ = false;
  uint32_t lastReadMs_ = 0;
};

