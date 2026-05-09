#pragma once

#include "sensor_types.h"
#include <ArduinoJson.h>

enum class CalibrationMode : uint8_t {
  None = 0,
  OffsetOnly,
  LinearTwoPoint,
};

struct CalibrationPoint {
  float raw = 0.0f;
  float reference = 0.0f;
};

struct CalibrationProfile {
  bool enabled = false;
  CalibrationMode mode = CalibrationMode::None;
  float scale = 1.0f;
  float offset = 0.0f;
  bool hasP1 = false;
  bool hasP2 = false;
  CalibrationPoint p1{};
  CalibrationPoint p2{};
};

struct CalibrationKey {
  SensorType sensor{SensorType::Unknown};
  SensorField field{SensorField::Unknown};

  constexpr CalibrationKey() = default;
  constexpr CalibrationKey(SensorType s, SensorField f) : sensor(s), field(f) {}

  bool operator==(const CalibrationKey &other) const {
    return sensor == other.sensor && field == other.field;
  }
};

class CalibrationManager {
public:
  bool begin();

  float apply(CalibrationKey key, float raw) const;

  bool setOffsetCalibration(CalibrationKey key, float raw, float reference);
  bool setTwoPointCalibration(CalibrationKey key, float raw1, float ref1,
                              float raw2, float ref2);
  bool clearCalibration(CalibrationKey key);

  bool hasCalibration(CalibrationKey key) const;
  CalibrationProfile getProfile(CalibrationKey key) const;

  bool writeProfileJson(CalibrationKey key, JsonObject obj) const;
  bool writeSensorJson(SensorType sensor, JsonDocument &doc) const;

private:
  static constexpr uint8_t kProfileCount = 5;
  CalibrationProfile profiles_[kProfileCount];

  uint8_t indexFor(CalibrationKey key) const;
  bool validKey(CalibrationKey key) const;

  bool loadProfile(CalibrationKey key);
  bool saveProfile(CalibrationKey key) const;
  bool removeProfile(CalibrationKey key) const;

  const char *storagePrefix(CalibrationKey key) const;
};
