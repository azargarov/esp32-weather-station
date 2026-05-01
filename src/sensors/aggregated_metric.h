#pragma once

#include <math.h>
#include <stdint.h>

class MetricStats {
public:
  void add(float value);

  float average() const;

  float stddev() const;

  float latest() const;

  uint32_t count() const;

  bool hasValue() const;

  void reset();

private:
  uint32_t count_ = 0;
  float mean_ = 0.0f;
  float m2_ = 0.0f;
  float latest_ = NAN;
};