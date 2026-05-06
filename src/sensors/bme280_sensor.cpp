#include "bme280_sensor.h"

#include <math.h>

bool Bme280Sensor::begin(uint8_t i2cAddress) {
  Wire.begin();

  available_ = driver_.begin(i2cAddress);
  lastReadOk_ = false;

  if (available_) {
    Serial.print("[bme280] initialized at 0x");
    Serial.println(i2cAddress, HEX);
  } else {
    Serial.print("[bme280] not found at 0x");
    Serial.println(i2cAddress, HEX);
  }

  return available_;
}

bool Bme280Sensor::read() {
  if (!available_) {
    lastReadOk_ = false;
    return false;
  }

  temperatureRawC_ = driver_.readTemperature();
  pressureRawHpa_ = driver_.readPressure() / 100.0f;
  humidityRawPercent_ = driver_.readHumidity();

  lastReadMs_ = millis();

  lastReadOk_ = isfinite(temperatureRawC_) && isfinite(pressureRawHpa_) &&
                isfinite(humidityRawPercent_);

  return lastReadOk_;
}

bool Bme280Sensor::available() const { return available_; }

bool Bme280Sensor::lastReadOk() const { return lastReadOk_; }

Bme280Reading Bme280Sensor::lastReading() const {
  Bme280Reading reading;
  reading.valid = lastReadOk_;
  reading.temperatureC = temperatureRawC_;
  reading.humidityPercent = humidityRawPercent_;
  reading.pressureHpa = pressureRawHpa_;
  reading.timestampMs = lastReadMs_;
  return reading;
}

SensorSample Bme280Sensor::temperatureSample() const {
  return {SensorType::Bme280, SensorField::Temperature, lastReadOk_,
          temperatureRawC_, lastReadMs_};
}

SensorSample Bme280Sensor::humiditySample() const {
  return {SensorType::Bme280, SensorField::Humidity, lastReadOk_,
          humidityRawPercent_, lastReadMs_};
}

SensorSample Bme280Sensor::pressureSample() const {
  return {SensorType::Bme280, SensorField::Pressure, lastReadOk_,
          pressureRawHpa_, lastReadMs_};
}
