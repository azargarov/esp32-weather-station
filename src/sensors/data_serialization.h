#pragma once

#include <functional>

class SerializableSensor {
public:
  virtual ~SerializableSensor() = default;

  using FieldVisitor =
      std::function<void(const char *key, float value, const char *unit)>;

  virtual void walkFields(FieldVisitor visitor) const = 0;
};
