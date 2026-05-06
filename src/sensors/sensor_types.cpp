#include "sensor_types.h"

const char* sensorTypeToString(SensorType st) {
  switch (st) {
    case SensorType::Bme280: return "bme280";
    case SensorType::Bh1750: return "bh1750";
    case SensorType::Adc:    return "adc";
    default:                 return "unknown";
  }
}

const char* sensorFieldToString(SensorField field) {
  switch (field) {
    case SensorField::Temperature:   return "temperature";
    case SensorField::Humidity:      return "humidity";
    case SensorField::Pressure:      return "pressure";
    case SensorField::Illuminance:   return "illuminance";
    case SensorField::SupplyVoltage: return "supply_voltage";
    default:                         return "unknown";
  }
}

const char* sensorFieldUnit(SensorField field) {
  switch (field) {
    case SensorField::Temperature:   return "celsius";
    case SensorField::Humidity:      return "percent";
    case SensorField::Pressure:      return "hpa";
    case SensorField::Illuminance:   return "lux";
    case SensorField::SupplyVoltage: return "volt";
    default:                         return "";
  }
}

SensorType parseSensorType(const char *sensor) {
  if (sensor == nullptr) {
    return SensorType::Unknown;
  }

  if (strcmp(sensor, "bme280") == 0) {
    return SensorType::Bme280;
  }
  if (strcmp(sensor, "bh1750") == 0) {
    return SensorType::Bh1750;
  }
  if (strcmp(sensor, "adc") == 0) {
    return SensorType::Adc;
  }

  return SensorType::Unknown;
}

SensorField parseSensorField(const char *field) {
  if (field == nullptr) {
    return SensorField::Unknown;
  }

  if (strcmp(field, "temperature") == 0) {
    return SensorField::Temperature;
  }
  if (strcmp(field, "humidity") == 0) {
    return SensorField::Humidity;
  }
  if (strcmp(field, "pressure") == 0) {
    return SensorField::Pressure;
  }
  if (strcmp(field, "illuminance") == 0) {
    return SensorField::Illuminance;
  }
  if (strcmp(field, "supply_voltage") == 0) {
    return SensorField::SupplyVoltage;
  }

  return SensorField::Unknown;
}