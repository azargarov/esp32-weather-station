#include "sensor_metric_utils.h"
#include "aggregated_metric.h"
#include "calibration_manager.h"

#include <math.h>

void appendStatMetric(SensorMetricVisitor visitor, void *context,
                      SensorType sensor, SensorField field, const char *family,
                      float value, const char *unit, const char *help) {
  if (!isfinite(value)) {
    return;
  }

  visitor({family, sensor, field, value, unit, help, SensorMetricType::Gauge},
          context);
}

void walkFieldStats(SensorMetricVisitor visitor, void *context,
                    SensorType sensor, SensorField field,
                    const MetricStats &stats) {
  if (!stats.hasValue()) {
    return;
  }

  const char *unit = sensorFieldUnit(field);

  appendStatMetric(visitor, context, sensor, field, "mean_60s", stats.average(),
                   unit, "Mean value over the last 60-second rolling window.");

  appendStatMetric(visitor, context, sensor, field, "median_60s",
                   stats.median(), unit,
                   "Median value over the last 60-second rolling window.");

  appendStatMetric(visitor, context, sensor, field, "min_60s", stats.min(),
                   unit,
                   "Minimum value over the last 60-second rolling window.");

  appendStatMetric(visitor, context, sensor, field, "max_60s", stats.max(),
                   unit,
                   "Maximum value over the last 60-second rolling window.");

  appendStatMetric(visitor, context, sensor, field, "range_60s", stats.range(),
                   unit, "Range over the last 60-second rolling window.");

  appendStatMetric(
      visitor, context, sensor, field, "stddev_60s", stats.stddev(), unit,
      "Standard deviation over the last 60-second rolling window.");

  appendStatMetric(
      visitor, context, sensor, field, "slope_per_minute_60s",
      stats.slopePerMinute(), unit,
      "Linear regression slope over the rolling window, expressed per minute.");

  visitor({"samples_60s", sensor, field, static_cast<float>(stats.count()),
           nullptr, "Number of valid samples in the rolling statistics window.",
           SensorMetricType::Gauge},
          context);
}

void walkCalibrationMetric(SensorMetricVisitor visitor, void *context,
                           SensorType sensor, SensorField field,
                           const char *offsetUnit,
                           const CalibrationProfile &profile) {
  visitor({"calibration_enabled", sensor, field, profile.enabled ? 1.0f : 0.0f,
           nullptr, "Whether calibration is enabled for this sensor field.",
           SensorMetricType::Gauge},
          context);

  visitor({"calibration_scale", sensor, field, profile.scale, nullptr,
           "Linear calibration scale coefficient.", SensorMetricType::Gauge},
          context);

  visitor({"calibration_offset", sensor, field, profile.offset, offsetUnit,
           "Linear calibration offset coefficient.", SensorMetricType::Gauge},
          context);

  visitor({"calibration_mode", sensor, field,
           static_cast<float>(static_cast<uint8_t>(profile.mode)), nullptr,
           "Calibration mode as numeric enum: 0 none, 1 offset, 2 two-point.",
           SensorMetricType::Gauge},
          context);
}
