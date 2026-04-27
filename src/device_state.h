#pragma once

#include <Arduino.h>
#include <WiFi.h>

struct DeviceState {
  bool wifiConnected;
  String ip;
  String wifiStatus;
  uint32_t uptimeSec;
  uint32_t freeHeapBytes;
  int32_t wifiRssiDbm;
};

String wifiStatusToString(wl_status_t status);
DeviceState collectDeviceState();
