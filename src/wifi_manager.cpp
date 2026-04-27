#include "wifi_manager.h"
#include "config.h"
#include "device_state.h"
#include "device_identity.h"

#include <ESPmDNS.h>

void WiFiManager::connect() {
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(Config::WIFI_SSID);

  WiFi.mode(WIFI_STA);
  

  String hostname = DeviceIdentity::getEffectiveHostname();
  
  Serial.print("[wifi] applying hostname: ");
  Serial.println(hostname);
  
  WiFi.setHostname(hostname.c_str());

  WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);
  applyStrongRadioSettings();

  uint32_t retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < Config::WIFI_CONNECT_MAX_RETRIES) {
    delay(Config::WIFI_CONNECT_RETRY_DELAY_MS);
    Serial.print(".");
    retries++;

  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected");
    startMdns();
    printNetworkInfoToSerial(true);
  } else {
    Serial.print("WiFi connect failed, status=");
    Serial.println(wifiStatusToString(WiFi.status()));
  }
  wasConnected_ = true;
}

void WiFiManager::ensureConnected() {
  const wl_status_t currentStatus = WiFi.status();
  const bool currentlyConnected = (currentStatus == WL_CONNECTED);

  // Detect reconnection: was disconnected, now connected
  if (currentlyConnected && !wasConnected_) {
    wasConnected_ = true;
    Serial.println("[wifi] reconnected");
    startMdns();
    printNetworkInfoToSerial(true);
    return;
  }

  if (currentlyConnected) {
    return;
  }

  if (wasConnected_) {
    wasConnected_ = false;
    Serial.println("[wifi] connection lost");
    MDNS.end();
  }

  unsigned long now = millis();
  if (now - lastReconnectAttempt_ < Config::WIFI_RECONNECT_INTERVAL_MS) {
    return;
  }

  lastReconnectAttempt_ = now;
  Serial.println("[wifi] retrying...");

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);

  String hostname = DeviceIdentity::getEffectiveHostname();
  WiFi.setHostname(hostname.c_str());
  WiFi.begin(Config::WIFI_SSID, Config::WIFI_PASSWORD);
}

void WiFiManager::printNetworkInfoToSerial(bool force) {
  if (WiFi.status() != WL_CONNECTED) {
    if (force) {
      Serial.println("[net] WiFi not connected");
    }
    return;
  }

  String currentIp = WiFi.localIP().toString();

  if (force || currentIp != lastPrintedIp_) {
    Serial.println();
    Serial.println("=== ESP32 network info ===");
    Serial.print("SSID: ");
    Serial.println(Config::WIFI_SSID);
    Serial.print("IP: ");
    Serial.println(currentIp);
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("Hostname: ");
    Serial.println(WiFi.getHostname());
    Serial.print("mDNS: http://");
    Serial.print(DeviceIdentity::getEffectiveHostname());
    Serial.println(".local");
    Serial.println("=========================");
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

bool WiFiManager::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::ip() const {
  return isConnected() ? WiFi.localIP().toString() : "";
}

int32_t WiFiManager::rssi() const {
  return isConnected() ? WiFi.RSSI() : 0;
}

wl_status_t WiFiManager::status() const {
  return WiFi.status();
}

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
  
    Serial.print("[mdns] using hostname: ");
    Serial.println(hostname);
  
    if (hostname.isEmpty()) {
    Serial.println("[mdns] hostname is empty, skipping mDNS");
    return;
  }

  if (MDNS.begin(hostname.c_str())) {
    MDNS.addService("http", "tcp", 80);
    Serial.print("[mdns] started: http://");
    Serial.print(hostname);
    Serial.println(".local");
  } else {
    Serial.println("[mdns] failed to start");
  }
}