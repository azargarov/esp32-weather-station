#include "sensor.h"


  void CalibratedField::setRaw(float raw) {
    raw_ = raw;
  }

  void CalibratedField::setOffset(float offset) {
    offset_ = offset;
  }

  float CalibratedField::raw() const {
    return raw_;
  }

  float CalibratedField::offset() const {
    return offset_;
  }

  float CalibratedField::value() const {
    return raw_ + offset_;
  }

  const char* CalibratedField::name() const {
    return name_;
  }

  const char* CalibratedField::unit() const {
    return unit_;
  }
