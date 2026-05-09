#include "bh1750_sensor.h"

#include <math.h>

bool Bh1750Sensor::begin(uint8_t i2cAddress) {
  Wire.begin();

  available_ = driver_.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, i2cAddress);
  lastReadOk_ = false;

  if (available_) {
    Serial.print("[bh1750] initialized at 0x");
    Serial.println(i2cAddress, HEX);
  } else {
    Serial.print("[bh1750] not found at 0x");
    Serial.println(i2cAddress, HEX);
  }

  return available_;
}

bool Bh1750Sensor::read() {
  if (!available_) {
    lastReadOk_ = false;
    luxRaw_ = NAN;
    return false;
  }

  float lux = driver_.readLightLevel();
  bool ok = isfinite(lux) && lux >= 0.0f;

  if (ok) {
    luxRaw_ = lux;
    lastReadMs_ = millis();
  } else {
    luxRaw_ = NAN;
  }

  lastReadOk_ = ok;
  return ok;
}

bool Bh1750Sensor::available() const { return available_; }

bool Bh1750Sensor::lastReadOk() const { return lastReadOk_; }

Bh1750Reading Bh1750Sensor::lastReading() const {
  Bh1750Reading reading;
  reading.valid = lastReadOk_;
  reading.lux = luxRaw_;
  return reading;
}

SensorSample Bh1750Sensor::luxSample() const {
  return {SensorType::Bh1750, SensorField::Illuminance, lastReadOk_, luxRaw_,
          lastReadMs_};
}
