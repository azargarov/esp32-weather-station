#pragma once

#include "device_service.h"
#include "sensors/sensor_manager.h"
#include "metrics_formatter.h"
#include <Arduino.h>
#include <WebServer.h>

class HttpServer {
public:
  HttpServer(SensorManager &sensorManager, uint16_t port = 80);
  void begin();
  void handleClient();

private:

  static constexpr const char *kCalibrationPrefix = "/api/sensors/";
  static constexpr const char *kCalibrationSuffix = "/calibration";

  static constexpr uint32_t kRebootDelayMs = 500;
  static constexpr uint32_t kRebootFinalDelayMs = 50;

  WebServer server_;
  SensorManager &sensorManager_;
  DeviceService deviceService_;

  void registerRoutes();
  void handleRoot();
  void handleHealthz();
  void handleJson();
  void handleMetrics();
  void handleDeviceInfo();
  void handleProvision();
  void handleSetHostname();
  void handleReboot();
  void handleDynamicCalibrationRoute(); 

  void handleGetCalibration(SensorType);
  void processSetCalibration(SensorType st, const char * field);
  bool extractCalibrationPath(const String& uri, String& sensor, String& field);
  bool extractSensorCalibrationPath(const String& uri, String& sensor);
};