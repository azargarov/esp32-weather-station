#include "device_state.h"

String wifiStatusToString(wl_status_t status) {
  switch (status) {
  case WL_NO_SHIELD:
    return "no_shield";
  case WL_IDLE_STATUS:
    return "idle";
  case WL_NO_SSID_AVAIL:
    return "no_ssid";
  case WL_SCAN_COMPLETED:
    return "scan_completed";
  case WL_CONNECTED:
    return "connected";
  case WL_CONNECT_FAILED:
    return "connect_failed";
  case WL_CONNECTION_LOST:
    return "connection_lost";
  case WL_DISCONNECTED:
    return "disconnected";
  default:
    return "unknown";
  }
}

void collectDeviceState(DeviceState &state) {
  state.wifiConnected = (WiFi.status() == WL_CONNECTED);
  state.ip = state.wifiConnected ? WiFi.localIP().toString() : "";
  state.wifiStatus = wifiStatusToString(WiFi.status());
  state.uptimeSec = millis() / 1000;
  state.freeHeapBytes = ESP.getFreeHeap();
  state.wifiRssiDbm = state.wifiConnected ? WiFi.RSSI() : 0;
}
