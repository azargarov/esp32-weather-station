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
  uint32_t kRebootDelayMs = 500;
  uint32_t kRebootFinalDelayMs = 50;

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

  void handleGetCalibration(SensorType);
  void handleSetCalibration(SensorType st, const char* field);
};