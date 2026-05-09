#include "bme280_module.h"
#include "../config.h"
//#include "sensor_types.h"
#include "sensor_metric_utils.h"

#include <Arduino.h>
#include <math.h>

namespace {

//void appendStatMetric(SensorMetricVisitor visitor, void *context,
//                      SensorType sensor, SensorField field, const char *family,
//                      float value, const char *unit, const char *help) {
//  if (!isfinite(value)) {
//    return;
//  }
//
//  visitor({family, sensor, field, value, unit, help, SensorMetricType::Gauge},
//          context);
//}

//void walkFieldStats(SensorMetricVisitor visitor, void *context,
//                    SensorType sensor, SensorField field,
//                    const MetricStats &stats) {
//  if (!stats.hasValue()) {
//    return;
//  }
//
//  const char *unit = sensorFieldUnit(field);
//
//  appendStatMetric(visitor, context, sensor, field, "mean_60s", stats.average(),
//                   unit, "Mean value over the last 60-second rolling window.");
//  appendStatMetric(visitor, context, sensor, field, "median_60s",
//                   stats.median(), unit,
//                   "Median value over the last 60-second rolling window.");
//  appendStatMetric(visitor, context, sensor, field, "min_60s", stats.min(),
//                   unit, "Minimum value over the last 60-second rolling window.");
//  appendStatMetric(visitor, context, sensor, field, "max_60s", stats.max(),
//                   unit, "Maximum value over the last 60-second rolling window.");
//  appendStatMetric(visitor, context, sensor, field, "range_60s", stats.range(),
//                   unit, "Range over the last 60-second rolling window.");
//  appendStatMetric(visitor, context, sensor, field, "stddev_60s", stats.stddev(),
//                   unit,
//                   "Standard deviation over the last 60-second rolling window.");
//  appendStatMetric(visitor, context, sensor, field, "slope_per_minute_60s",
//                   stats.slopePerMinute(), unit,
//                   "Linear regression slope over the rolling window, expressed per minute.");
//
//  visitor({"samples_60s", sensor, field, static_cast<float>(stats.count()),
//           nullptr, "Number of valid samples in the rolling statistics window.",
//           SensorMetricType::Gauge},
//          context);
//}

//void walkCalibrationMetric(SensorMetricVisitor visitor, void *context,
//                           SensorType sensor, SensorField field,
//                           const char *offsetUnit,
//                           const CalibrationProfile &profile) {
//  visitor({"calibration_enabled", sensor, field, profile.enabled ? 1.0f : 0.0f,
//           nullptr, "Whether calibration is enabled for this sensor field.",
//           SensorMetricType::Gauge},
//          context);
//
//  visitor({"calibration_scale", sensor, field, profile.scale, nullptr,
//           "Linear calibration scale coefficient.", SensorMetricType::Gauge},
//          context);
//
//  visitor({"calibration_offset", sensor, field, profile.offset, offsetUnit,
//           "Linear calibration offset coefficient.", SensorMetricType::Gauge},
//          context);
//
//  visitor({"calibration_mode", sensor, field,
//           static_cast<float>(static_cast<uint8_t>(profile.mode)), nullptr,
//           "Calibration mode as numeric enum: 0 none, 1 offset, 2 two-point.",
//           SensorMetricType::Gauge},
//          context);
//}

} // namespace

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

bool Bme280Module::setCalibration(SensorField field, float reference) {
  const float raw = latestRawValue(field);

  if (!isfinite(raw)) {
    return false;
  }

  return calibration_.setOffsetCalibration({SensorType::Bme280, field}, raw,
                                           reference);
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

  walkFieldStats(visitor, context, SensorType::Bme280,
                 SensorField::Temperature, metrics_.temperature);
  walkFieldStats(visitor, context, SensorType::Bme280,
                 SensorField::Humidity, metrics_.humidity);
  walkFieldStats(visitor, context, SensorType::Bme280,
                 SensorField::Pressure, metrics_.pressure);

  walkCalibrationMetric(
      visitor, context, SensorType::Bme280, SensorField::Temperature,
      "celsius",
      calibration_.getProfile({SensorType::Bme280, SensorField::Temperature}));

  walkCalibrationMetric(
      visitor, context, SensorType::Bme280, SensorField::Humidity,
      "percent",
      calibration_.getProfile({SensorType::Bme280, SensorField::Humidity}));

  walkCalibrationMetric(
      visitor, context, SensorType::Bme280, SensorField::Pressure,
      "hpa",
      calibration_.getProfile({SensorType::Bme280, SensorField::Pressure}));
}