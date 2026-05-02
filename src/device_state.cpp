#include "device_state.h"

const char* resetReasonToString(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:   return "poweron";
    case ESP_RST_EXT:       return "external";
    case ESP_RST_SW:        return "software";
    case ESP_RST_PANIC:     return "panic";
    case ESP_RST_INT_WDT:   return "int_wdt";
    case ESP_RST_TASK_WDT:  return "task_wdt";
    case ESP_RST_WDT:       return "other_wdt";
    case ESP_RST_DEEPSLEEP: return "deepsleep";
    case ESP_RST_BROWNOUT:  return "brownout";
    case ESP_RST_SDIO:      return "sdio";
    default:                return "unknown";
  }
}

esp_reset_reason_t readResetReason(){
  return esp_reset_reason();
}

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

