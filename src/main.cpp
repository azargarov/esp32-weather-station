#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "http_server.h"
#include "sensors/sensor_manager.h"
#include "device_identity.h"

WiFiManager wifiManager;
SensorManager sensorManager;
HttpServer httpServer(sensorManager, 80);

void setup() {
  DeviceIdentity::begin();
  setCpuFrequencyMhz(Config::CPU_FREQ_MHZ);

  Serial.begin(Config::SERIAL_BAUD);
  delay(1000);

  Serial.println();
  Serial.println("Booting...");

  sensorManager.begin();
  wifiManager.connect();
  httpServer.begin();

  Serial.println("HTTP server started");
  wifiManager.printNetworkInfoToSerial(true);

  Serial.println();
  Serial.println("=== ESP32 startup info ===");
  Serial.print("Hardware ID: ");
  Serial.println(DeviceIdentity::getHardwareId());
  Serial.print("Provisioned ID: ");
  if (DeviceIdentity::hasProvisionedId()) {
    Serial.println(DeviceIdentity::getProvisionedId());
  } else {
    Serial.println("(not set)");
  }
  Serial.print("Effective ID: ");
  Serial.println(DeviceIdentity::getEffectiveId());
  Serial.println("End of booting");
  Serial.println("=========================");
}

void loop() {
  sensorManager.update();
  httpServer.handleClient();
  wifiManager.ensureConnected();
  wifiManager.handleSerialInput();
  delay(Config::LOOP_DELAY_MS);
}