#include "sensor_manager.h"

#include <Arduino.h>

SensorManager::SensorManager()
    : calibration_(), bme280_(calibration_), bh1750_(calibration_),
      modules_{&bme280_, &bh1750_} {}

void SensorManager::begin() {
  calibration_.begin();

  for (auto *module : modules_) {
    module->begin();
  }
}

void SensorManager::update() {
  const uint32_t now = millis();

  for (auto *module : modules_) {
    module->update(now);
  }
}

void SensorManager::walkMetrics(SensorMetricVisitor visitor,
                                void *context) const {
  if (!visitor) {
    return;
  }

  for (auto *module : modules_) {
    module->walkMetrics(visitor, context);
  }
}

void SensorManager::walkFields(SerializableSensor::FieldVisitor visitor) const {
  if (!visitor) {
    return;
  }

  for (auto *module : modules_) {
    module->walkFields(visitor);
  }
}

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

bool SensorManager::setCalibrationPoint(SensorType st, const char *field,
                                        uint8_t pointIndex, float reference) {
  const SensorField sensorField = parseSensorField(field);

  if (sensorField == SensorField::Unknown) {
    return false;
  }

  for (auto *module : modules_) {
    if (module->type() == st) {
      return module->setCalibrationPoint(sensorField, pointIndex, reference);
    }
  }

  return false;
}

bool SensorManager::getCalibration(SensorType st, JsonDocument &doc) const {
  for (auto *module : modules_) {
    if (module->type() == st) {
      return module->getCalibration(doc);
    }
  }

  return false;
}

SensorSnapshot SensorManager::snapshot() const {
  SensorSnapshot s{};

  s.bme280.available = bme280_.available();
  s.bme280.readOk = bme280_.lastReadOk();

  s.bh1750.available = bh1750_.available();
  s.bh1750.readOk = bh1750_.lastReadOk();

  return s;
}
