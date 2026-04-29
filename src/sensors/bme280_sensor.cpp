#include "bme280_sensor.h"
#include <Preferences.h>
#include <mat.h>

static float round1(float value) {
  return roundf(value * 10.0f) / 10.0f;
}

Bme280Field Bme280Sensor::parseBme280Field(const char* field) {
  if (strcmp(field, "temperature") == 0) {
    return Bme280Field::Temperature;
  }

  if (strcmp(field, "humidity") == 0) {
    return Bme280Field::Humidity;
  }

  if (strcmp(field, "pressure") == 0) {
    return Bme280Field::Pressure;
  }

  return Bme280Field::Unknown;
}


bool Bme280Sensor::begin(uint8_t i2cAddress) {
  Wire.begin();

  available_ = driver_.begin(i2cAddress);
  lastReadOk_ = false;

  if (available_) {
    Serial.print("[bme280] initialized at 0x");
    Serial.println(i2cAddress, HEX);

    loadCalibration();
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

  temperature_.setRaw(driver_.readTemperature());
  pressure_.setRaw(driver_.readPressure() / 100.0f);
  humidity_.setRaw(driver_.readHumidity());

  lastReadMs_ = millis();

  lastReadOk_ =
    !isnan(temperature_.raw()) &&
    !isnan(pressure_.raw()) &&
    !isnan(humidity_.raw());

  return lastReadOk_;
}

bool Bme280Sensor::available() const { return available_; }

bool Bme280Sensor::lastReadOk() const { return lastReadOk_; }

Bme280Reading Bme280Sensor::lastReading() const {
  Bme280Reading reading;

  reading.valid = lastReadOk_;

  reading.temperatureC = temperature_.value();
  reading.temperatureRawC = temperature_.raw();
  reading.temperatureOffsetC = temperature_.offset();

  reading.humidityPercent = humidity_.value();
  reading.humidityRawPercent = humidity_.raw();
  reading.humidityOffsetPercent = humidity_.offset();

  reading.pressureHpa = pressure_.value();
  reading.pressureRawHpa = pressure_.raw();
  reading.pressureOffsetHpa = pressure_.offset();

  reading.timestampMs = lastReadMs_;

  return reading;
}

void Bme280Sensor::walkFields(FieldVisitor visitor) const {
  if (!available_ || !lastReadOk_) {
    return;
  }

  visitor(temperature_.name(), temperature_.value(), temperature_.unit());
  visitor(humidity_.name(), humidity_.value(), humidity_.unit());
  visitor(pressure_.name(), pressure_.value(), pressure_.unit());
}

bool Bme280Sensor::loadCalibration() {
  Preferences prefs;

  if (!prefs.begin("bme280", true)) {
    return false;
  }

  temperature_.setOffset(prefs.getFloat("temp_off", 0.0f));
  humidity_.setOffset(prefs.getFloat("hum_off", 0.0f));
  pressure_.setOffset(prefs.getFloat("press_off", 0.0f));

  prefs.end();

  return true;
}

bool Bme280Sensor::saveCalibration() {
  Preferences prefs;

  if (!prefs.begin("bme280", false)) {
    return false;
  }

  prefs.putFloat("temp_off", temperature_.offset());
  prefs.putFloat("hum_off", humidity_.offset());
  prefs.putFloat("press_off", pressure_.offset());

  prefs.end();

  return true;
}

void Bme280Sensor::clearCalibration() {
  Preferences prefs;

  if (prefs.begin("bme280", false)) {
    prefs.clear();
    prefs.end();
  }

  temperature_.setOffset(0.0f);
  humidity_.setOffset(0.0f);
  pressure_.setOffset(0.0f);
}

void Bme280Sensor::setTemperatureOffset(float offset) {
  temperature_.setOffset(offset);
}

void Bme280Sensor::setHumidityOffset(float offset) {
  humidity_.setOffset(offset);
}

void Bme280Sensor::setPressureOffset(float offset) {
  pressure_.setOffset(offset);
}

bool Bme280Sensor::setCalibrationFromReference(Bme280Field f, float reference){
  
  if (!lastReadOk_) {
    return false;
  }
  
  float offset = 0.0f; 

  switch(f){
    case Bme280Field::Temperature:
      offset = round1(reference - temperature_.raw()) ;
      setTemperatureOffset(offset);
      break;
    case Bme280Field::Humidity:
      offset = round1(reference - humidity_.raw());
      setHumidityOffset(offset);
      break;
    case Bme280Field::Pressure:
      offset = round1(reference - pressure_.raw()); 
      setPressureOffset(offset);
      break;
    
    default:
      return false;
  }
  return saveCalibration();
}

bool Bme280Sensor::getCalibration(JsonDocument & doc) const{

  doc["sensor"] = "bme280";
  doc["available"] = available_;

  doc["temperature"]["offset"] = temperature_.offset();
  doc["temperature"]["unit"] = temperature_.unit();

  doc["humidity"]["offset"] = humidity_.offset();
  doc["humidity"]["unit"] = humidity_.unit();

  doc["pressure"]["offset"] = pressure_.offset();
  doc["pressure"]["unit"] = pressure_.unit();

  return true;

}
