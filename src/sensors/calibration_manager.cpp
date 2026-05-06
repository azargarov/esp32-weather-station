#include "calibration_manager.h"

#include <Preferences.h>
#include <math.h>
#include <stdio.h>

namespace {

constexpr const char *kNamespace = "cal";
constexpr float kMinRawDistance = 0.000001f;

const char *calibrationModeToString(CalibrationMode mode) {
  switch (mode) {
  case CalibrationMode::OffsetOnly:
    return "offset";
  case CalibrationMode::LinearTwoPoint:
    return "two_point";
  default:
    return "none";
  }
}

void makeKey(char *out, size_t outSize, const char *prefix, const char *suffix) {
  snprintf(out, outSize, "%s_%s", prefix, suffix);
}

} // namespace

bool CalibrationManager::begin() {
  bool ok = true;
  ok = loadProfile({SensorType::Bme280, SensorField::Temperature}) && ok;
  ok = loadProfile({SensorType::Bme280, SensorField::Humidity}) && ok;
  ok = loadProfile({SensorType::Bme280, SensorField::Pressure}) && ok;
  ok = loadProfile({SensorType::Bh1750, SensorField::Illuminance}) && ok;
  ok = loadProfile({SensorType::Adc, SensorField::SupplyVoltage}) && ok;
  return ok;
}

float CalibrationManager::apply(CalibrationKey key, float raw) const {
  if (!isfinite(raw) || !validKey(key)) {
    return raw;
  }

  const CalibrationProfile &profile = profiles_[indexFor(key)];
  if (!profile.enabled) {
    return raw;
  }

  return profile.scale * raw + profile.offset;
}

bool CalibrationManager::setOffsetCalibration(CalibrationKey key, float raw,
                                              float reference) {
  if (!validKey(key) || !isfinite(raw) || !isfinite(reference)) {
    return false;
  }

  CalibrationProfile profile;
  profile.enabled = true;
  profile.mode = CalibrationMode::OffsetOnly;
  profile.scale = 1.0f;
  profile.offset = reference - raw;
  profile.hasP1 = true;
  profile.p1.raw = raw;
  profile.p1.reference = reference;

  profiles_[indexFor(key)] = profile;
  return saveProfile(key);
}

bool CalibrationManager::setTwoPointCalibration(CalibrationKey key, float raw1,
                                                float ref1, float raw2,
                                                float ref2) {
  if (!validKey(key) || !isfinite(raw1) || !isfinite(ref1) ||
      !isfinite(raw2) || !isfinite(ref2)) {
    return false;
  }

  const float dx = raw2 - raw1;
  if (fabsf(dx) < kMinRawDistance) {
    return false;
  }

  CalibrationProfile profile;
  profile.enabled = true;
  profile.mode = CalibrationMode::LinearTwoPoint;
  profile.scale = (ref2 - ref1) / dx;
  profile.offset = ref1 - profile.scale * raw1;
  profile.hasP1 = true;
  profile.hasP2 = true;
  profile.p1.raw = raw1;
  profile.p1.reference = ref1;
  profile.p2.raw = raw2;
  profile.p2.reference = ref2;


  if (!isfinite(profile.scale) || !isfinite(profile.offset)) {
    return false;
  }

  profiles_[indexFor(key)] = profile;
  return saveProfile(key);
}

bool CalibrationManager::clearCalibration(CalibrationKey key) {
  if (!validKey(key)) {
    return false;
  }

  profiles_[indexFor(key)] = CalibrationProfile{};
  return removeProfile(key);
}

bool CalibrationManager::hasCalibration(CalibrationKey key) const {
  return validKey(key) && profiles_[indexFor(key)].enabled;
}

CalibrationProfile CalibrationManager::getProfile(CalibrationKey key) const {
  if (!validKey(key)) {
    return CalibrationProfile{};
  }
  return profiles_[indexFor(key)];
}

bool CalibrationManager::writeProfileJson(CalibrationKey key,
                                          JsonObject obj) const {
  if (!validKey(key)) {
    return false;
  }

  const CalibrationProfile &profile = profiles_[indexFor(key)];
  obj["sensor"] = sensorTypeToString(key.sensor);
  obj["field"] = sensorFieldToString(key.field);
  obj["unit"] = sensorFieldUnit(key.field);
  obj["enabled"] = profile.enabled;
  obj["mode"] = calibrationModeToString(profile.mode);
  obj["scale"] = profile.scale;
  obj["offset"] = profile.offset;

  if (profile.hasP1) {
    JsonObject point = obj["p1"].to<JsonObject>();
    point["raw"] = profile.p1.raw;
    point["reference"] = profile.p1.reference;
  }

  if (profile.hasP2) {
    JsonObject point = obj["p2"].to<JsonObject>();
    point["raw"] = profile.p2.raw;
    point["reference"] = profile.p2.reference;
  }

  return true;
}

bool CalibrationManager::writeSensorJson(SensorType sensor,
                                         JsonDocument &doc) const {
  doc["sensor"] = sensorTypeToString(sensor);

  if (sensor == SensorType::Bme280) {
    writeProfileJson({sensor, SensorField::Temperature},
                     doc["temperature"].to<JsonObject>());
    writeProfileJson({sensor, SensorField::Humidity},
                     doc["humidity"].to<JsonObject>());
    writeProfileJson({sensor, SensorField::Pressure},
                     doc["pressure"].to<JsonObject>());
    return true;
  }

  if (sensor == SensorType::Bh1750) {
    writeProfileJson({sensor, SensorField::Illuminance},
                     doc["illuminance"].to<JsonObject>());
    return true;
  }

  if (sensor == SensorType::Adc) {
    writeProfileJson({sensor, SensorField::SupplyVoltage},
                     doc["supply_voltage"].to<JsonObject>());
    return true;
  }

  return false;
}

uint8_t CalibrationManager::indexFor(CalibrationKey key) const {
  if (key.sensor == SensorType::Bme280) {
    switch (key.field) {
    case SensorField::Temperature:
      return 0;
    case SensorField::Humidity:
      return 1;
    case SensorField::Pressure:
      return 2;
    default:
      return 0;
    }
  }

  if (key.sensor == SensorType::Bh1750 &&
      key.field == SensorField::Illuminance) {
    return 3;
  }

  if (key.sensor == SensorType::Adc &&
      key.field == SensorField::SupplyVoltage) {
    return 4;
  }

  return 0;
}

bool CalibrationManager::validKey(CalibrationKey key) const {
  if (key.sensor == SensorType::Bme280) {
    return key.field == SensorField::Temperature ||
           key.field == SensorField::Humidity ||
           key.field == SensorField::Pressure;
  }
  if (key.sensor == SensorType::Bh1750) {
    return key.field == SensorField::Illuminance;
  }
  if (key.sensor == SensorType::Adc) {
    return key.field == SensorField::SupplyVoltage;
  }
  return false;
}

const char *CalibrationManager::storagePrefix(CalibrationKey key) const {
  if (key.sensor == SensorType::Bme280) {
    switch (key.field) {
    case SensorField::Temperature:
      return "bt";
    case SensorField::Humidity:
      return "bh";
    case SensorField::Pressure:
      return "bp";
    default:
      return "xx";
    }
  }
  if (key.sensor == SensorType::Bh1750 &&
      key.field == SensorField::Illuminance) {
    return "li";
  }
  if (key.sensor == SensorType::Adc &&
      key.field == SensorField::SupplyVoltage) {
    return "sv";
  }
  return "xx";
}

bool CalibrationManager::loadProfile(CalibrationKey key) {
  if (!validKey(key)) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kNamespace, true)) {
    return false;
  }

  CalibrationProfile profile;
  const char *prefix = storagePrefix(key);
  char name[14];

  makeKey(name, sizeof(name), prefix, "en");
  profile.enabled = prefs.getBool(name, false);

  makeKey(name, sizeof(name), prefix, "mode");
  profile.mode = static_cast<CalibrationMode>(prefs.getUChar(name, 0));

  makeKey(name, sizeof(name), prefix, "scale");
  profile.scale = prefs.getFloat(name, 1.0f);

  makeKey(name, sizeof(name), prefix, "offset");
  profile.offset = prefs.getFloat(name, 0.0f);

  makeKey(name, sizeof(name), prefix, "h1");
  profile.hasP1 = prefs.getBool(name, false);

  makeKey(name, sizeof(name), prefix, "r1");
  profile.p1.raw = prefs.getFloat(name, 0.0f);

  makeKey(name, sizeof(name), prefix, "f1");
  profile.p1.reference = prefs.getFloat(name, 0.0f);

  makeKey(name, sizeof(name), prefix, "h2");
  profile.hasP2 = prefs.getBool(name, false);

  makeKey(name, sizeof(name), prefix, "r2");
  profile.p2.raw = prefs.getFloat(name, 0.0f);

  makeKey(name, sizeof(name), prefix, "f2");
  profile.p2.reference = prefs.getFloat(name, 0.0f);

  prefs.end();

  profiles_[indexFor(key)] = profile;
  return true;
}

bool CalibrationManager::saveProfile(CalibrationKey key) const {
  if (!validKey(key)) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }

  const CalibrationProfile &profile = profiles_[indexFor(key)];
  const char *prefix = storagePrefix(key);
  char name[14];

  makeKey(name, sizeof(name), prefix, "en");
  prefs.putBool(name, profile.enabled);

  makeKey(name, sizeof(name), prefix, "mode");
  prefs.putUChar(name, static_cast<uint8_t>(profile.mode));

  makeKey(name, sizeof(name), prefix, "scale");
  prefs.putFloat(name, profile.scale);

  makeKey(name, sizeof(name), prefix, "offset");
  prefs.putFloat(name, profile.offset);

  makeKey(name, sizeof(name), prefix, "h1");
  prefs.putBool(name, profile.hasP1);

  makeKey(name, sizeof(name), prefix, "r1");
  prefs.putFloat(name, profile.p1.raw);

  makeKey(name, sizeof(name), prefix, "f1");
  prefs.putFloat(name, profile.p1.reference);

  makeKey(name, sizeof(name), prefix, "h2");
  prefs.putBool(name, profile.hasP2);

  makeKey(name, sizeof(name), prefix, "r2");
  prefs.putFloat(name, profile.p2.raw);

  makeKey(name, sizeof(name), prefix, "f2");
  prefs.putFloat(name, profile.p2.reference);

  prefs.end();
  return true;
}

bool CalibrationManager::removeProfile(CalibrationKey key) const {
  if (!validKey(key)) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) {
    return false;
  }

  const char *prefix = storagePrefix(key);
  char name[14];
  const char *suffixes[] = {"en",   "mode", "scale", "offset", "h1",
                            "r1",   "f1",   "h2",    "r2",     "f2"};

  for (const char *suffix : suffixes) {
    makeKey(name, sizeof(name), prefix, suffix);
    prefs.remove(name);
  }

  prefs.end();
  return true;
}
