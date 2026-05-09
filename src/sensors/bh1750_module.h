#pragma once

#include "bh1750_sensor.h"
#include "calibration_manager.h"
#include "sensor_module.h"

class Bh1750Module : public ISensorModule {
public:
  explicit Bh1750Module(CalibrationManager &calibration);

  SensorType type() const override;

  void begin() override;
  void update(uint32_t nowMs) override;

  void walkMetrics(SensorMetricVisitor visitor, void *context) const override;
  void walkFields(SerializableSensor::FieldVisitor visitor) const override;

  bool setCalibration(SensorField field, float reference) override;
  bool getCalibration(JsonDocument &doc) const override;

  bool available() const;
  bool lastReadOk() const;

private:
  void probe();
  void readSensor();
  void addSample(const SensorSample &sample);
  float latestRawValue(SensorField field) const;

private:
  Bh1750Sensor sensor_;
  Bh1750Metrics metrics_;
  CalibrationManager &calibration_;

  uint32_t lastReadMs_ = 0;
  uint32_t lastProbeMs_ = 0;
  uint32_t readErrorsTotal_ = 0;
  bool present_ = false;
};