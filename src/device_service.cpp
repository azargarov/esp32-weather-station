#include "device_service.h"

#include "device_identity.h"
#include "device_state.h"
#include "metrics_formatter.h"
#include <Arduino.h>

namespace {
constexpr uint32_t kRebootDelayMs = 500;
constexpr uint32_t kRestartFlushDelayMs = 50;

struct SensorMetricFormatContext {
  String *out;
  String *nameBuffer;
  String *labelsBuffer;
};

void appendSensorMetricCallback(const SensorMetric &metric, void *context) {
  auto *ctx = static_cast<SensorMetricFormatContext *>(context);

  appendSensorMetric(*ctx->out, *ctx->nameBuffer, *ctx->labelsBuffer, metric);
}

void fillIdentityJson(JsonDocument &doc) {
  doc["device_id"] = DeviceIdentity::getProvisionedId();
  doc["hardware_id"] = DeviceIdentity::getHardwareId();
  doc["effective_id"] = DeviceIdentity::getEffectiveId();
  doc["hostname"] = DeviceIdentity::getHostname();
  doc["effective_hostname"] = DeviceIdentity::getEffectiveHostname();
  doc["provisioned"] = DeviceIdentity::hasProvisionedId();
}

DeviceService::Result ok(uint16_t statusCode = 200) {
  return {true, statusCode, nullptr};
}

DeviceService::Result fail(uint16_t statusCode, const char *error) {
  return {false, statusCode, error};
}

void fillStatusJson(JsonDocument &doc, const DeviceState &state) {
  doc["status"] = "ok";
  doc["ip"] = state.ip;
  doc["uptime_sec"] = state.uptimeSec;
  doc["heap_free"] = state.freeHeapBytes;
  doc["wifi_rssi"] = state.wifiRssiDbm;
  doc["wifi_connected"] = state.wifiConnected;
  doc["wifi_status"] = state.wifiStatus;
}

void fillSensorJson(JsonObject sensors, const SensorManager &sensorManager) {
  const SensorSnapshot snapshot = sensorManager.snapshot();
  sensors["bme280_available"] = snapshot.bme280Available;
  sensors["bme280_read_ok"] = snapshot.bme280ReadOk;

  JsonObject fields = sensors["fields"].to<JsonObject>();

  sensorManager.walkFields(
      [&fields](const char *key, float value, const char *unit) {
        JsonObject entry = fields[key].to<JsonObject>();
        entry["value"] = value;
        entry["unit"] = unit;
      });
}

void appendSensorText(String &out, const SensorManager &sensorManager) {
  out.reserve(out.length() + 300);

  bool hasFields = false;
  char lineBuffer[80];

  sensorManager.walkFields([&](const char *key, float value, const char *unit) {
    if (!hasFields) {
      out += "\n--- Sensors ---\n";
      hasFields = true;
    }

    snprintf(lineBuffer, sizeof(lineBuffer), "%-15s: %.2f %s\n", key, value,
             unit);
    out += lineBuffer;
  });

  if (!hasFields) {
    out += "\nSensors: (no data available)\n";
  }
}

void fillProvisioningResponse(JsonDocument &doc) {
  Serial.print("[provision] device_id set to: ");
  Serial.println(DeviceIdentity::getProvisionedId());

  Serial.print("[provision] hostname set to: ");
  Serial.println(DeviceIdentity::getHostname());

  Serial.println("[provision] reboot required for WiFi hostname/mDNS change");

  fillIdentityJson(doc);
  doc["status"] = "ok";
  doc["note"] = "reboot_required_for_hostname_change";
}

void fillHostnameResponse(JsonDocument &doc) {
  Serial.print("[hostname] hostname set to: ");
  Serial.println(DeviceIdentity::getHostname());

  Serial.println("[hostname] reboot required for WiFi hostname/mDNS change");

  doc["hostname"] = DeviceIdentity::getHostname();
  doc["status"] = "ok";
  doc["note"] = "reboot_required_for_hostname_change";
}

} // namespace

DeviceService::DeviceService(SensorManager& sensorManager, BootInfo bootInfo)
      : sensorManager_(sensorManager), bootInfo_(bootInfo) {
  cachedMetrics_.reserve(2600);
  metricNameBuffer_.reserve(64);
  metricLabelsBuffer_.reserve(128);
}

const String &DeviceService::getMetrics() const { return cachedMetrics_; }

void DeviceService::updateMetricsCache() {
  const uint32_t startMs = millis();

  DeviceState state;
  collectDeviceState(state);

  cachedMetrics_ = "";

  formatDeviceMetrics(cachedMetrics_, state, metricsBuildDurationMs_,
                      metricsLastBuildUptimeSeconds_);

  SensorMetricFormatContext ctx{&cachedMetrics_, &metricNameBuffer_,
                                &metricLabelsBuffer_};

  sensorManager_.walkMetrics(appendSensorMetricCallback, &ctx);

  metricsBuildDurationMs_ = millis() - startMs;
  metricsLastBuildUptimeSeconds_ = millis() / 1000;
}

String DeviceService::getTextStatus() {
  DeviceState state;
  collectDeviceState(state);

  const String hwId = DeviceIdentity::getHardwareId();
  const String effId = DeviceIdentity::getEffectiveId();
  const String hostname = DeviceIdentity::getEffectiveHostname();

  String body;
  body.reserve(400);

  body += "ESP32 is alive\n";
  body += "Hardware ID:    ";
  body += hwId;
  body += '\n';
  body += "Provisioned ID: ";
  if (DeviceIdentity::hasProvisionedId()) {
    body += DeviceIdentity::getProvisionedId();
  } else {
    body += "(not set)";
  }
  body += '\n';
  body += "Effective ID:   ";
  body += effId;
  body += '\n';
  body += "Hostname:       ";
  body += hostname;
  body += '\n';
  body += "IP:             ";
  body += state.wifiConnected ? state.ip : "n/a";
  body += '\n';
  body += "Uptime:         ";
  body += state.uptimeSec;
  body += " sec\n\n";

  appendSensorText(body, sensorManager_);
  return body;
}

void DeviceService::getJSONStatus(JsonDocument &doc) {
  DeviceState state;
  collectDeviceState(state);

  fillIdentityJson(doc);
  fillStatusJson(doc, state);
  fillSensorJson(doc["sensors"].to<JsonObject>(), sensorManager_);
}
void DeviceService::getDeviceInfo(JsonDocument &doc) {
  DeviceState state;
  collectDeviceState(state);

  fillIdentityJson(doc);
  doc["ip"] = state.wifiConnected ? state.ip : "n/a";
  doc["reset_reason"] = resetReasonToString(bootInfo_.resetReason);
  doc["reset_reason_code"] = static_cast<int>(bootInfo_.resetReason);
}

DeviceService::Result DeviceService::provisionDevice(const String &newId,
                                                     const String &newHostname,
                                                     JsonDocument &response) {
  if (newId.isEmpty()) {
    return fail(400, "missing_device_id");
  }

  if (!DeviceIdentity::isValidDeviceId(newId)) {
    return fail(400, "invalid_device_id");
  }

  if (DeviceIdentity::hasProvisionedId()) {
    return fail(409, "already_provisioned");
  }

  if (newHostname.isEmpty()) {
    return fail(400, "missing_hostname");
  }

  if (!DeviceIdentity::isValidHostname(newHostname)) {
    return fail(400, "invalid_hostname");
  }

  if (!DeviceIdentity::setProvisionedId(newId)) {
    return fail(500, "failed_to_store_device_id");
  }

  if (!DeviceIdentity::setHostname(newHostname)) {
    return fail(500, "failed_to_store_hostname");
  }

  fillProvisioningResponse(response);
  return ok(200);
}

DeviceService::Result DeviceService::setHostname(const String &newHostname,
                                                 JsonDocument &response) {
  if (newHostname.isEmpty()) {
    return fail(400, "missing_hostname");
  }

  if (!DeviceIdentity::isValidHostname(newHostname)) {
    return fail(400, "invalid_hostname");
  }

  if (!DeviceIdentity::setHostname(newHostname)) {
    return fail(500, "failed_to_store_hostname");
  }

  fillHostnameResponse(response);
  return ok(200);
}

DeviceService::Result DeviceService::requestReboot(JsonDocument &response) {
  if (rebootRequested_) {
    response["status"] = "ok";
    response["message"] = "reboot_already_scheduled";
    return {true, 202, nullptr};
  }

  Serial.println("[device] reboot requested");

  rebootRequested_ = true;
  rebootAtMs_ = millis() + kRebootDelayMs;

  response["status"] = "ok";
  response["message"] = "reboot_scheduled";

  return {true, 202, nullptr};
}

void DeviceService::handlePendingReboot() {
  if (!rebootRequested_) {
    return;
  }

  if (static_cast<long>(millis() - rebootAtMs_) < 0) {
    return;
  }

  Serial.println("[device] rebooting now...");
  delay(kRestartFlushDelayMs);
  ESP.restart();
}
