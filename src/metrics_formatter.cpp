#include "metrics_formatter.h"
#include "device_identity.h"

namespace {

String cachedBaseLabels;

void appendEscapedLabelValue(String& out, const String& value) {
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    switch (c) {
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
        out += c;
        break;
    }
  }
}

void buildBaseLabels(String& out) {
  out = "device_id=\"";
  appendEscapedLabelValue(out, DeviceIdentity::getEffectiveId());
  out += "\",hardware_id=\"";
  appendEscapedLabelValue(out, DeviceIdentity::getHardwareId());
  out += "\"";
}

void sensorLabels(String& out, const String &baseLabels, const char *unit) {
  out = baseLabels;
  out += ",unit=\"";
  appendEscapedLabelValue(out, String(unit));
  out += "\"";
}

void appendGauge(String& out, const String& labels, const char* name,
                 const char* help, const char* value) {
  const size_t extra =
      8 + strlen(name) + 1 + strlen(help) + 1 +
      8 + strlen(name) + 7 +
      strlen(name) + 1 + labels.length() + 2 + strlen(value) + 1;

  out.reserve(out.length() + extra);

  out += F("# HELP ");
  out += name;
  out += ' ';
  out += help;
  out += '\n';
  out += F("# TYPE ");
  out += name;
  out += F(" gauge\n");
  out += name;
  out += '{';
  out += labels;
  out += F("} ");
  out += value;
  out += '\n';
}

void appendGauge(String& out, const String& labels, const char* name,
                 const char* help, const String& value) {
  appendGauge(out, labels, name, help, value.c_str());
}

void appendGauge(String& out, const String& labels, const char* name,
                 const char* help, int32_t value) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%ld", static_cast<long>(value));
  appendGauge(out, labels, name, help, buf);
}

void appendGauge(String& out, const String& labels, const char* name,
                 const char* help, uint32_t value) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(value));
  appendGauge(out, labels, name, help, buf);
}

void appendGauge(String& out, const String& labels, const char* name,
                 const char* help, bool value) {
  appendGauge(out, labels, name, help, value ? "1" : "0");
}

void appendGauge(String& out, const String& labels, const char* name,
                 const char* help, float value, int decimals = 2) {
  char buf[32];
  dtostrf(value, 0, decimals, buf);

  char* start = buf;
  while (*start == ' ') {
    ++start;
  }

  appendGauge(out, labels, name, help, start);
}

void sensorMetricName(String& out, const char *key) {
  out = "esp32_sensor_";
  for (const char *p = key; *p != '\0'; ++p) {
    const char c = *p;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '_') {
      out += c;
    } else {
      out += '_';
    }
  }
}

void appendInfoGauge(String &out, const String &labels, const char *name,
                     const char *help, const char *labelKey,
                     const String &labelVal) {
  String labeled = labels;
  labeled += ",";
  labeled += labelKey;
  labeled += "=\"";
  appendEscapedLabelValue(labeled, labelVal);
  labeled += "\"";

  appendGauge(out, labeled, name, help, "1");
}

} // namespace

void initMetricsFormatter() {
  buildBaseLabels(cachedBaseLabels);
}

const String& getBaseLabels() {
  if (cachedBaseLabels.isEmpty()) {
    buildBaseLabels(cachedBaseLabels);
  }
  return cachedBaseLabels;
}

String formatPrometheusMetrics(const DeviceState &state,
                               const SensorManager &sensorManager) {
  String metrics;
  metrics.reserve(2600);

  const String& labels = getBaseLabels();
  const SensorSnapshot sensors = sensorManager.snapshot();

  appendGauge(metrics, labels, "esp32_up",
              "Whether the ESP32 application is running.", true);

  appendGauge(metrics, labels, "esp32_wifi_connected",
              "Whether WiFi is connected.", state.wifiConnected);

  appendGauge(metrics, labels, "esp32_uptime_seconds",
              "Time since boot in seconds.", state.uptimeSec);

  appendGauge(metrics, labels, "esp32_heap_free_bytes", "Free heap in bytes.",
              state.freeHeapBytes);

  appendGauge(metrics, labels, "esp32_wifi_rssi_dbm", "WiFi RSSI in dBm.",
              state.wifiRssiDbm);

  appendInfoGauge(metrics, labels, "esp32_wifi_status_info",
                  "WiFi status as an informational labeled metric.", "status",
                  state.wifiStatus);

  appendGauge(metrics, labels, "esp32_bme280_available",
              "Whether the BME280 sensor is available.",
              sensors.bme280Available);

  appendGauge(metrics, labels, "esp32_bme280_read_ok",
              "Whether the last BME280 read succeeded.", sensors.bme280ReadOk);

  sensorManager.walkFields([&metrics, &labels](const char *key, float value,
                                               const char *unit) {
    String name, labeled;
    sensorMetricName(name, key);
    sensorLabels(labeled, labels, unit);
    appendGauge(metrics, labeled, name.c_str(), "Sensor measurement.", value);
  });

  return metrics;
}
