#include "metrics_formatter.h"
#include "device_identity.h"

namespace {

String cachedBaseLabels;


void appendEscapedLabelValue(String &out, const char *value) {
  for (const char *p = value; *p != '\0'; ++p) {
    const char c = *p;
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

void appendEscapedLabelValue(String &out, const String &value) {
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

void sensorMetricLabels(String &out, const String &baseLabels,
                        SensorType sensor, SensorField field,
                        const char *unit) {
  out = baseLabels;

  out += ",sensor=\"";
  appendEscapedLabelValue(out, sensorTypeToString(sensor));
  out += "\"";

  out += ",field=\"";
  appendEscapedLabelValue(out, sensorFieldToString(field));
  out += "\"";

  if (unit != nullptr && unit[0] != '\0') {
    out += ",unit=\"";
    appendEscapedLabelValue(out, unit);
    out += "\"";
  }
}

struct MetricHeaderRegistry {
  static constexpr size_t kMaxNames = 32;
  String names[kMaxNames];
  size_t count = 0;

  bool contains(const String &name) const {
    for (size_t i = 0; i < count; ++i) {
      if (names[i] == name) {
        return true;
      }
    }
    return false;
  }

  void add(const String &name) {
    if (count < kMaxNames) {
      names[count++] = name;
    }
  }
};

void appendMetricHeader(String &out, const char *name, const char *help,
                        const char *type) {
  out += F("# HELP ");
  out += name;
  out += ' ';
  out += help;
  out += '\n';

  out += F("# TYPE ");
  out += name;
  out += ' ';
  out += type;
  out += '\n';
}

void appendMetricHeaderIfNeeded(String &out, MetricHeaderRegistry &registry,
                                const String &name, const char *help,
                                const char *type) {
  if (registry.contains(name)) {
    return;
  }

  appendMetricHeader(out, name.c_str(), help, type);
  registry.add(name);
}

void appendMetricSample(String &out, const char *name, const String &labels,
                        const char *value) {
  out += name;
  out += '{';
  out += labels;
  out += F("} ");
  out += value;
  out += '\n';
}

void appendGaugeSample(String &out, const char *name, const String &labels,
                       const char *value) {
  appendMetricSample(out, name, labels, value);
}

void appendGaugeSample(String &out, const char *name, const String &labels,
                       bool value) {
  appendGaugeSample(out, name, labels, value ? "1" : "0");
}

void appendGaugeSample(String &out, const char *name, const String &labels,
                       int32_t value) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%ld", static_cast<long>(value));
  appendGaugeSample(out, name, labels, buf);
}

void appendGaugeSample(String &out, const char *name, const String &labels,
                       uint32_t value) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(value));
  appendGaugeSample(out, name, labels, buf);
}

void appendGaugeSample(String &out, const char *name, const String &labels,
                       float value, int decimals = 2) {
  char buf[32];
  dtostrf(value, 0, decimals, buf);

  char *start = buf;
  while (*start == ' ') {
    ++start;
  }

  appendGaugeSample(out, name, labels, start);
}

void appendCounterSample(String &out, const char *name, const String &labels,
                         float value, int decimals = 2) {
  char buf[32];
  dtostrf(value, 0, decimals, buf);

  char *start = buf;
  while (*start == ' ') {
    ++start;
  }

  appendMetricSample(out, name, labels, start);
}

void buildBaseLabels(String &out) {
  out = "device_id=\"";
  appendEscapedLabelValue(out, DeviceIdentity::getEffectiveId());
  out += "\",hardware_id=\"";
  appendEscapedLabelValue(out, DeviceIdentity::getHardwareId());
  out += "\"";
}

void sensorLabels(String &out, const String &baseLabels, const char *unit) {
  out = baseLabels;
  out += ",unit=\"";
  appendEscapedLabelValue(out, unit);
  out += "\"";
}

void appendGauge(String &out, const String &labels, const char *name,
                 const char *help, const char *value) {
  const size_t extra = 8 + strlen(name) + 1 + strlen(help) + 1 + 8 +
                       strlen(name) + 7 + strlen(name) + 1 + labels.length() +
                       2 + strlen(value) + 1;

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

void appendGauge(String &out, const String &labels, const char *name,
                 const char *help, const String &value) {
  appendGauge(out, labels, name, help, value.c_str());
}

void appendGauge(String &out, const String &labels, const char *name,
                 const char *help, int32_t value) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%ld", static_cast<long>(value));
  appendGauge(out, labels, name, help, buf);
}

void appendGauge(String &out, const String &labels, const char *name,
                 const char *help, uint32_t value) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(value));
  appendGauge(out, labels, name, help, buf);
}

void appendGauge(String &out, const String &labels, const char *name,
                 const char *help, bool value) {
  appendGauge(out, labels, name, help, value ? "1" : "0");
}

void appendGauge(String &out, const String &labels, const char *name,
                 const char *help, float value, int decimals = 2) {
  char buf[32];
  dtostrf(value, 0, decimals, buf);

  char *start = buf;
  while (*start == ' ') {
    ++start;
  }

  appendGauge(out, labels, name, help, start);
}

void appendCounter(String &out, const String &labels, const char *name,
                   const char *help, const char *value) {
  const size_t extra = 8 + strlen(name) + 1 + strlen(help) + 1 + 8 +
                       strlen(name) + 9 + strlen(name) + 1 + labels.length() +
                       2 + strlen(value) + 1;

  out.reserve(out.length() + extra);

  out += F("# HELP ");
  out += name;
  out += ' ';
  out += help;
  out += '\n';
  out += F("# TYPE ");
  out += name;
  out += F(" counter\n");
  out += name;
  out += '{';
  out += labels;
  out += F("} ");
  out += value;
  out += '\n';
}

void appendCounter(String &out, const String &labels, const char *name,
                   const char *help, const String &value) {
  appendCounter(out, labels, name, help, value.c_str());
}

void appendCounter(String &out, const String &labels, const char *name,
                   const char *help, int32_t value) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%ld", static_cast<long>(value));
  appendCounter(out, labels, name, help, buf);
}

void appendCounter(String &out, const String &labels, const char *name,
                   const char *help, uint32_t value) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(value));
  appendCounter(out, labels, name, help, buf);
}

void appendCounter(String &out, const String &labels, const char *name,
                   const char *help, float value, int decimals = 2) {
  char buf[32];
  dtostrf(value, 0, decimals, buf);

  char *start = buf;
  while (*start == ' ') {
    ++start;
  }

  appendCounter(out, labels, name, help, start);
}

void sensorMetricName(String &out, const char *key) {
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
  String labeled;
  labeled.reserve(labels.length() + strlen(labelKey) + labelVal.length() + 8);
  labeled = labels;
  labeled += ",";
  labeled += labelKey;
  labeled += "=\"";
  appendEscapedLabelValue(labeled, labelVal);
  labeled += "\"";

  appendGauge(out, labeled, name, help, "1");
}

} // namespace

void initMetricsFormatter() { buildBaseLabels(cachedBaseLabels); }

const String &getBaseLabels() {
  if (cachedBaseLabels.isEmpty()) {
    buildBaseLabels(cachedBaseLabels);
  }
  return cachedBaseLabels;
}

void appendSensorMetric(String &out, String &nameBuffer, String &labelsBuffer,
                        const SensorMetric &metric) {
  const String &baseLabels = getBaseLabels();

  if (metric.family != nullptr) {
    sensorMetricName(nameBuffer, metric.family);
    sensorMetricLabels(labelsBuffer, baseLabels, metric.sensor, metric.field, metric.unit);
  } else {
    sensorMetricName(nameBuffer, metric.name);
    sensorLabels(labelsBuffer, baseLabels, metric.unit ? metric.unit : "");
  }

  if (metric.type == SensorMetricType::Counter) {
    appendCounter(out, labelsBuffer, nameBuffer.c_str(), metric.help, metric.value);
    return;
  }

  appendGauge(out, labelsBuffer, nameBuffer.c_str(), metric.help, metric.value);
}

//void appendSensorMetric(String &out, String &nameBuffer, String &labelsBuffer,
//                        const SensorMetric &metric) {
//  const String &labels = getBaseLabels();
//
//  sensorMetricName(nameBuffer, metric.name);
//  sensorLabels(labelsBuffer, labels, metric.unit);
//
//  //if (metric.type == SensorMetricType::) {
//  //  appendGauge(out, labelsBuffer, nameBuffer.c_str(), metric.help,
//  //              metric.value != 0.0f);
//  //  return;
//  //}
//
//  if (metric.type == SensorMetricType::Counter) {
//    appendCounter(out, labelsBuffer, nameBuffer.c_str(), metric.help, metric.value);
//    return;
//  }
//
//  appendGauge(out, labelsBuffer, nameBuffer.c_str(), metric.help, metric.value);
//}

void formatDeviceMetrics(String &out, const DeviceState &state) {
  const String &labels = getBaseLabels();

  appendGauge(out, labels, "esp32_up",
              "Whether the ESP32 application is running.", true);

  appendGauge(out, labels, "esp32_wifi_connected", "Whether WiFi is connected.",
              state.wifiConnected);

  appendGauge(out, labels, "esp32_uptime_seconds",
              "Time since boot in seconds.", state.uptimeSec);

  appendGauge(out, labels, "esp32_heap_free_bytes", "Free heap in bytes.",
              state.freeHeapBytes);

  appendGauge(out, labels, "esp32_wifi_rssi_dbm", "WiFi RSSI in dBm.",
              state.wifiRssiDbm);

  appendInfoGauge(out, labels, "esp32_wifi_status_info",
                  "WiFi status as an informational labeled metric.", "status",
                  state.wifiStatus);
}

void formatDeviceMetrics(String &out, const DeviceState &state,
                         uint32_t metricsBuildDurationMs,
                         uint32_t metricsLastBuildUptimeSeconds) {
  const String &labels = getBaseLabels();

  formatDeviceMetrics(out, state);

  appendGauge(out, labels, "esp32_metrics_build_duration_ms",
              "Time spent rebuilding the cached Prometheus metrics.",
              metricsBuildDurationMs);

  appendGauge(out, labels, "esp32_metrics_last_build_uptime_seconds",
              "Device uptime when cached Prometheus metrics were last rebuilt.",
              metricsLastBuildUptimeSeconds);
}


