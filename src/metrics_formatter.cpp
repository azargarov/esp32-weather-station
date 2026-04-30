#include "metrics_formatter.h"
#include "device_identity.h"

namespace {

String cachedBaseLabels;
bool formatterInitialized = false;

void appendEscapedLabelValue(String& out, const char* s) {
  if (!s) {
    return;
  }

  for (const char* p = s; *p != '\0'; ++p) {
    switch (*p) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      default:
        out += *p;
        break;
    }
  }
}

void appendEscapedLabelValue(String& out, const String& s) {
  appendEscapedLabelValue(out, s.c_str());
}

void rebuildBaseLabels() {
  cachedBaseLabels = "device_id=\"";
  appendEscapedLabelValue(cachedBaseLabels, DeviceIdentity::getEffectiveId());
  cachedBaseLabels += "\",hardware_id=\"";
  appendEscapedLabelValue(cachedBaseLabels, DeviceIdentity::getHardwareId());
  cachedBaseLabels += "\"";

  formatterInitialized = true;
}

const String& getBaseLabels() {
  if (!formatterInitialized) {
    rebuildBaseLabels();
  }
  return cachedBaseLabels;
}

void appendMetricHeader(String& out, const char* name, const char* help) {
  out += "# HELP ";
  out += name;
  out += ' ';
  out += help;
  out += '\n';

  out += "# TYPE ";
  out += name;
  out += " gauge\n";
}

void appendSample(String& out, const char* name,
                  const String& labels, const String& value) {
  out += name;
  out += '{';
  out += labels;
  out += "} ";
  out += value;
  out += '\n';
}

void appendSample(String& out, const char* name,
                  const String& labels, const char* value) {
  appendSample(out, name, labels, String(value));
}

void appendSample(String& out, const char* name,
                  const String& labels, bool value) {
  appendSample(out, name, labels, value ? "1" : "0");
}

void appendSample(String& out, const char* name,
                  const String& labels, int32_t value) {
  appendSample(out, name, labels, String(value));
}

void appendSample(String& out, const char* name,
                  const String& labels, uint32_t value) {
  appendSample(out, name, labels, String(value));
}

void appendSample(String& out, const char* name,
                  const String& labels, float value, int decimals = 2) {
  appendSample(out, name, labels, String(value, decimals));
}

void appendInfoSample(String& out, const char* name, const String& baseLabels,
                      const char* labelKey, const String& labelValue) {
  String labels = baseLabels;
  labels.reserve(baseLabels.length() + strlen(labelKey) + labelValue.length() + 8);

  labels += ",";
  labels += labelKey;
  labels += "=\"";
  appendEscapedLabelValue(labels, labelValue);
  labels += "\"";

  appendSample(out, name, labels, "1");
}

void appendSensorSample(String& out, const String& baseLabels,
                        const char* field, const char* unit, float value) {
  String labels = baseLabels;
  labels.reserve(baseLabels.length() + strlen(field) + strlen(unit) + 24);

  labels += ",field=\"";
  appendEscapedLabelValue(labels, field);
  labels += "\",unit=\"";
  appendEscapedLabelValue(labels, unit);
  labels += "\"";

  appendSample(out, "esp32_sensor_value", labels, value);
}

}  // namespace

void initMetricsFormatter() {
  rebuildBaseLabels();
}

String formatPrometheusMetrics(const DeviceState& state,
                               const SensorManager& sensorManager) {
  String metrics;
  metrics.reserve(2400);

  const String& baseLabels = getBaseLabels();
  const SensorSnapshot sensors = sensorManager.snapshot();

  appendMetricHeader(metrics, "esp32_up",
                     "Whether the ESP32 application is running.");
  appendSample(metrics, "esp32_up", baseLabels, true);

  appendMetricHeader(metrics, "esp32_wifi_connected",
                     "Whether WiFi is connected.");
  appendSample(metrics, "esp32_wifi_connected", baseLabels, state.wifiConnected);

  appendMetricHeader(metrics, "esp32_uptime_seconds",
                     "Time since boot in seconds.");
  appendSample(metrics, "esp32_uptime_seconds", baseLabels, state.uptimeSec);

  appendMetricHeader(metrics, "esp32_heap_free_bytes",
                     "Free heap in bytes.");
  appendSample(metrics, "esp32_heap_free_bytes", baseLabels, state.freeHeapBytes);

  appendMetricHeader(metrics, "esp32_wifi_rssi_dbm",
                     "WiFi RSSI in dBm.");
  appendSample(metrics, "esp32_wifi_rssi_dbm", baseLabels, state.wifiRssiDbm);

  appendMetricHeader(metrics, "esp32_wifi_status_info",
                     "WiFi status as an informational labeled metric.");
  appendInfoSample(metrics, "esp32_wifi_status_info", baseLabels,
                   "status", state.wifiStatus);

  appendMetricHeader(metrics, "esp32_bme280_available",
                     "Whether the BME280 sensor is available.");
  appendSample(metrics, "esp32_bme280_available",
               baseLabels, sensors.bme280Available);

  appendMetricHeader(metrics, "esp32_bme280_read_ok",
                     "Whether the last BME280 read succeeded.");
  appendSample(metrics, "esp32_bme280_read_ok",
               baseLabels, sensors.bme280ReadOk);

  appendMetricHeader(metrics, "esp32_sensor_value",
                     "Sensor measurement.");

  sensorManager.walkFields(
      [&metrics, &baseLabels](const char* key, float value, const char* unit) {
        appendSensorSample(metrics, baseLabels, key, unit, value);
      });

  return metrics;
}