#include "bh1750_module.h"
#include "../config.h"
#include "sensor_metric.h"
#include "sensor_metric_utils.h"

#include <Arduino.h>
#include <math.h>

Bh1750Module::Bh1750Module(CalibrationManager &calibration)
    : calibration_(calibration) {}

SensorType Bh1750Module::type() const { return SensorType::Bh1750; }

bool Bh1750Module::available() const { return sensor_.available(); }

bool Bh1750Module::lastReadOk() const { return sensor_.lastReadOk(); }

void Bh1750Module::begin() {
  lastProbeMs_ = millis();

  if (Config::BH1750_ENABLED) {
    probe();
  } else {
    present_ = false;
  }

  if (present_) {
    readSensor();
    lastReadMs_ = millis();
  }
}

void Bh1750Module::update(uint32_t nowMs) {
  if (present_ && nowMs - lastReadMs_ >= Config::SENSOR_READ_INTERVAL_MS) {
    lastReadMs_ = nowMs;
    readSensor();
  }

  if (!present_ && nowMs - lastProbeMs_ >= Config::SENSOR_PROBE_INTERVAL_MS) {
    lastProbeMs_ = nowMs;
    probe();
  }
}

void Bh1750Module::probe() {
  present_ = sensor_.begin(Config::BH1750_I2C_ADDRESS);

  if (present_) {
    Serial.println("[sensors] BH1750 detected");
  } else {
    Serial.println("[sensors] BH1750 not detected");
  }
}

void Bh1750Module::readSensor() {
  if (!present_) {
    return;
  }

  if (!sensor_.read()) {
    readErrorsTotal_++;
    Serial.println("[sensors] BH1750 read failed");
    return;
  }

  addSample(sensor_.luxSample());
}

void Bh1750Module::addSample(const SensorSample &sample) {
  if (!sample.valid || !isfinite(sample.value)) {
    return;
  }

  const float calibrated =
      calibration_.apply({sample.sensor, sample.field}, sample.value);

  switch (sample.field) {
  case SensorField::Illuminance:
    metrics_.illuminance.add(calibrated);
    break;
  default:
    break;
  }
}

float Bh1750Module::latestRawValue(SensorField field) const {
  const Bh1750Reading reading = sensor_.lastReading();

  if (!reading.valid) {
    return NAN;
  }

  switch (field) {
  case SensorField::Illuminance:
    return reading.lux;
  default:
    return NAN;
  }
}

bool Bh1750Module::setCalibration(SensorField field, float reference) {
  if (field != SensorField::Illuminance) {
    return false;
  }

  const float raw = latestRawValue(field);

  if (!isfinite(raw)) {
    return false;
  }

  return calibration_.setOffsetCalibration({SensorType::Bh1750, field}, raw,
                                           reference);
}

bool Bh1750Module::getCalibration(JsonDocument &doc) const {
  return calibration_.writeSensorJson(SensorType::Bh1750, doc);
}

void Bh1750Module::walkFields(SerializableSensor::FieldVisitor visitor) const {
  if (!visitor) {
    return;
  }

  if (metrics_.illuminance.hasValue()) {
    visitor("illuminance_mean_60s", metrics_.illuminance.average(), "lux");
    visitor("illuminance_median_60s", metrics_.illuminance.median(), "lux");
  }
}

void Bh1750Module::walkMetrics(SensorMetricVisitor visitor,
                               void *context) const {
  if (!visitor) {
    return;
  }

  visitor({"bh1750_available", sensor_.available() ? 1.0f : 0.0f, "bool",
           "Whether the BH1750 sensor is available.", SensorMetricType::Gauge},
          context);

  visitor({"bh1750_read_ok", sensor_.lastReadOk() ? 1.0f : 0.0f, "bool",
           "Whether the last BH1750 read succeeded.", SensorMetricType::Gauge},
          context);

  visitor({"bh1750_read_errors_total", static_cast<float>(readErrorsTotal_),
           "count", "Total number of BH1750 read errors.",
           SensorMetricType::Counter},
          context);

  walkFieldStats(visitor, context, SensorType::Bh1750,
                 SensorField::Illuminance, metrics_.illuminance);

  walkCalibrationMetric(
      visitor, context, SensorType::Bh1750, SensorField::Illuminance, "lux",
      calibration_.getProfile({SensorType::Bh1750, SensorField::Illuminance}));
}
