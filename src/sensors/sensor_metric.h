#pragma once
#include "sensor_types.h"

enum class SensorMetricType { Gauge, Counter, Info };

struct SensorMetric {
  const char *name = nullptr; // old style "bme280_available"
  float value = 0.0f;
  const char *unit = nullptr;
  const char *help = nullptr;
  SensorMetricType type = SensorMetricType::Gauge;

  const char *family = nullptr; // new style "mean_60s"
  SensorType sensor = SensorType::Unknown;
  SensorField field = SensorField::Unknown;

  // old style
  SensorMetric(const char *metricName, float metricValue,
               const char *metricUnit, const char *metricHelp,
               SensorMetricType metricType = SensorMetricType::Gauge)
      : name(metricName), value(metricValue), unit(metricUnit),
        help(metricHelp), type(metricType), family(nullptr),
        sensor(SensorType::Unknown), field(SensorField::Unknown) {}

  // new generic family + labels style
  SensorMetric(const char *metricFamily, SensorType metricSensor,
               SensorField metricField, float metricValue,
               const char *metricUnit, const char *metricHelp,
               SensorMetricType metricType = SensorMetricType::Gauge)
      : name(nullptr), value(metricValue), unit(metricUnit), help(metricHelp),
        type(metricType), family(metricFamily), sensor(metricSensor),
        field(metricField) {}
};

using SensorMetricVisitor = void (*)(const SensorMetric &metric, void *context);