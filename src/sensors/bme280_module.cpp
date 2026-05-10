#include "bme280_module.h"
#include "../config.h"
// #include "sensor_types.h"
#include "sensor_metric_utils.h"

#include <Arduino.h>
#include <math.h>

Bme280Module::Bme280Module(CalibrationManager &calibration)
    : calibration_(calibration) {}

SensorType Bme280Module::type() const { return SensorType::Bme280; }

bool Bme280Module::available() const { return sensor_.available(); }

bool Bme280Module::lastReadOk() const { return sensor_.lastReadOk(); }

void Bme280Module::begin() {
  lastProbeMs_ = millis();

  if (Config::BME280_ENABLED) {
    probe();
  } else {
    present_ = false;
  }

  if (present_) {
    readSensor();
    lastReadMs_ = millis();
  }
}

void Bme280Module::update(uint32_t nowMs) {
  if (present_ && nowMs - lastReadMs_ >= Config::SENSOR_READ_INTERVAL_MS) {
    lastReadMs_ = nowMs;
    readSensor();
  }

  if (!present_ && nowMs - lastProbeMs_ >= Config::SENSOR_PROBE_INTERVAL_MS) {
    lastProbeMs_ = nowMs;
    probe();
  }
}

void Bme280Module::probe() {
  present_ = sensor_.begin(Config::BME280_I2C_ADDRESS);

  if (present_) {
    Serial.println("[sensors] BME280 detected");
  } else {
    Serial.println("[sensors] BME280 not detected");
  }
}

void Bme280Module::readSensor() {
  if (!present_) {
    return;
  }

  if (!sensor_.read()) {
    readErrorsTotal_++;
    Serial.println("[sensors] BME280 read failed");
    return;
  }

  addSample(sensor_.temperatureSample());
  addSample(sensor_.humiditySample());
  addSample(sensor_.pressureSample());
}

void Bme280Module::addSample(const SensorSample &sample) {
  if (!sample.valid || !isfinite(sample.value)) {
    return;
  }

  const float calibrated =
      calibration_.apply({sample.sensor, sample.field}, sample.value);

  switch (sample.field) {
  case SensorField::Temperature:
    metrics_.temperature.add(calibrated);
    break;
  case SensorField::Humidity:
    metrics_.humidity.add(calibrated);
    break;
  case SensorField::Pressure:
    metrics_.pressure.add(calibrated);
    break;
  default:
    break;
  }
}

float Bme280Module::latestRawValue(SensorField field) const {
  const Bme280Reading reading = sensor_.lastReading();

  if (!reading.valid) {
    return NAN;
  }

  switch (field) {
  case SensorField::Temperature:
    return reading.temperatureC;
  case SensorField::Humidity:
    return reading.humidityPercent;
  case SensorField::Pressure:
    return reading.pressureHpa;
  default:
    return NAN;
  }
}

bool Bme280Module::setCalibrationPoint(SensorField field, uint8_t pointIndex,
                                       float reference) {
  const float raw = latestRawValue(field);

  if (!isfinite(raw)) {
    return false;
  }

  return calibration_.updateCalibrationPoint({SensorType::Bme280, field},
                                             pointIndex, raw, reference);
}

bool Bme280Module::getCalibration(JsonDocument &doc) const {
  return calibration_.writeSensorJson(SensorType::Bme280, doc);
}

void Bme280Module::walkFields(SerializableSensor::FieldVisitor visitor) const {
  if (!visitor) {
    return;
  }

  if (metrics_.temperature.hasValue()) {
    visitor("temperature_mean_60s", metrics_.temperature.average(), "celsius");
    visitor("temperature_median_60s", metrics_.temperature.median(), "celsius");
  }

  if (metrics_.humidity.hasValue()) {
    visitor("humidity_mean_60s", metrics_.humidity.average(), "percent");
    visitor("humidity_median_60s", metrics_.humidity.median(), "percent");
  }

  if (metrics_.pressure.hasValue()) {
    visitor("pressure_mean_60s", metrics_.pressure.average(), "hpa");
    visitor("pressure_median_60s", metrics_.pressure.median(), "hpa");
  }
}

void Bme280Module::walkMetrics(SensorMetricVisitor visitor,
                               void *context) const {
  if (!visitor) {
    return;
  }

  visitor({"bme280_available", sensor_.available() ? 1.0f : 0.0f, "bool",
           "Whether the BME280 sensor is available.", SensorMetricType::Gauge},
          context);

  visitor({"bme280_read_ok", sensor_.lastReadOk() ? 1.0f : 0.0f, "bool",
           "Whether the last BME280 read succeeded.", SensorMetricType::Gauge},
          context);

  visitor({"bme280_read_errors_total", static_cast<float>(readErrorsTotal_),
           "count", "Total number of BME280 read errors.",
           SensorMetricType::Counter},
          context);

  walkFieldStats(visitor, context, SensorType::Bme280, SensorField::Temperature,
                 metrics_.temperature);
  walkFieldStats(visitor, context, SensorType::Bme280, SensorField::Humidity,
                 metrics_.humidity);
  walkFieldStats(visitor, context, SensorType::Bme280, SensorField::Pressure,
                 metrics_.pressure);

  walkCalibrationMetric(
      visitor, context, SensorType::Bme280, SensorField::Temperature, "celsius",
      calibration_.getProfile({SensorType::Bme280, SensorField::Temperature}));

  walkCalibrationMetric(
      visitor, context, SensorType::Bme280, SensorField::Humidity, "percent",
      calibration_.getProfile({SensorType::Bme280, SensorField::Humidity}));

  walkCalibrationMetric(
      visitor, context, SensorType::Bme280, SensorField::Pressure, "hpa",
      calibration_.getProfile({SensorType::Bme280, SensorField::Pressure}));
}