#pragma once

#include <stdint.h>
#include <string.h>

enum class SensorType : uint8_t { Bme280 = 0, Bh1750, Adc, Unknown };

enum class SensorField : uint8_t {
  Temperature = 0,
  Humidity,
  Pressure,
  Illuminance,
  SupplyVoltage,
  Unknown
};

SensorType parseSensorType(const char *sensor);
SensorField parseSensorField(const char *field);

const char *sensorTypeToString(SensorType st);
const char *sensorFieldToString(SensorField field);
const char *sensorFieldUnit(SensorField field);
// const char* calibrationModeToString(CalibrationMode mode);