#pragma once

#include "aggregated_metric.h"
#include "bme280_sensor.h"
#include "calibration_manager.h"
#include "data_serialization.h"
#include "sensor_data.h"
#include "sensor_types.h"

#include <ArduinoJson.h>

enum class SensorMetricType { 
  Gauge,
  Counter,
  Info
};

struct SensorMetric {
  const char *name = nullptr;
  const char *family = nullptr;
  
  SensorType sensor = SensorType::Unknown;
  SensorField field = SensorField::Unknown;
  SensorMetricType type = SensorMetricType::Gauge;

  float value = 0.0f;
  const char *unit = nullptr;
  const char *help = nullptr;

  SensorMetric() = default;

  SensorMetric(const char *metricName, float metricValue,
               const char *metricUnit, const char *metricHelp)
      : name(metricName), value(metricValue), unit(metricUnit),
        help(metricHelp), type(SensorMetricType::Gauge) {}

  SensorMetric(const char *metricName, float metricValue,
               const char *metricUnit, const char *metricHelp,
               SensorMetricType metricType)
      : name(metricName), value(metricValue), unit(metricUnit),
        help(metricHelp), type(metricType) {}
        
  SensorMetric(const char *metricFamily, SensorType metricSensor,
               SensorField metricField, float metricValue,
               const char *metricUnit, const char *metricHelp,
               SensorMetricType metricType = SensorMetricType::Gauge)
      : name(nullptr),
        family(metricFamily),
        sensor(metricSensor),
        field(metricField),
        value(metricValue),
        unit(metricUnit),
        help(metricHelp),
        type(metricType) {}
};

using SensorMetricVisitor = void (*)(const SensorMetric &metric, void *context);

struct Bme280Metrics {
  MetricStats temperature;
  MetricStats humidity;
  MetricStats pressure;

  bool hasValue() const {
    return temperature.hasValue() || humidity.hasValue() || pressure.hasValue();
  }
};

class SensorManager {
public:
  void begin();
  void update();
  void walkMetrics(SensorMetricVisitor visitor, void *context) const;

  SensorSnapshot snapshot() const;
  void walkFields(SerializableSensor::FieldVisitor visitor) const;

  SensorType parseSensorType(const char *sensor) const;
  const char *sensorTypeToString(SensorType st) const;
  SensorField parseSensorField(const char *field) const;
  const char *sensorFieldToString(SensorField field) const;

  bool setCalibration(SensorType st, const char *field, float reference);
  bool getCalibration(SensorType st, JsonDocument &doc) const;

private:
  Bme280Sensor bme280_;
  CalibrationManager calibration_;
  Bme280Metrics bme280Metrics_;

  unsigned long lastReadMs_ = 0;
  unsigned long lastProbeMs_ = 0;

  uint32_t bme280ReadErrorsTotal_ = 0;

  void readSensors();
  void probeBME280();
  void addSample(const SensorSample &sample);
  float latestRawValue(SensorType sensor, SensorField field) const;

  bool bme280Present_ = false;
};
