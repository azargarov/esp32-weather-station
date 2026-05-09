#pragma once

#include "sensor_data.h"
#include "aggregated_metric.h"
#include <stdint.h>
#include <ArduinoJson.h>
#include "data_serialization.h"
#include "sensor_metric_utils.h"

class ISensorModule {
public:
  virtual ~ISensorModule() = default;

  virtual SensorType type() const = 0;

  virtual void begin() = 0;
  virtual void update(uint32_t nowMs) = 0;

  virtual void walkMetrics(SensorMetricVisitor visitor, void* context) const = 0;
  virtual void walkFields(SerializableSensor::FieldVisitor visitor) const = 0;

  virtual bool setCalibration(SensorField field, float reference) = 0;
  virtual bool getCalibration(JsonDocument& doc) const = 0;
};

