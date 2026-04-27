#include "sensor_manager.h"
#include "../config.h"

void SensorManager::begin() {
  if (Config::BME280_ENABLED) {
    probeBME280();
  } else {
    bme280Present_ = false;
  }

  if (bme280Present_) {
    readSensors();
  }
}

void SensorManager::update() {
  const unsigned long now = millis();

  if (bme280Present_) {
    if (now - lastReadMs_ >= Config::SENSOR_READ_INTERVAL_MS) {
      lastReadMs_ = now;
      readSensors();
    }
    return;
  }

  // if (now - lastProbeMs_ >= Config::SENSOR_PROBE_INTERVAL_MS) {
  //   lastProbeMs_ = now;
  //   probeBME280();
  //   if (bme280Present_) {
  //     readSensors();
  //     lastReadMs_ = now;
  //   }
  // }
}

SensorSnapshot SensorManager::snapshot() const {
  SensorSnapshot snapshot;
  snapshot.bme280Available = bme280_.available();
  snapshot.bme280ReadOk = bme280_.lastReadOk();
  return snapshot;
}

void SensorManager::walkFields(SerializableSensor::FieldVisitor visitor) const {
  bme280_.walkFields(visitor);
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
    Serial.println("[sensors] BME280 read failed");
  }
}
