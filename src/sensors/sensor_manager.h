#pragma once

#include "bme280_sensor.h"
#include "data_serialization.h"
#include "sensor_data.h"
#include <Arduino.h>

class SensorManager {
public:
  void begin();
  void update();

  SensorSnapshot snapshot() const;
  void walkFields(SerializableSensor::FieldVisitor visitor) const;

private:
  Bme280Sensor bme280_;
  unsigned long lastReadMs_ = 0;
  unsigned long lastProbeMs_ = 0;

  void readSensors();
  void probeBME280();

  bool bme280Present_ = false;
};
