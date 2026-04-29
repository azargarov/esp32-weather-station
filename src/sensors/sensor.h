#pragma once
#include <Wire.h>

class CalibratedField {
public:
  CalibratedField(const char* name, const char* unit): name_(name), unit_(unit) {}

  void setRaw(float raw); 
  void setOffset(float offset);
  float raw() const;
  float offset() const;
  float value() const;

  const char* name() const;
  const char* unit() const ;

private:
  const char* name_;
  const char* unit_;

  float raw_ = NAN;
  float offset_ = 0.0f;
};