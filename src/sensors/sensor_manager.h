#pragma once

#include "bh1750_module.h"
#include "bme280_module.h"
#include "calibration_manager.h"
#include "sensor_module.h"
#include "sensor_types.h"

#include <ArduinoJson.h>

class SensorManager {
public:
  SensorManager();

  void begin();
  void update();
  void walkMetrics(SensorMetricVisitor visitor, void *context) const;
  void walkFields(SerializableSensor::FieldVisitor visitor) const;

  SensorType parseSensorType(const char *sensor) const;
  const char *sensorTypeToString(SensorType st) const;
  SensorField parseSensorField(const char *field) const;
  const char *sensorFieldToString(SensorField field) const;

  bool setCalibrationPoint(SensorType st, const char *field, uint8_t pointIndex,
                           float reference);
  bool getCalibration(SensorType st, JsonDocument &doc) const;
  SensorSnapshot snapshot() const;

private:
  CalibrationManager calibration_;
  Bme280Module bme280_;
  Bh1750Module bh1750_;

  ISensorModule *modules_[2];
};