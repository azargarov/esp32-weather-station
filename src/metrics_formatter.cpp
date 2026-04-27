#include "metrics_formatter.h"
#include "device_identity.h"

namespace {

String prometheusLabelEscape(const String& s) {
  String out;
  out.reserve(s.length() + 8);

  for (size_t i = 0; i < s.length(); ++i) {
    char c = s[i];
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '\"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      default: out += c; break;
    }
  }

  return out;
}

String baseLabels() {
  String labels;
  labels.reserve(96);

  labels += "device_id=\"";
  labels += prometheusLabelEscape(DeviceIdentity::getEffectiveId());
  labels += "\",hardware_id=\"";
  labels += prometheusLabelEscape(DeviceIdentity::getHardwareId());
  labels += "\"";

  return labels;
}

String sensorLabels(const String& baseLabels, const char* unit) {
  String labels = baseLabels;
  labels += ",unit=\"";
  labels += prometheusLabelEscape(String(unit));
  labels += "\"";
  return labels;
}

void appendGauge(String& out, const String& labels,
                 const char* name, const char* help,
                 const String& value) {
  out += "# HELP "; out += name; out += ' '; out += help; out += '\n';
  out += "# TYPE "; out += name; out += " gauge\n";
  out += name; out += '{'; out += labels; out += "} "; out += value; out += '\n';
}

void appendGauge(String& out, const String& labels,
                 const char* name, const char* help,
                 const char* value) {
  appendGauge(out, labels, name, help, String(value));
}

void appendGauge(String& out, const String& labels,
                 const char* name, const char* help,
                 int32_t value) {
  appendGauge(out, labels, name, help, String(value));
}

void appendGauge(String& out, const String& labels,
                 const char* name, const char* help,
                 uint32_t value) {
  appendGauge(out, labels, name, help, String(value));
}

void appendGauge(String& out, const String& labels,
                 const char* name, const char* help,
                 bool value) {
  appendGauge(out, labels, name, help, value ? "1" : "0");
}

void appendGauge(String& out, const String& labels,
                 const char* name, const char* help,
                 float value, int decimals = 2) {
  appendGauge(out, labels, name, help, String(value, decimals));
}

void appendInfoGauge(String& out, const String& labels,
                     const char* name, const char* help,
                     const char* labelKey, const String& labelVal) {
  String labeled = labels;
  labeled += ",";
  labeled += labelKey;
  labeled += "=\"";
  labeled += prometheusLabelEscape(labelVal);
  labeled += "\"";

  appendGauge(out, labeled, name, help, "1");
}

String sensorMetricName(const char* key) {
  String name = "esp32_sensor_";
  for (const char* p = key; *p != '\0'; ++p) {
    const char c = *p;
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_') {
      name += c;
    } else {
      name += '_';
    }
  }
  return name;
}

}  // namespace

String formatPrometheusMetrics(const DeviceState& state, const SensorManager& sensorManager) {
  String metrics;
  metrics.reserve(2600);

  const String labels = baseLabels();
  const SensorSnapshot sensors = sensorManager.snapshot();

  appendGauge(metrics, labels, "esp32_up",
              "Whether the ESP32 application is running.", true);

  appendGauge(metrics, labels, "esp32_wifi_connected",
              "Whether WiFi is connected.", state.wifiConnected);

  appendGauge(metrics, labels, "esp32_uptime_seconds",
              "Time since boot in seconds.", state.uptimeSec);

  appendGauge(metrics, labels, "esp32_heap_free_bytes",
              "Free heap in bytes.", state.freeHeapBytes);

  appendGauge(metrics, labels, "esp32_wifi_rssi_dbm",
              "WiFi RSSI in dBm.", state.wifiRssiDbm);

  appendInfoGauge(metrics, labels, "esp32_wifi_status_info",
                  "WiFi status as an informational labeled metric.",
                  "status", state.wifiStatus);

  appendGauge(metrics, labels, "esp32_bme280_available",
              "Whether the BME280 sensor is available.", sensors.bme280Available);

  appendGauge(metrics, labels, "esp32_bme280_read_ok",
              "Whether the last BME280 read succeeded.", sensors.bme280ReadOk);

  sensorManager.walkFields([&metrics, &labels](const char* key, float value, const char* unit) {
    const String name = sensorMetricName(key);
    const String labeled = sensorLabels(labels, unit);
    appendGauge(metrics, labeled, name.c_str(), "Sensor measurement.", value);
  });

  return metrics;
}
