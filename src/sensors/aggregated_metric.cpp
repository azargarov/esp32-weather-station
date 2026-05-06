#include "aggregated_metric.h"

#include <math.h>

namespace {
void insertionSort(float *values, uint8_t count) {
  for (uint8_t i = 1; i < count; ++i) {
    const float key = values[i];
    int8_t j = static_cast<int8_t>(i) - 1;

    while (j >= 0 && values[j] > key) {
      values[j + 1] = values[j];
      --j;
    }

    values[j + 1] = key;
  }
}
} // namespace

void MetricStats::add(float value) {
  if (!isfinite(value)) {
    return;
  }

  values_[nextIndex_] = value;
  nextIndex_ = (nextIndex_ + 1) % kCapacity;

  if (count_ < kCapacity) {
    count_++;
  }

  latest_ = value;
}

float MetricStats::average() const {
  if (count_ == 0) {
    return NAN;
  }

  float sum = 0.0f;
  for (uint8_t i = 0; i < count_; ++i) {
    sum += values_[i];
  }
  return sum / count_;
}

float MetricStats::median() const {
  if (count_ == 0) {
    return NAN;
  }

  float sorted[kCapacity];
  const uint8_t n = orderedValues(sorted, kCapacity);
  insertionSort(sorted, n);

  if ((n % 2) == 1) {
    return sorted[n / 2];
  }
  return (sorted[n / 2 - 1] + sorted[n / 2]) / 2.0f;
}

float MetricStats::min() const {
  if (count_ == 0) {
    return NAN;
  }

  float result = values_[0];
  for (uint8_t i = 1; i < count_; ++i) {
    if (values_[i] < result) {
      result = values_[i];
    }
  }
  return result;
}

float MetricStats::max() const {
  if (count_ == 0) {
    return NAN;
  }

  float result = values_[0];
  for (uint8_t i = 1; i < count_; ++i) {
    if (values_[i] > result) {
      result = values_[i];
    }
  }
  return result;
}

float MetricStats::range() const {
  if (count_ == 0) {
    return NAN;
  }
  return max() - min();
}

float MetricStats::stddev() const {
  if (count_ < 2) {
    return 0.0f;
  }

  const float mean = average();
  float sumSquares = 0.0f;
  for (uint8_t i = 0; i < count_; ++i) {
    const float delta = values_[i] - mean;
    sumSquares += delta * delta;
  }
  return sqrtf(sumSquares / count_);
}

float MetricStats::slopePerMinute() const {
  if (count_ < 2) {
    return 0.0f;
  }

  float ordered[kCapacity];
  const uint8_t n = orderedValues(ordered, kCapacity);

  const float meanX = (n - 1) / 2.0f;
  float meanY = 0.0f;
  for (uint8_t i = 0; i < n; ++i) {
    meanY += ordered[i];
  }
  meanY /= n;

  float numerator = 0.0f;
  float denominator = 0.0f;
  for (uint8_t i = 0; i < n; ++i) {
    const float dx = static_cast<float>(i) - meanX;
    const float dy = ordered[i] - meanY;
    numerator += dx * dy;
    denominator += dx * dx;
  }

  if (denominator == 0.0f) {
    return 0.0f;
  }

  return (numerator / denominator) * 60.0f;
}

float MetricStats::latest() const { return latest_; }

uint32_t MetricStats::count() const { return count_; }

bool MetricStats::hasValue() const { return count_ > 0; }

void MetricStats::reset() {
  nextIndex_ = 0;
  count_ = 0;
  latest_ = NAN;
}

uint8_t MetricStats::orderedValues(float *out, uint8_t outSize) const {
  if (out == nullptr || outSize < count_) {
    return 0;
  }

  const uint8_t start = (count_ == kCapacity) ? nextIndex_ : 0;
  for (uint8_t i = 0; i < count_; ++i) {
    const uint8_t index = (start + i) % kCapacity;
    out[i] = values_[index];
  }
  return count_;
}
