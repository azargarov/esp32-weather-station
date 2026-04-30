#include "wifi_manager.h"
#include "config.h"
#include "device_identity.h"
#include "device_state.h"

#include <ESPmDNS.h>

void WiFiManager::connect() {
  Serial.println();
  Serial.print(F("Connecting to WiFi: "));
  Serial.println(Config::WIFI_SSID);

  WiFi.mode(WIFI_STA);

  String hostname = DeviceIdentity::getEffectiveHostname();

  Serial.print(F("[wifi] applying hostname: "));
  Serial.println(hostname);

  WiFi.setHostname(hostname.c_str());
  applyStrongRadioSettings();

  WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);

  uint32_t retries = 0;
  while (WiFi.status() != WL_CONNECTED &&
         retries < Config::WIFI_CONNECT_MAX_RETRIES) {
    delay(Config::WIFI_CONNECT_RETRY_DELAY_MS);
    Serial.print(".");
    retries++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("Connected"));
    startMdns();
    printNetworkInfoToSerial(true);
    wasConnected_ = true;
  } else {
    wasConnected_ = false;
    Serial.print(F("WiFi connect failed, status="));
    Serial.println(wifiStatusToString(WiFi.status()));
  }
}

void WiFiManager::ensureConnected() {
  const wl_status_t currentStatus = WiFi.status();
  const bool currentlyConnected = (currentStatus == WL_CONNECTED);

  // Detect reconnection: was disconnected, now connected
  if (currentlyConnected && !wasConnected_) {
    wasConnected_ = true;
    Serial.println(F("[wifi] reconnected"));
    startMdns();
    printNetworkInfoToSerial(true);
    return;
  }

  if (currentlyConnected) {
    return;
  }

  if (wasConnected_) {
    wasConnected_ = false;
    Serial.println(F("[wifi] connection lost"));
    MDNS.end();
  }

  unsigned long now = millis();
  if (now - lastReconnectAttempt_ < Config::WIFI_RECONNECT_INTERVAL_MS) {
    return;
  }

  lastReconnectAttempt_ = now;
  Serial.println(F("[wifi] retrying, status="));
  Serial.println(wifiStatusToString(currentStatus));

  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_STA);
  applyStrongRadioSettings();

  String hostname = DeviceIdentity::getEffectiveHostname();
  WiFi.setHostname(hostname.c_str());
  WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);
}

void WiFiManager::printNetworkInfoToSerial(bool force) {
  if (WiFi.status() != WL_CONNECTED) {
    if (force) {
      Serial.println(F("[net] WiFi not connected"));
    }
    return;
  }

  String currentIp = WiFi.localIP().toString();

  if (force || currentIp != lastPrintedIp_) {
    Serial.println();
    Serial.println(F("=== ESP32 network info ==="));
    Serial.print(F("SSID: "));
    Serial.println(Config::WIFI_SSID);
    Serial.print(F("IP: "));
    Serial.println(currentIp);
    Serial.print(F("RSSI: "));
    Serial.print(WiFi.RSSI());
    Serial.println(F(" dBm"));
    Serial.print(F("Hostname: "));
    Serial.println(WiFi.getHostname());
    Serial.print(F("mDNS: http://"));
    Serial.print(DeviceIdentity::getEffectiveHostname());
    Serial.println(F(".local"));
    Serial.println(F("========================="));
    Serial.println();

    lastPrintedIp_ = currentIp;
  }
}

void WiFiManager::handleSerialInput() {
  if (!Serial.available()) {
    return;
  }

  char c = Serial.read();
  if (c == 'i' || c == 'I') {
    printNetworkInfoToSerial(true);
  }
}

bool WiFiManager::isConnected() const { return WiFi.status() == WL_CONNECTED; }

String WiFiManager::ip() const {
  return isConnected() ? WiFi.localIP().toString() : "";
}

int32_t WiFiManager::rssi() const { return isConnected() ? WiFi.RSSI() : 0; }

wl_status_t WiFiManager::status() const { return WiFi.status(); }

String WiFiManager::statusString() const {
  return wifiStatusToString(WiFi.status());
}

void WiFiManager::applyNormalRadioSettings() {
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_11dBm);
}

void WiFiManager::applyStrongRadioSettings() {
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
}

void WiFiManager::startMdns() {
  String hostname = DeviceIdentity::getEffectiveHostname();

  Serial.print(F("[mdns] using hostname: "));
  Serial.println(hostname);

  if (hostname.isEmpty()) {
    Serial.println(F("[mdns] hostname is empty, skipping mDNS"));
    return;
  }

  if (MDNS.begin(hostname.c_str())) {
    MDNS.addService("http", "tcp", 80);
    Serial.print(F("[mdns] started: http://"));
    Serial.print(hostname);
    Serial.println(F(".local"));
  } else {
    Serial.println(F("[mdns] failed to start"));
  }
}