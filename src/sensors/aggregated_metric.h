#pragma once

#include <math.h>
#include <stdint.h>

class MetricStats {
public:
  static constexpr uint8_t kCapacity = 60;

  void add(float value);

  float average() const;
  float median() const;
  float min() const;
  float max() const;
  float range() const;
  float stddev() const;
  float slopePerMinute() const;
  float latest() const;

  uint32_t count() const;
  bool hasValue() const;

  void reset();

private:
  float values_[kCapacity]{};
  uint8_t nextIndex_ = 0;
  uint8_t count_ = 0;
  float latest_ = NAN;

  uint8_t orderedValues(float *out, uint8_t outSize) const;
};
