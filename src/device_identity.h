#pragma once

#include <Arduino.h>

namespace DeviceIdentity {

void begin();

String getHardwareId();
String getProvisionedId();
String getEffectiveId();

String getHostname();
String getEffectiveHostname();

String getPlacement();
bool getReference();
String getReferenceLabel();

bool hasProvisionedId();
bool hasHostname();

bool setProvisionedId(const String &id);
bool setHostname(const String &hostname);

bool setPlacement(const String &placement);
bool setReference(bool  ref);

bool clearProvisionedId();
bool clearHostname();

bool isValidDeviceId(const String &id);
bool isValidHostname(const String &hostname);
bool isValidPlacement(const String &placement);
// void fillDeviceStatusJson(JsonDocument& doc);

} // namespace DeviceIdentity
