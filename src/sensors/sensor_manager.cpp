#include "sensor_manager.h"
#include "../config.h"

#include <math.h>
#include <stdio.h>

namespace {

void appendStatMetric(SensorMetricVisitor visitor, void *context,
                      const char *field, const char *stat, float value,
                      const char *unit, const char *help) {
  if (!isfinite(value)) {
    return;
  }

  char name[64];
  snprintf(name, sizeof(name), "%s_%s_60s", field, stat);
  visitor({name, value, unit, help}, context);
}

void walkFieldStats(SensorMetricVisitor visitor, void *context,
                    const char *field, const char *unit,
                    const MetricStats &stats) {
  if (!stats.hasValue()) {
    return;
  }

  appendStatMetric(visitor, context, field, "mean", stats.average(), unit,
                   "Mean value over the last 60-second rolling window.");
  appendStatMetric(visitor, context, field, "median", stats.median(), unit,
                   "Median value over the last 60-second rolling window.");
  appendStatMetric(visitor, context, field, "min", stats.min(), unit,
                   "Minimum value over the last 60-second rolling window.");
  appendStatMetric(visitor, context, field, "max", stats.max(), unit,
                   "Maximum value over the last 60-second rolling window.");
  appendStatMetric(visitor, context, field, "range", stats.range(), unit,
                   "Range over the last 60-second rolling window.");
  appendStatMetric(visitor, context, field, "stddev", stats.stddev(), unit,
                   "Standard deviation over the last 60-second rolling window.");
  appendStatMetric(visitor, context, field, "slope_per_minute",
                   stats.slopePerMinute(), unit,
                   "Linear regression slope over the rolling window, expressed per minute.");

  char samplesName[64];
  snprintf(samplesName, sizeof(samplesName), "%s_samples_60s", field);
  visitor({samplesName, static_cast<float>(stats.count()), "count",
           "Number of valid samples in the rolling statistics window."},
          context);
}

void walkCalibrationMetric(SensorMetricVisitor visitor, void *context,
                           SensorType sensor, SensorField field,
                           const char *offsetUnit,
                           const CalibrationProfile &profile) {
  visitor({"calibration_enabled", sensor, field,
           profile.enabled ? 1.0f : 0.0f,
           nullptr,
           "Whether calibration is enabled for this sensor field.",
           SensorMetricType::Gauge},
          context);

  visitor({"calibration_scale", sensor, field,
           profile.scale,
           nullptr,
           "Linear calibration scale coefficient.",
           SensorMetricType::Gauge},
          context);

  visitor({"calibration_offset", sensor, field,
           profile.offset,
           offsetUnit,
           "Linear calibration offset coefficient.",
           SensorMetricType::Gauge},
          context);

  visitor({"calibration_mode", sensor, field,
           static_cast<float>(static_cast<uint8_t>(profile.mode)),
           nullptr,
           "Calibration mode as numeric enum: 0 none, 1 offset, 2 two-point.",
           SensorMetricType::Gauge},
          context);
}

} // namespace

SensorType SensorManager::parseSensorType(const char *sensor) const {
  return ::parseSensorType(sensor);
}

const char *SensorManager::sensorTypeToString(SensorType st) const {
  return ::sensorTypeToString(st);
}

SensorField SensorManager::parseSensorField(const char *field) const {
  return ::parseSensorField(field);
}

const char *SensorManager::sensorFieldToString(SensorField field) const {
  return ::sensorFieldToString(field);
}

void SensorManager::begin() {
  calibration_.begin();
  lastProbeMs_ = millis();

  if (Config::BME280_ENABLED) {
    probeBME280();
  } else {
    bme280Present_ = false;
  }

  if (bme280Present_) {
    readSensors();
    lastReadMs_ = millis();
  }
}

void SensorManager::update() {
  const unsigned long now = millis();

  if (bme280Present_ && now - lastReadMs_ >= Config::SENSOR_READ_INTERVAL_MS) {
    lastReadMs_ = now;
    readSensors();
  }

  if (!bme280Present_ &&
      now - lastProbeMs_ >= Config::SENSOR_PROBE_INTERVAL_MS) {
    lastProbeMs_ = now;
    probeBME280();
  }
}

SensorSnapshot SensorManager::snapshot() const {
  SensorSnapshot snapshot;
  snapshot.bme280Available = bme280_.available();
  snapshot.bme280ReadOk = bme280_.lastReadOk();
  return snapshot;
}

void SensorManager::walkFields(SerializableSensor::FieldVisitor visitor) const {
  if (!visitor) {
    return;
  }

  if (bme280Metrics_.temperature.hasValue()) {
    visitor("temperature_mean_60s", bme280Metrics_.temperature.average(),
            "celsius");
    visitor("temperature_median_60s", bme280Metrics_.temperature.median(),
            "celsius");
  }

  if (bme280Metrics_.humidity.hasValue()) {
    visitor("humidity_mean_60s", bme280Metrics_.humidity.average(),
            "percent");
    visitor("humidity_median_60s", bme280Metrics_.humidity.median(),
            "percent");
  }

  if (bme280Metrics_.pressure.hasValue()) {
    visitor("pressure_mean_60s", bme280Metrics_.pressure.average(), "hpa");
    visitor("pressure_median_60s", bme280Metrics_.pressure.median(), "hpa");
  }
}

void SensorManager::probeBME280() {
  bme280Present_ = bme280_.begin(Config::BME280_I2C_ADDRESS);

  if (bme280Present_) {
    Serial.println("[sensors] BME280 detected");
  } else {
    Serial.println("[sensors] BME280 not detected");
  }
}

void SensorManager::readSensors() {
  if (!bme280Present_) {
    return;
  }

  if (!bme280_.read()) {
    bme280ReadErrorsTotal_++;
    Serial.println("[sensors] BME280 read failed");
    return;
  }

  addSample(bme280_.temperatureSample());
  addSample(bme280_.humiditySample());
  addSample(bme280_.pressureSample());
}

void SensorManager::addSample(const SensorSample &sample) {
  if (!sample.valid || !isfinite(sample.value)) {
    return;
  }

  const float calibrated =
      calibration_.apply({sample.sensor, sample.field}, sample.value);

  if (sample.sensor == SensorType::Bme280) {
    switch (sample.field) {
    case SensorField::Temperature:
      bme280Metrics_.temperature.add(calibrated);
      break;
    case SensorField::Humidity:
      bme280Metrics_.humidity.add(calibrated);
      break;
    case SensorField::Pressure:
      bme280Metrics_.pressure.add(calibrated);
      break;
    default:
      break;
    }
  }
}

float SensorManager::latestRawValue(SensorType sensor, SensorField field) const {
  if (sensor == SensorType::Bme280) {
    const Bme280Reading reading = bme280_.lastReading();
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

  return NAN;
}

bool SensorManager::setCalibration(SensorType st, const char *field,
                                   float reference) {
  const SensorField sensorField = parseSensorField(field);
  const float raw = latestRawValue(st, sensorField);

  if (!isfinite(raw)) {
    return false;
  }

  return calibration_.setOffsetCalibration({st, sensorField}, raw, reference);
}

bool SensorManager::getCalibration(SensorType st, JsonDocument &doc) const {
  return calibration_.writeSensorJson(st, doc);
}

void SensorManager::walkMetrics(SensorMetricVisitor visitor,
                                void *context) const {
  if (visitor == nullptr) {
    return;
  }

  visitor({"bme280_available", bme280_.available() ? 1.0f : 0.0f, "bool",
           "Whether the BME280 sensor is available.",
           SensorMetricType::Gauge},
          context);

  visitor({"bme280_read_ok", bme280_.lastReadOk() ? 1.0f : 0.0f, "bool",
           "Whether the last BME280 read succeeded.",
           SensorMetricType::Gauge},
          context);

  visitor({"bme280_read_errors_total",
           static_cast<float>(bme280ReadErrorsTotal_), "count",
           "Total number of BME280 read errors.",
           SensorMetricType::Counter},
          context);

  walkFieldStats(visitor, context, "temperature", "celsius",
                 bme280Metrics_.temperature);
  walkFieldStats(visitor, context, "humidity", "percent",
                 bme280Metrics_.humidity);
  walkFieldStats(visitor, context, "pressure", "hpa",
                 bme280Metrics_.pressure);

  walkCalibrationMetric(visitor, context,
                      SensorType::Bme280, SensorField::Temperature,
                      "celsius",
                      calibration_.getProfile({SensorType::Bme280, SensorField::Temperature}));

  walkCalibrationMetric(visitor, context,
                      SensorType::Bme280, SensorField::Humidity,
                      "percent",
                      calibration_.getProfile({SensorType::Bme280, SensorField::Humidity}));

   walkCalibrationMetric(visitor, context,
                      SensorType::Bme280, SensorField::Pressure,
                      "hpa",
                      calibration_.getProfile({SensorType::Bme280, SensorField::Pressure}));
  //walkCalibrationMetric(
  //    visitor, context, "temperature", "celsius",
  //    calibration_.getProfile({SensorType::Bme280, SensorField::Temperature}));
  //walkCalibrationMetric(
  //    visitor, context, "humidity", "percent",
  //    calibration_.getProfile({SensorType::Bme280, SensorField::Humidity}));
  //walkCalibrationMetric(
  //    visitor, context, "pressure", "hpa",
  //    calibration_.getProfile({SensorType::Bme280, SensorField::Pressure}));
}
