#include "aggregated_metric.h"

void MetricStats::add(float value) {
  if (isnan(value)) {
    return;
  }

  count_++;

  const float delta = value - mean_;
  mean_ += delta / count_;

  const float delta2 = value - mean_;
  m2_ += delta * delta2;

  latest_ = value;
}

float MetricStats::average() const {
  if (count_ == 0) {
    return NAN;
  }

  return mean_;
}

float MetricStats::stddev() const {
  if (count_ < 2) {
    return 0.0f;
  }

  return sqrt(m2_ / count_);
}

float MetricStats::latest() const { return latest_; }

uint32_t MetricStats::count() const { return count_; }

bool MetricStats::hasValue() const { return count_ > 0; }

void MetricStats::reset() {
  count_ = 0;
  mean_ = 0.0f;
  m2_ = 0.0f;
  latest_ = NAN;
}
