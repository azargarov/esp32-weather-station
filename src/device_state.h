#pragma once

#include <Arduino.h>
#include <WiFi.h>

const char* resetReasonToString(esp_reset_reason_t reason);
esp_reset_reason_t readResetReason();

struct DeviceState {
  bool wifiConnected;
  String ip;
  String wifiStatus;
  uint32_t uptimeSec;
  uint32_t freeHeapBytes;
  int32_t wifiRssiDbm;
};

String wifiStatusToString(wl_status_t status);
void collectDeviceState(DeviceState &state);
