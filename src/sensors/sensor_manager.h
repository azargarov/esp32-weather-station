#pragma once

#include "bme280_sensor.h"
#include "data_serialization.h"
#include "sensor_data.h"
#include <ArduinoJson.h>

enum class SensorType { Bme280, Unknown };

enum class SensorMetricValueType { Float, Bool };

struct SensorMetric {
  const char *name;
  float value;
  const char *unit;
  const char *help;
  SensorMetricValueType type;

  SensorMetric(const char *metricName, float metricValue,
               const char *metricUnit, const char *metricHelp)
      : name(metricName), value(metricValue), unit(metricUnit),
        help(metricHelp), type(SensorMetricValueType::Float) {}

  SensorMetric(const char *metricName, float metricValue,
               const char *metricUnit, const char *metricHelp,
               SensorMetricValueType metricType)
      : name(metricName), value(metricValue), unit(metricUnit),
        help(metricHelp), type(metricType) {}
};

using SensorMetricVisitor = void (*)(const SensorMetric &metric, void *context);

class SensorManager {
public:
  void begin();
  void update();
  void walkMetrics(SensorMetricVisitor visitor, void *context) const;

  SensorSnapshot snapshot() const;
  void walkFields(SerializableSensor::FieldVisitor visitor) const;

  SensorType parseSensorType(const char *sensor);
  const char *sensorTypeToString(SensorType st);
  bool setCalibration(SensorType st, const char *field, float reference);
  bool getCalibration(SensorType st, JsonDocument &doc);

private:
  Bme280Sensor bme280_;
  Bme280Metrics bme280Current_;
  Bme280Metrics bme280Previous_;

  unsigned long lastReadMs_ = 0;
  unsigned long lastProbeMs_ = 0;
  unsigned long lastMetricRotationMs_ = 0;

  uint32_t bme280ReadErrorsTotal_ = 0;

  void readSensors();
  void probeBME280();
  void rotateMetricBuckets();

  bool bme280Present_ = false;
};
