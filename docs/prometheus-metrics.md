# ESP32 Sensor Node Metrics Reference

This document describes the current Prometheus metrics format exposed by the ESP32 sensor node at `/metrics`.

Example endpoint:

```bash
curl http://192.168.1.233/metrics
```

The endpoint returns Prometheus text exposition format.

## Metric model overview

The firmware exposes three main groups of metrics:

1. Device-level metrics
2. Sensor state and rolling statistics
3. Calibration state metrics

The current design uses a mixed model:

- **device metrics** use dedicated metric names such as `esp32_up`
- **sensor availability and read status** use sensor-specific metric names such as `esp32_sensor_bme280_available`
- **measured values and rolling statistics** use shared metric families such as `esp32_sensor_mean_60s` with labels like `sensor`, `field`, and `unit`
- **calibration metrics** use shared metric families such as `esp32_sensor_calibration_scale`

## Common labels

Most metrics include these identity labels:

```text
device_id="esp32-001",hardware_id="esp32-6CD534A50528"
```

Some metric families also include:

- `sensor` — sensor type, for example `bme280` or `bh1750`
- `field` — measured field, for example `temperature`, `humidity`, `pressure`, `illuminance`
- `unit` — unit string, for example `celsius`, `percent`, `hpa`, `lux`, `bool`, `count`
- `status` — WiFi state for `esp32_wifi_status_info`

## 1. Core device metrics

### `esp32_up`

Whether the ESP32 firmware is running.

```text
# HELP esp32_up Whether the ESP32 application is running.
# TYPE esp32_up gauge
esp32_up{device_id="esp32-001",hardware_id="esp32-6CD534A50528"} 1
```

### `esp32_wifi_connected`

Whether WiFi is currently connected.

```text
# HELP esp32_wifi_connected Whether WiFi is connected.
# TYPE esp32_wifi_connected gauge
esp32_wifi_connected{device_id="esp32-001",hardware_id="esp32-6CD534A50528"} 1
```

### `esp32_uptime_seconds`

Time since boot in seconds.

```text
# HELP esp32_uptime_seconds Time since boot in seconds.
# TYPE esp32_uptime_seconds gauge
esp32_uptime_seconds{device_id="esp32-001",hardware_id="esp32-6CD534A50528"} 49328
```

### `esp32_heap_free_bytes`

Free heap memory in bytes.

```text
# HELP esp32_heap_free_bytes Free heap in bytes.
# TYPE esp32_heap_free_bytes gauge
esp32_heap_free_bytes{device_id="esp32-001",hardware_id="esp32-6CD534A50528"} 231500
```

### `esp32_wifi_rssi_dbm`

Current WiFi RSSI in dBm.

```text
# HELP esp32_wifi_rssi_dbm WiFi RSSI in dBm.
# TYPE esp32_wifi_rssi_dbm gauge
esp32_wifi_rssi_dbm{device_id="esp32-001",hardware_id="esp32-6CD534A50528"} -51
```

### `esp32_wifi_status_info`

WiFi status represented as a labeled informational metric.

```text
# HELP esp32_wifi_status_info WiFi status as an informational labeled metric.
# TYPE esp32_wifi_status_info gauge
esp32_wifi_status_info{device_id="esp32-001",hardware_id="esp32-6CD534A50528",status="connected"} 1
```

## 2. Metrics cache diagnostics

### `esp32_metrics_build_duration_ms`

Time spent rebuilding the cached metrics response.

```text
# HELP esp32_metrics_build_duration_ms Time spent rebuilding the cached Prometheus metrics.
# TYPE esp32_metrics_build_duration_ms gauge
esp32_metrics_build_duration_ms{device_id="esp32-001",hardware_id="esp32-6CD534A50528"} 6
```

### `esp32_metrics_last_build_uptime_seconds`

Device uptime when cached metrics were last rebuilt.

```text
# HELP esp32_metrics_last_build_uptime_seconds Device uptime when cached Prometheus metrics were last rebuilt.
# TYPE esp32_metrics_last_build_uptime_seconds gauge
esp32_metrics_last_build_uptime_seconds{device_id="esp32-001",hardware_id="esp32-6CD534A50528"} 49327
```

Useful PromQL:

```promql
esp32_uptime_seconds - esp32_metrics_last_build_uptime_seconds
```

This estimates the age of the cached metrics snapshot.

## 3. Sensor availability and read health

### BME280

#### `esp32_sensor_bme280_available`

Whether the BME280 sensor was detected.

#### `esp32_sensor_bme280_read_ok`

Whether the last BME280 read succeeded.

#### `esp32_sensor_bme280_read_errors_total`

Total number of BME280 read errors.

Example:

```text
# HELP esp32_sensor_bme280_available Whether the BME280 sensor is available.
# TYPE esp32_sensor_bme280_available gauge
esp32_sensor_bme280_available{device_id="esp32-001",hardware_id="esp32-6CD534A50528",unit="bool"} 1.00

# HELP esp32_sensor_bme280_read_ok Whether the last BME280 read succeeded.
# TYPE esp32_sensor_bme280_read_ok gauge
esp32_sensor_bme280_read_ok{device_id="esp32-001",hardware_id="esp32-6CD534A50528",unit="bool"} 1.00

# HELP esp32_sensor_bme280_read_errors_total Total number of BME280 read errors.
# TYPE esp32_sensor_bme280_read_errors_total counter
esp32_sensor_bme280_read_errors_total{device_id="esp32-001",hardware_id="esp32-6CD534A50528",unit="count"} 0.00
```

### BH1750

#### `esp32_sensor_bh1750_available`

Whether the BH1750 sensor was detected.

#### `esp32_sensor_bh1750_read_ok`

Whether the last BH1750 read succeeded.

#### `esp32_sensor_bh1750_read_errors_total`

Total number of BH1750 read errors.

Example:

```text
# HELP esp32_sensor_bh1750_available Whether the BH1750 sensor is available.
# TYPE esp32_sensor_bh1750_available gauge
esp32_sensor_bh1750_available{device_id="esp32-001",hardware_id="esp32-6CD534A50528",unit="bool"} 0.00

# HELP esp32_sensor_bh1750_read_ok Whether the last BH1750 read succeeded.
# TYPE esp32_sensor_bh1750_read_ok gauge
esp32_sensor_bh1750_read_ok{device_id="esp32-001",hardware_id="esp32-6CD534A50528",unit="bool"} 0.00

# HELP esp32_sensor_bh1750_read_errors_total Total number of BH1750 read errors.
# TYPE esp32_sensor_bh1750_read_errors_total counter
esp32_sensor_bh1750_read_errors_total{device_id="esp32-001",hardware_id="esp32-6CD534A50528",unit="count"} 0.00
```

## 4. Rolling sensor statistics

Measured sensor values are exposed through shared metric families.

These metric names do **not** encode the sensor field directly. Instead, the field is stored in labels.

### Rolling-stat metric families

| Metric | Meaning |
|---|---|
| `esp32_sensor_mean_60s` | Mean value over the last 60-second rolling window |
| `esp32_sensor_median_60s` | Median value over the last 60-second rolling window |
| `esp32_sensor_min_60s` | Minimum value over the last 60-second rolling window |
| `esp32_sensor_max_60s` | Maximum value over the last 60-second rolling window |
| `esp32_sensor_range_60s` | Range over the last 60-second rolling window |
| `esp32_sensor_stddev_60s` | Standard deviation over the last 60-second rolling window |
| `esp32_sensor_slope_per_minute_60s` | Linear regression slope over the rolling window, expressed per minute |
| `esp32_sensor_samples_60s` | Number of valid samples in the rolling window |

### Label structure

Typical labels for rolling-stat metrics:

```text
sensor="bme280",field="temperature",unit="celsius"
```

Current field values seen in the metrics stream:

- `temperature`
- `humidity`
- `pressure`

The metrics model is also ready for additional fields such as:

- `illuminance`

### Example: temperature

```text
# HELP esp32_sensor_mean_60s Mean value over the last 60-second rolling window.
# TYPE esp32_sensor_mean_60s gauge
esp32_sensor_mean_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="temperature",unit="celsius"} 21.98

# HELP esp32_sensor_median_60s Median value over the last 60-second rolling window.
# TYPE esp32_sensor_median_60s gauge
esp32_sensor_median_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="temperature",unit="celsius"} 21.99

# HELP esp32_sensor_min_60s Minimum value over the last 60-second rolling window.
# TYPE esp32_sensor_min_60s gauge
esp32_sensor_min_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="temperature",unit="celsius"} 21.97

# HELP esp32_sensor_max_60s Maximum value over the last 60-second rolling window.
# TYPE esp32_sensor_max_60s gauge
esp32_sensor_max_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="temperature",unit="celsius"} 22.01

# HELP esp32_sensor_range_60s Range over the last 60-second rolling window.
# TYPE esp32_sensor_range_60s gauge
esp32_sensor_range_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="temperature",unit="celsius"} 0.04

# HELP esp32_sensor_stddev_60s Standard deviation over the last 60-second rolling window.
# TYPE esp32_sensor_stddev_60s gauge
esp32_sensor_stddev_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="temperature",unit="celsius"} 0.01

# HELP esp32_sensor_slope_per_minute_60s Linear regression slope over the rolling window, expressed per minute.
# TYPE esp32_sensor_slope_per_minute_60s gauge
esp32_sensor_slope_per_minute_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="temperature",unit="celsius"} -0.00

# HELP esp32_sensor_samples_60s Number of valid samples in the rolling statistics window.
# TYPE esp32_sensor_samples_60s gauge
esp32_sensor_samples_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="temperature"} 60.00
```

### Example: humidity

```text
esp32_sensor_mean_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="humidity",unit="percent"} 22.51
esp32_sensor_median_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="humidity",unit="percent"} 22.50
esp32_sensor_min_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="humidity",unit="percent"} 22.47
esp32_sensor_max_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="humidity",unit="percent"} 22.60
esp32_sensor_range_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="humidity",unit="percent"} 0.13
esp32_sensor_stddev_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="humidity",unit="percent"} 0.03
esp32_sensor_slope_per_minute_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="humidity",unit="percent"} 0.02
esp32_sensor_samples_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="humidity"} 60.00
```

### Example: pressure

```text
esp32_sensor_mean_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="pressure",unit="hpa"} 1010.59
esp32_sensor_median_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="pressure",unit="hpa"} 1010.59
esp32_sensor_min_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="pressure",unit="hpa"} 1010.52
esp32_sensor_max_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="pressure",unit="hpa"} 1010.68
esp32_sensor_range_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="pressure",unit="hpa"} 0.17
esp32_sensor_stddev_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="pressure",unit="hpa"} 0.04
esp32_sensor_slope_per_minute_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="pressure",unit="hpa"} -0.04
esp32_sensor_samples_60s{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="pressure"} 60.00
```

## 5. Calibration metrics

Calibration state is also exposed through shared metric families.

### Metric families

| Metric | Meaning |
|---|---|
| `esp32_sensor_calibration_enabled` | Whether calibration is enabled for this sensor field |
| `esp32_sensor_calibration_scale` | Linear calibration scale coefficient |
| `esp32_sensor_calibration_offset` | Linear calibration offset coefficient |
| `esp32_sensor_calibration_mode` | Calibration mode enum: `0` none, `1` offset, `2` two-point |

### Example: BME280 temperature calibration

```text
# HELP esp32_sensor_calibration_enabled Whether calibration is enabled for this sensor field.
# TYPE esp32_sensor_calibration_enabled gauge
esp32_sensor_calibration_enabled{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="temperature"} 1.00

# HELP esp32_sensor_calibration_scale Linear calibration scale coefficient.
# TYPE esp32_sensor_calibration_scale gauge
esp32_sensor_calibration_scale{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="temperature"} 1.02

# HELP esp32_sensor_calibration_offset Linear calibration offset coefficient.
# TYPE esp32_sensor_calibration_offset gauge
esp32_sensor_calibration_offset{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="temperature",unit="celsius"} 0.26

# HELP esp32_sensor_calibration_mode Calibration mode as numeric enum: 0 none, 1 offset, 2 two-point.
# TYPE esp32_sensor_calibration_mode gauge
esp32_sensor_calibration_mode{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="temperature"} 2.00
```

### Example: BME280 humidity calibration

```text
esp32_sensor_calibration_enabled{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="humidity"} 1.00
esp32_sensor_calibration_scale{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="humidity"} 1.19
esp32_sensor_calibration_offset{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="humidity",unit="percent"} -12.64
esp32_sensor_calibration_mode{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="humidity"} 2.00
```

### Example: BME280 pressure calibration

```text
esp32_sensor_calibration_enabled{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="pressure"} 1.00
esp32_sensor_calibration_scale{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="pressure"} 1.00
esp32_sensor_calibration_offset{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="pressure",unit="hpa"} -0.72
esp32_sensor_calibration_mode{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bme280",field="pressure"} 1.00
```

### Example: BH1750 calibration state

Even when the BH1750 is not available, the calibration metrics can still exist.

```text
esp32_sensor_calibration_enabled{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bh1750",field="illuminance"} 0.00
esp32_sensor_calibration_scale{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bh1750",field="illuminance"} 1.00
esp32_sensor_calibration_offset{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bh1750",field="illuminance",unit="lux"} 0.00
esp32_sensor_calibration_mode{device_id="esp32-001",hardware_id="esp32-6CD534A50528",sensor="bh1750",field="illuminance"} 0.00
```

## 6. PromQL examples

### Temperature mean by room

```promql
esp32_sensor_mean_60s{sensor="bme280",field="temperature",room="livingroom"}
esp32_sensor_mean_60s{sensor="bme280",field="temperature",room="balcony"}
```

### Indoor vs outdoor temperature difference

```promql
esp32_sensor_mean_60s{sensor="bme280",field="temperature",room="livingroom"}
-
ignoring(room)
esp32_sensor_mean_60s{sensor="bme280",field="temperature",room="balcony"}
```

### Pressure stability

```promql
esp32_sensor_stddev_60s{sensor="bme280",field="pressure"}
esp32_sensor_range_60s{sensor="bme280",field="pressure"}
```

### Sensor health

```promql
esp32_sensor_bme280_read_ok
esp32_sensor_bme280_read_errors_total
esp32_sensor_bh1750_read_ok
```

### Calibration overview

```promql
esp32_sensor_calibration_mode{sensor="bme280"}
esp32_sensor_calibration_scale{sensor="bme280"}
esp32_sensor_calibration_offset{sensor="bh1750",field="illuminance"}
```

## 7. Notes on the current format

A few details about the current exposition format:

- Boolean-like metrics currently use `unit="bool"`
- Read error counters currently use `unit="count"`
- Rolling measurement metrics are generic and differentiated through labels rather than unique metric names per field
- Calibration metrics are generic and work across multiple sensors and fields
- The current format is well suited for dashboards that are sensor-name independent and rely on labels such as `sensor`, `field`, and Prometheus target labels like `room`

## 8. Suggested dashboard direction

The current metric design works well for:

- per-room environmental panels
- inside/outside comparisons
- calibration state dashboards
- sensor health and reliability panels
- long-term statistical analysis using rolling metrics such as mean, median, range, standard deviation, and slope
