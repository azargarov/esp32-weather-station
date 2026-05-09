#pragma once

#include "aggregated_metric.h"
#include "calibration_manager.h"
#include "sensor_metric.h"
#include "sensor_types.h"

void appendStatMetric(SensorMetricVisitor visitor, void *context,
                      SensorType sensor, SensorField field, const char *family,
                      float value, const char *unit, const char *help);

void walkFieldStats(SensorMetricVisitor visitor, void *context,
                    SensorType sensor, SensorField field,
                    const MetricStats &stats);

void walkCalibrationMetric(SensorMetricVisitor visitor, void *context,
                           SensorType sensor, SensorField field,
                           const char *offsetUnit,
                           const CalibrationProfile &profile);