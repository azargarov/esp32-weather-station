#pragma once

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
  void connect();
  void ensureConnected();
  void printNetworkInfoToSerial(bool force = false);
  void handleSerialInput();

  bool isConnected() const;
  String ip() const;
  int32_t rssi() const;
  wl_status_t status() const;
  String statusString() const;

private:
  void startMdns();
  unsigned long lastReconnectAttempt_ = 0;
  void applyNormalRadioSettings();
  void applyStrongRadioSettings();
  String lastPrintedIp_;
  bool wasConnected_ = false;
};
