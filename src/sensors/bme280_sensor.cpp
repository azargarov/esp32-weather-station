#include "bme280_sensor.h"

bool Bme280Sensor::begin(uint8_t i2cAddress) {
  Wire.begin();
  available_ = bme_.begin(i2cAddress);
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

  const float temp = bme_.readTemperature();
  const float hum = bme_.readHumidity();
  const float pressPa = bme_.readPressure();

  if (isnan(temp) || isnan(hum) || isnan(pressPa)) {
    lastReadOk_ = false;
    return false;
  }

  temperatureC_ = temp;
  humidityPct_ = hum;
  pressureHpa_ = pressPa / 100.0f;
  lastReadOk_ = true;

  return true;
}

bool Bme280Sensor::available() const {
  return available_;
}

bool Bme280Sensor::lastReadOk() const {
  return lastReadOk_;
}

void Bme280Sensor::walkFields(FieldVisitor visitor) const {
  if (!available_ || !lastReadOk_) {
    return;
  }

  visitor("temperature", temperatureC_, "celsius");
  visitor("humidity", humidityPct_, "percent");
  visitor("pressure", pressureHpa_, "hpa");
}
