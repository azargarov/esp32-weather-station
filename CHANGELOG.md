## Changelog

### Changed
- Refactored BME280 calibration API from direct offset input to reference-based calibration.
- Calibration endpoint now accepts a measured reference value and calculates the offset internally.
- Calibration values are persisted in ESP32 NVS and restored after reboot.
- Calibration data is available even when the physical BME280 sensor is not detected.
- Metrics update interval is now configurable via `Config::METRICS_CACHE_UPDATE_INTERVAL_MS`.

### Added
- `GET /api/sensors/bme280/calibration`
  - Returns current calibration values for temperature, humidity, and pressure.
- `POST /api/sensors/bme280/temperature/calibration`
  - Calibrates temperature using a reference value.
- `POST /api/sensors/bme280/humidity/calibration`
  - Calibrates humidity using a reference value.
- `POST /api/sensors/bme280/pressure/calibration`
  - Calibrates pressure using a reference value.

- Prometheus metrics:
  - `esp32_metrics_build_duration_ms`
    - Time spent rebuilding the cached metrics response.
  - `esp32_metrics_last_build_uptime_seconds`
    - Device uptime when metrics cache was last rebuilt.

- Metrics caching:
  - Metrics are now precomputed and served from cache.
  - Cache is updated periodically instead of on every request.

### Example

```bash
curl -X POST http://<device-ip>/api/sensors/bme280/temperature/calibration \
  -H "Content-Type: application/json" \
  -d '{"reference": 22.7}'