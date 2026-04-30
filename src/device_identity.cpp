#include "device_identity.h"
#include "metrics_formatter.h"
#include <Preferences.h>
#include <esp_system.h>

namespace DeviceIdentity {

namespace {
Preferences prefs;
bool initialized = false;

constexpr const char *kNamespace = "device";
constexpr const char *kKeyDeviceId = "id";
constexpr const char *kKeyHostname = "hostname";

String cachedHardwareId;
String cachedProvisionedId;
String cachedHostname;

void ensureInitialized() {
  if (initialized) {
    return;
  }

  prefs.begin(kNamespace, false);

  uint64_t chipid = ESP.getEfuseMac();

  char buf[24];
  snprintf(buf, sizeof(buf), "esp32-%04X%08X",
           static_cast<uint16_t>(chipid >> 32), static_cast<uint32_t>(chipid));

  cachedHardwareId = String(buf);

  if (prefs.isKey(kKeyDeviceId)) {
    cachedProvisionedId = prefs.getString(kKeyDeviceId, "");
  } else {
    cachedProvisionedId = "";
  }

  if (prefs.isKey(kKeyHostname)) {
    cachedHostname = prefs.getString(kKeyHostname, "");
  } else {
    cachedHostname = "";
  }

  initialized = true;
}

} // namespace

void begin() { 
  ensureInitialized(); 
  initMetricsFormatter();
}

String getHardwareId() {
  ensureInitialized();
  return cachedHardwareId;
}

String getProvisionedId() {
  ensureInitialized();
  return cachedProvisionedId;
}

String getEffectiveId() {
  ensureInitialized();
  return cachedProvisionedId.isEmpty() ? cachedHardwareId : cachedProvisionedId;
}

String getHostname() {
  ensureInitialized();
  return cachedHostname;
}

String getEffectiveHostname() {
  ensureInitialized();
  return cachedHostname.isEmpty() ? getEffectiveId() : cachedHostname;
}

bool hasProvisionedId() {
  ensureInitialized();
  return !cachedProvisionedId.isEmpty();
}

bool hasHostname() {
  ensureInitialized();
  return !cachedHostname.isEmpty();
}

bool isValidDeviceId(const String &id) {
  if (id.isEmpty() || id.length() > 32) {
    return false;
  }

  for (size_t i = 0; i < id.length(); ++i) {
    char c = id[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_';

    if (!ok) {
      return false;
    }
  }

  return true;
}

bool isValidHostname(const String &hostname) {
  if (hostname.isEmpty() || hostname.length() > 32) {
    return false;
  }

  if (hostname[0] == '-' || hostname[hostname.length() - 1] == '-') {
    return false;
  }

  for (size_t i = 0; i < hostname.length(); ++i) {
    char c = hostname[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-';

    if (!ok) {
      return false;
    }
  }

  return true;
}

bool setProvisionedId(const String &id) {
  ensureInitialized();

  if (!isValidDeviceId(id)) {
    return false;
  }

  if (prefs.putString(kKeyDeviceId, id) == 0) {
    return false;
  }

  cachedProvisionedId = id;
  return true;
}

bool setHostname(const String &hostname) {
  ensureInitialized();

  if (!isValidHostname(hostname)) {
    return false;
  }

  if (prefs.putString(kKeyHostname, hostname) == 0) {
    return false;
  }

  cachedHostname = hostname;
  return true;
}

bool clearProvisionedId() {
  ensureInitialized();

  if (!prefs.remove(kKeyDeviceId)) {
    return false;
  }

  cachedProvisionedId = "";
  return true;
}

bool clearHostname() {
  ensureInitialized();

  if (!prefs.remove(kKeyHostname)) {
    return false;
  }

  cachedHostname = "";
  return true;
}

} // namespace DeviceIdentity