#include "config.h"
#include "device_identity.h"
#include "http_server.h"
#include "sensors/sensor_manager.h"
#include "wifi_manager.h"
#include <Arduino.h>

WiFiManager wifiManager;
SensorManager sensorManager;
HttpServer httpServer(sensorManager, 80);

void setup() {
  DeviceIdentity::begin();
  setCpuFrequencyMhz(Config::CPU_FREQ_MHZ);

  Serial.begin(Config::SERIAL_BAUD);
  delay(1000);

  Serial.println();
  Serial.println(F("Booting..."));

  sensorManager.begin();
  wifiManager.connect();
  httpServer.begin();
  httpServer.updateMetricsCache();

  Serial.println(F("HTTP server started"));
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
  httpServer.handleClient();

  sensorManager.update();

  static uint32_t lastMetricsCacheUpdateMs = 0;
  const uint32_t now = millis();

  if (now - lastMetricsCacheUpdateMs >=
      Config::METRICS_CACHE_UPDATE_INTERVAL_MS) {
    lastMetricsCacheUpdateMs = now;
    httpServer.updateMetricsCache();
  }

  wifiManager.ensureConnected();
  wifiManager.handleSerialInput();

  delay(Config::LOOP_DELAY_MS);
}
