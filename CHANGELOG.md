## Changelog

### Changed
- Refactored sensor calibration from single-reference offset updates to point-based calibration updates.
- Calibration endpoints now accept both a `reference` value and a calibration `point` (`1` or `2`).
- Calibration profiles are recalculated automatically:
  - one point produces offset calibration
  - two points produce linear two-point calibration
- Calibration logic is now shared through `CalibrationManager::updateCalibrationPoint()`.
- Calibration values are persisted in ESP32 NVS and restored after reboot.
- Calibration data remains available even when the physical sensor is not currently detected.
- BME280 now uses forced measurement mode for on-demand sampling.
- BME280 pressure sampling was increased to `SAMPLING_X8` and IIR filtering to `FILTER_X4` for steadier pressure readings.
- Metrics update interval is now configurable via `Config::METRICS_CACHE_UPDATE_INTERVAL_MS`.

### Added
- Point-based calibration API for sensor fields:
  - `POST /api/sensors/bme280/temperature/calibration`
  - `POST /api/sensors/bme280/humidity/calibration`
  - `POST /api/sensors/bme280/pressure/calibration`
- Calibration request validation:
  - rejects invalid or missing `reference`
  - rejects invalid `point` values outside `1` and `2`
- `GET /api/sensors/bme280/calibration`
  - Returns current calibration values for temperature, humidity, and pressure.

- Prometheus metrics:
  - `esp32_metrics_build_duration_ms`
    - Time spent rebuilding the cached metrics response.
  - `esp32_metrics_last_build_uptime_seconds`
    - Device uptime when metrics cache was last rebuilt.

- Metrics caching:
  - Metrics are precomputed and served from cache.
  - Cache is updated periodically instead of on every request.

### Fixed
- BME280 reads now fail cleanly when a forced measurement cannot be completed.
- Cached BME280 readings are updated only when all measured values are valid.
- Invalid sensor reads now reset cached temperature, pressure, and humidity values to `NaN`.

### Example

```bash
curl -X POST http://<device-ip>/api/sensors/bme280/temperature/calibration \
  -H "Content-Type: application/json" \
  -d '{"reference": 22.7, "point": 1}'