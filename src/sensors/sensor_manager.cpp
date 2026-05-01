#include "sensor_manager.h"
#include "../config.h"

namespace {
constexpr unsigned long METRIC_BUCKET_INTERVAL_MS = 60000;
}

SensorType SensorManager::parseSensorType(const char *sensor) {
  if (strcmp(sensor, "bme280") == 0) {
    return SensorType::Bme280;
  }
  return SensorType::Unknown;
}

const char *SensorManager::sensorTypeToString(SensorType st) {
  switch (st) {
  case SensorType::Bme280:
    return "bme280";
  default:
    return "unknown";
  }
}

void SensorManager::begin() {
  lastMetricRotationMs_ = millis();

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

  if (bme280Present_ &&
      now - lastMetricRotationMs_ >= METRIC_BUCKET_INTERVAL_MS) {
    lastMetricRotationMs_ = now;
    rotateMetricBuckets();
  }
}

SensorSnapshot SensorManager::snapshot() const {
  SensorSnapshot snapshot;
  snapshot.bme280Available = bme280_.available();
  snapshot.bme280ReadOk = bme280_.lastReadOk();
  return snapshot;
}

void SensorManager::walkFields(SerializableSensor::FieldVisitor visitor) const {
  const Bme280Metrics &metrics =
      bme280Previous_.hasValue() ? bme280Previous_ : bme280Current_;

  if (!metrics.hasValue()) {
    return;
  }

  visitor("temperature_avg_60s", metrics.temperature.average(), "celsius");
  visitor("temperature_stddev_60s", metrics.temperature.stddev(), "celsius");

  visitor("humidity_avg_60s", metrics.humidity.average(), "percent");
  visitor("humidity_stddev_60s", metrics.humidity.stddev(), "percent");

  visitor("pressure_avg_60s", metrics.pressure.average(), "hpa");
  visitor("pressure_stddev_60s", metrics.pressure.stddev(), "hpa");

  visitor("bme280_samples_60s", static_cast<float>(metrics.sampleCount()),
          "count");
  visitor("bme280_read_errors_total",
          static_cast<float>(bme280ReadErrorsTotal_), "count");
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

  bme280Current_.add(bme280_.lastReading());
}

void SensorManager::rotateMetricBuckets() {
  bme280Previous_ = bme280Current_;
  bme280Current_.reset();
}

bool SensorManager::setCalibration(SensorType st, const char *field,
                                   float reference) {

  if (st == SensorType::Bme280) {
    return bme280_.setCalibrationFromReference(bme280_.parseBme280Field(field),
                                               reference);
  } else {
    return false;
  }

  return true;
}

bool SensorManager::getCalibration(SensorType st, JsonDocument &doc) {

  if (st == SensorType::Bme280) {
    return bme280_.getCalibration(doc);
  }

  return false;
}

void SensorManager::walkMetrics(SensorMetricVisitor visitor,
                                void *context) const {
  if (visitor == nullptr) {
    return;
  }

  visitor({"bme280_available", bme280_.available() ? 1.0f : 0.0f, "bool",
           "Whether the BME280 sensor is available.",
           SensorMetricValueType::Bool},
          context);

  visitor({"bme280_read_ok", bme280_.lastReadOk() ? 1.0f : 0.0f, "bool",
           "Whether the last BME280 read succeeded.",
           SensorMetricValueType::Bool},
          context);

  const Bme280Metrics &metrics =
      bme280Previous_.hasValue() ? bme280Previous_ : bme280Current_;

  if (!metrics.hasValue()) {
    return;
  }

  visitor({"temperature_avg_60s", metrics.temperature.average(), "celsius",
           "Average temperature over the last statistics window."},
          context);

  visitor({"temperature_stddev_60s", metrics.temperature.stddev(), "celsius",
           "Temperature standard deviation over the last statistics window."},
          context);

  visitor({"humidity_avg_60s", metrics.humidity.average(), "percent",
           "Average humidity over the last statistics window."},
          context);

  visitor({"humidity_stddev_60s", metrics.humidity.stddev(), "percent",
           "Humidity standard deviation over the last statistics window."},
          context);

  visitor({"pressure_avg_60s", metrics.pressure.average(), "hpa",
           "Average pressure over the last statistics window."},
          context);

  visitor({"pressure_stddev_60s", metrics.pressure.stddev(), "hpa",
           "Pressure standard deviation over the last statistics window."},
          context);

  visitor({"bme280_samples_60s", static_cast<float>(metrics.sampleCount()),
           "count", "Number of BME280 samples in the statistics window."},
          context);

  visitor({"bme280_read_errors_total",
           static_cast<float>(bme280ReadErrorsTotal_), "count",
           "Total number of BME280 read errors."},
          context);
}