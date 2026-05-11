#include "device_identity.h"
#include "metrics_formatter.h"
#include <Preferences.h>
#include <esp_system.h>

namespace DeviceIdentity {

namespace {
Preferences prefs;
bool initialized = false;
bool prefsReady = false;

constexpr const char *kNamespace = "device";
constexpr const char *kKeyDeviceId = "id";
constexpr const char *kKeyHostname = "hostname";
constexpr const char *kKeyPlacement = "placement";
constexpr const char *kKeyReference = "reference";

String cachedHardwareId;
String cachedProvisionedId;
String cachedHostname;
String cachedPlacement="unknown";
bool cachedReference=false;

void ensureInitialized() {
  if (initialized) {
    return;
  }

  uint64_t chipid = ESP.getEfuseMac();

  char buf[24];
  snprintf(buf, sizeof(buf), "esp32-%04X%08X",
           static_cast<uint16_t>(chipid >> 32),
           static_cast<uint32_t>(chipid));

  cachedHardwareId = String(buf);

  prefsReady = prefs.begin(kNamespace, false);
  if (prefsReady) {
    cachedProvisionedId = prefs.getString(kKeyDeviceId, "");
    cachedHostname = prefs.getString(kKeyHostname, "");
    cachedPlacement = prefs.getString(kKeyPlacement, "unknown");
    cachedReference = prefs.getBool(kKeyReference, false);
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
  return cachedHostname.isEmpty() ? 
       (cachedProvisionedId.isEmpty() ? cachedHardwareId : cachedProvisionedId)
       : cachedHostname;
}

String getPlacement(){
  ensureInitialized();
  return cachedPlacement;
}

bool getReference(){
  ensureInitialized();
  return cachedReference;
}

String getReferenceLabel() {
  ensureInitialized();
  return cachedReference ? "true" : "false";
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

bool isValidPlacement(const String &placement) {
  return placement == "indoor" ||
         placement == "outdoor" ||
         placement == "unknown";
}

bool setProvisionedId(const String &id) {
  ensureInitialized();

  if (!prefsReady) {
    return false;
  }
  
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

  if (!prefsReady) {
    return false;
  }

  if (!isValidHostname(hostname)) {
    return false;
  }

  if (prefs.putString(kKeyHostname, hostname) == 0) {
    return false;
  }

  cachedHostname = hostname;
  return true;
}

bool setPlacement(const String &placement){
  ensureInitialized();
  
  if (!prefsReady) {
    return false;
  }

  if (!isValidPlacement(placement)) {
    return false;
  }

  if (prefs.putString(kKeyPlacement, placement) == 0) {
    return false;
  }
  cachedPlacement = placement;
  return true;
}

bool setReference(bool  ref){
  ensureInitialized();

  if (!prefsReady) {
    return false;
  }
  
  if (!prefs.putBool(kKeyReference, ref)) {
    return false;
  }

  cachedReference = ref;
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

bool clearPlacement() {
  ensureInitialized();

  if (!prefs.remove(kKeyPlacement)) {
    return false;
  }

  cachedPlacement = "unknown";
  return true;
}

bool clearReference() {
  ensureInitialized();

  if (!prefs.remove(kKeyReference)) {
    return false;
  }

  cachedReference = false;
  return true;
}

} // namespace DeviceIdentity
