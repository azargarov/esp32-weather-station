#include "bme280_sensor.h"

#include <math.h>

bool Bme280Sensor::begin(uint8_t i2cAddress) {
  Wire.begin();

  available_ = driver_.begin(i2cAddress);
  lastReadOk_ = false;

  if (available_) {
    driver_.setSampling(Adafruit_BME280::MODE_FORCED,
                        Adafruit_BME280::SAMPLING_X1, // temperature
                        Adafruit_BME280::SAMPLING_X8, // pressure
                        Adafruit_BME280::SAMPLING_X1, // humidity
                        Adafruit_BME280::FILTER_X4);
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
    setNaN();
    lastReadOk_ = false;
    return false;
  }

  if (!driver_.takeForcedMeasurement()) {
    setNaN();
    lastReadOk_ = false;
    return false;
  }

  const float temperatureC = driver_.readTemperature();
  const float pressureHpa = driver_.readPressure() / 100.0f;
  const float humidityPercent = driver_.readHumidity();

  const bool ok = isfinite(temperatureC) && isfinite(pressureHpa) &&
                  isfinite(humidityPercent);

  if (!ok) {
    setNaN();
    lastReadOk_ = false;
    return false;
  }

  temperatureRawC_ = temperatureC;
  pressureRawHpa_ = pressureHpa;
  humidityRawPercent_ = humidityPercent;
  lastReadMs_ = millis();
  lastReadOk_ = true;
  return true;
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

void Bme280Sensor::setNaN() {
  temperatureRawC_ = NAN;
  pressureRawHpa_ = NAN;
  humidityRawPercent_ = NAN;
}