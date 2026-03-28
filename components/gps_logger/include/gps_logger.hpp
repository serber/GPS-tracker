#pragma once

#include <cstdint>

#include "esp_err.h"

namespace gps_logger
{

struct logger_config
{
    // UART1 by default.
    int gps_uart_port = 1;
    int gps_rx_pin = 3;
    int gps_tx_pin = 2;
    uint32_t gps_baud_rate = 9600;

    // SPI2 / VSPI host by default on ESP32.
    int sd_spi_host = 1;
    int sd_mosi_pin = 23;
    int sd_miso_pin = 19;
    int sd_sclk_pin = 18;
    int sd_cs_pin = 4;

    uint32_t log_interval_ms = 1000;
    uint32_t poll_delay_ms = 20;
    const char *mount_point = "/sdcard";
};

esp_err_t run_logger(const logger_config &config = logger_config{});

}  // namespace gps_logger
