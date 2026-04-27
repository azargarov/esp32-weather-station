#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

namespace DeviceIdentity {

void begin();

String getHardwareId();
String getProvisionedId();
String getEffectiveId();

String getHostname();
String getEffectiveHostname();

bool hasProvisionedId();
bool hasHostname();

bool setProvisionedId(const String &id);
bool setHostname(const String &hostname);

bool clearProvisionedId();
bool clearHostname();

bool isValidDeviceId(const String &id);
bool isValidHostname(const String &hostname);

// void fillDeviceStatusJson(JsonDocument& doc);

} // namespace DeviceIdentity