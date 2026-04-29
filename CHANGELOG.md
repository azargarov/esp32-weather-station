## Changelog

### Changed
- Refactored BME280 calibration API from direct offset input to reference-based calibration.
- Calibration endpoint now accepts a measured reference value and calculates the offset internally.
- Calibration values are persisted in ESP32 NVS and restored after reboot.
- Calibration data is available even when the physical BME280 sensor is not detected.

### Added
- `GET /api/sensors/bme280/calibration`
  - Returns current calibration values for temperature, humidity, and pressure.
- `POST /api/sensors/bme280/temperature/calibration`
  - Calibrates temperature using a reference value.
- `POST /api/sensors/bme280/humidity/calibration`
  - Calibrates humidity using a reference value.
- `POST /api/sensors/bme280/pressure/calibration`
  - Calibrates pressure using a reference value.

### Example

```bash
curl -X POST http://<device-ip>/api/sensors/bme280/temperature/calibration \
  -H "Content-Type: application/json" \
  -d '{"reference": 22.7}'
```
### Note

- Offset is calculated as:
```bash
offset = reference - raw
```
- Corrected value is calculated as:
  - Offset is calculated as:
```bash
corrected = raw + offset
```