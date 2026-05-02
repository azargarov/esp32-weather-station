#include "config.h"
#include "device_identity.h"
#include "http_server.h"
#include "sensors/sensor_manager.h"
#include "wifi_manager.h"
#include "device_state.h"
#include <Arduino.h>

namespace{
  WiFiManager wifiManager;
  SensorManager sensorManager;
  //DeviceService* deviceService = nullptr;
  HttpServer* httpServer = nullptr;
}

void setup() {
  DeviceIdentity::begin();
  setCpuFrequencyMhz(Config::CPU_FREQ_MHZ);

  Serial.begin(Config::SERIAL_BAUD);
  delay(1000);

  Serial.println();
  Serial.println(F("Booting..."));

  BootInfo bootInfo{readResetReason()};

  Serial.print(F("[boot] reset reason: "));
  Serial.println(resetReasonToString(bootInfo.resetReason));

  static DeviceService deviceServiceInstance(sensorManager, bootInfo);
  static HttpServer httpServerInstance(deviceServiceInstance, sensorManager, 80);

 // deviceService = &deviceServiceInstance;
  httpServer = &httpServerInstance;

  sensorManager.begin();
  wifiManager.connect();

  httpServer->begin();
  httpServer->updateMetricsCache();

  Serial.println(F("[boot] HTTP server started"));
  wifiManager.printNetworkInfoToSerial(true);

  Serial.println();
  Serial.println(F("=== ESP32 startup info ==="));
  Serial.print(F("Hardware ID: "));
  Serial.println(DeviceIdentity::getHardwareId());
  Serial.print(F("Provisioned ID: "));
  if (DeviceIdentity::hasProvisionedId()) {
    Serial.println(DeviceIdentity::getProvisionedId());
  } else {
    Serial.println(F("(not set)"));
  }
  Serial.print(F("Effective ID: "));
  Serial.println(DeviceIdentity::getEffectiveId());
  Serial.println(F("End of booting"));
  Serial.println(F("========================="));
}

void loop() {
  wifiManager.ensureConnected();
  wifiManager.handleSerialInput();

  sensorManager.update();
  
  static uint32_t lastMetricsCacheUpdateMs = 0;
  const uint32_t now = millis();
  
  if (now - lastMetricsCacheUpdateMs >=
    Config::METRICS_CACHE_UPDATE_INTERVAL_MS) {
      lastMetricsCacheUpdateMs = now;
      httpServer->updateMetricsCache();
  }
  
  httpServer->handleClient();
  delay(Config::LOOP_DELAY_MS);
}
