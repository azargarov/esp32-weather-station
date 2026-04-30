#pragma once

#include <stdint.h>
#include <secrets.h>

namespace Config {
constexpr uint32_t CPU_FREQ_MHZ = 240;
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t LOOP_DELAY_MS = 10;

constexpr uint32_t WIFI_CONNECT_RETRY_DELAY_MS = 500;
constexpr uint32_t WIFI_CONNECT_MAX_RETRIES_MAX_POWER = 30;
constexpr uint32_t WIFI_CONNECT_MAX_RETRIES = 60;
constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 10000;

constexpr bool BME280_ENABLED = true;
constexpr uint8_t BME280_I2C_ADDRESS = 0x76; //  0x77
constexpr uint32_t SENSOR_READ_INTERVAL_MS = 5000;
constexpr uint32_t SENSOR_PROBE_INTERVAL_MS = 60000;
} // namespace Config