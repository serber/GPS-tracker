#include "gps_logger.hpp"

#include <array>
#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <unistd.h>

#include "TinyGPSPlus.h"
#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdmmc_cmd.h"

namespace gps_logger
{
namespace
{

constexpr const char *TAG = "gps_logger";
constexpr int UART_RX_BUFFER_SIZE = 2048;
constexpr int UART_CHUNK_SIZE = 128;
constexpr uint32_t MAX_FIX_AGE_MS = 5000;

uint64_t now_ms()
{
    return static_cast<uint64_t>(esp_timer_get_time() / 1000);
}

struct gps_fix
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude_m = 0.0;
    double speed_kmph = 0.0;
    double course_deg = 0.0;
    uint32_t satellites = 0;
    double hdop = 0.0;

    uint64_t utc_stamp() const
    {
        return static_cast<uint64_t>(year) * 10000000000ULL
            + static_cast<uint64_t>(month) * 100000000ULL
            + static_cast<uint64_t>(day) * 1000000ULL
            + static_cast<uint64_t>(hour) * 10000ULL
            + static_cast<uint64_t>(minute) * 100ULL
            + static_cast<uint64_t>(second);
    }
};

bool extract_fix(TinyGPSPlus &gps, gps_fix *fix)
{
    if (fix == nullptr) {
        return false;
    }

    if (!gps.location.isValid() || !gps.date.isValid() || !gps.time.isValid()) {
        return false;
    }

    if (gps.location.age() > MAX_FIX_AGE_MS || gps.date.age() > MAX_FIX_AGE_MS || gps.time.age() > MAX_FIX_AGE_MS) {
        return false;
    }

    fix->year = gps.date.year();
    fix->month = gps.date.month();
    fix->day = gps.date.day();
    fix->hour = gps.time.hour();
    fix->minute = gps.time.minute();
    fix->second = gps.time.second();
    fix->latitude = gps.location.lat();
    fix->longitude = gps.location.lng();
    fix->altitude_m = gps.altitude.isValid() ? gps.altitude.meters() : 0.0;
    fix->speed_kmph = gps.speed.isValid() ? gps.speed.kmph() : 0.0;
    fix->course_deg = gps.course.isValid() ? gps.course.deg() : 0.0;
    fix->satellites = gps.satellites.isValid() ? gps.satellites.value() : 0;
    fix->hdop = gps.hdop.isValid() ? gps.hdop.hdop() : 0.0;
    return true;
}

class gps_uart_reader
{
public:
    explicit gps_uart_reader(const logger_config &config) : config_(config) {}

    ~gps_uart_reader()
    {
        if (driver_installed_) {
            uart_driver_delete(static_cast<uart_port_t>(config_.gps_uart_port));
        }
    }

    esp_err_t init()
    {
        uart_config_t uart_config = {};
        uart_config.baud_rate = static_cast<int>(config_.gps_baud_rate);
        uart_config.data_bits = UART_DATA_8_BITS;
        uart_config.parity = UART_PARITY_DISABLE;
        uart_config.stop_bits = UART_STOP_BITS_1;
        uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        uart_config.source_clk = UART_SCLK_DEFAULT;

        const uart_port_t uart_port = static_cast<uart_port_t>(config_.gps_uart_port);

        ESP_RETURN_ON_ERROR(
            uart_driver_install(uart_port, UART_RX_BUFFER_SIZE, 0, 0, nullptr, 0),
            TAG,
            "failed to install GPS UART driver");
        driver_installed_ = true;

        esp_err_t err = uart_param_config(uart_port, &uart_config);
        if (err != ESP_OK) {
            return err;
        }

        return uart_set_pin(
            uart_port,
            config_.gps_tx_pin,
            config_.gps_rx_pin,
            UART_PIN_NO_CHANGE,
            UART_PIN_NO_CHANGE);
    }

    int pump(TinyGPSPlus &gps)
    {
        std::array<uint8_t, UART_CHUNK_SIZE> buffer{};
        const int bytes_read = uart_read_bytes(
            static_cast<uart_port_t>(config_.gps_uart_port),
            buffer.data(),
            buffer.size(),
            0);
        for (int i = 0; i < bytes_read; ++i) {
            gps.encode(static_cast<char>(buffer[static_cast<size_t>(i)]));
        }
        return bytes_read;
    }

private:
    const logger_config &config_;
    bool driver_installed_ = false;
};

class sd_card_logger
{
public:
    explicit sd_card_logger(const logger_config &config) : config_(config) {}

    ~sd_card_logger()
    {
        if (mounted_) {
            esp_vfs_fat_sdcard_unmount(config_.mount_point, card_);
        }
        if (spi_bus_initialized_) {
            spi_bus_free(static_cast<spi_host_device_t>(config_.sd_spi_host));
        }
    }

    esp_err_t init()
    {
        spi_bus_config_t bus_config = {};
        bus_config.mosi_io_num = config_.sd_mosi_pin;
        bus_config.miso_io_num = config_.sd_miso_pin;
        bus_config.sclk_io_num = config_.sd_sclk_pin;
        bus_config.quadwp_io_num = -1;
        bus_config.quadhd_io_num = -1;
        bus_config.max_transfer_sz = 4000;

        const spi_host_device_t spi_host = static_cast<spi_host_device_t>(config_.sd_spi_host);

        ESP_RETURN_ON_ERROR(
            spi_bus_initialize(spi_host, &bus_config, SDSPI_DEFAULT_DMA),
            TAG,
            "failed to initialize SPI bus for SD card");
        spi_bus_initialized_ = true;

        sdmmc_host_t host = SDSPI_HOST_DEFAULT();
        host.slot = spi_host;

        sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
        slot_config.gpio_cs = static_cast<gpio_num_t>(config_.sd_cs_pin);
        slot_config.host_id = spi_host;

        esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
        mount_config.format_if_mount_failed = false;
        mount_config.max_files = 4;
        mount_config.allocation_unit_size = 16 * 1024;

        esp_err_t err = esp_vfs_fat_sdspi_mount(
            config_.mount_point,
            &host,
            &slot_config,
            &mount_config,
            &card_);
        if (err != ESP_OK) {
            return err;
        }

        mounted_ = true;
        sdmmc_card_print_info(stdout, card_);
        return ESP_OK;
    }

    esp_err_t append_fix(const gps_fix &fix)
    {
        char file_path[64];
        std::snprintf(
            file_path,
            sizeof(file_path),
            "%s/%04d%02d%02d.csv",
            config_.mount_point,
            fix.year,
            fix.month,
            fix.day);

        const bool file_exists = access(file_path, F_OK) == 0;
        FILE *file = std::fopen(file_path, "a");
        if (file == nullptr) {
            ESP_LOGE(TAG, "failed to open %s: errno=%d", file_path, errno);
            return ESP_FAIL;
        }

        if (!file_exists) {
            std::fputs("date,time,latitude,longitude,altitude_m,speed_kmph,course_deg\n", file);
        }

        char line[192];
        std::snprintf(
            line,
            sizeof(line),
            "%04d.%02d.%02d,%02d:%02d:%02d,%.8f,%.8f,%.2f,%.2f,%.2f\n",
            fix.year,
            fix.month,
            fix.day,
            fix.hour,
            fix.minute,
            fix.second,
            fix.latitude,
            fix.longitude,
            fix.altitude_m,
            fix.speed_kmph,
            fix.course_deg);

        const bool write_failed = std::fputs(line, file) < 0 || std::fflush(file) != 0;
        std::fclose(file);
        if (write_failed) {
            ESP_LOGE(TAG, "failed to append GPS data to %s", file_path);
            return ESP_FAIL;
        }

        ESP_LOGI(
            TAG,
            "logged fix %04d-%02d-%02d %02d:%02d:%02d lat=%.8f lon=%.8f alt=%.2fm speed=%.2fkm/h course=%.2fdeg sats=%" PRIu32 " hdop=%.1f",
            fix.year,
            fix.month,
            fix.day,
            fix.hour,
            fix.minute,
            fix.second,
            fix.latitude,
            fix.longitude,
            fix.altitude_m,
            fix.speed_kmph,
            fix.course_deg,
            fix.satellites,
            fix.hdop);
        return ESP_OK;
    }

private:
    const logger_config &config_;
    sdmmc_card_t *card_ = nullptr;
    bool mounted_ = false;
    bool spi_bus_initialized_ = false;
};

}  // namespace

esp_err_t run_logger(const logger_config &config)
{
    ESP_LOGI(
        TAG,
        "starting GPS logger: gps_uart=%d rx=%d tx=%d baud=%" PRIu32 " sd_cs=%d interval=%" PRIu32 "ms",
        static_cast<int>(config.gps_uart_port),
        config.gps_rx_pin,
        config.gps_tx_pin,
        config.gps_baud_rate,
        config.sd_cs_pin,
        config.log_interval_ms);

    gps_uart_reader uart_reader(config);
    ESP_RETURN_ON_ERROR(uart_reader.init(), TAG, "GPS UART init failed");

    sd_card_logger storage(config);
    ESP_RETURN_ON_ERROR(storage.init(), TAG, "SD card init failed");

    TinyGPSPlus gps;
    uint64_t last_logged_at_ms = 0;
    uint64_t last_logged_fix_stamp = 0;
    bool first_fix_reported = false;
    bool no_data_warning_reported = false;
    const uint64_t start_ms = now_ms();

    while (true) {
        const int bytes_read = uart_reader.pump(gps);
        if (bytes_read <= 0) {
            vTaskDelay(pdMS_TO_TICKS(config.poll_delay_ms));
        }

        if (!no_data_warning_reported && now_ms() - start_ms > 30000 && gps.charsProcessed() == 0) {
            ESP_LOGW(TAG, "no GPS UART data after 30 seconds; check power, pins and baud rate");
            no_data_warning_reported = true;
        }

        gps_fix fix;
        if (!extract_fix(gps, &fix)) {
            continue;
        }

        if (!first_fix_reported) {
            ESP_LOGI(TAG, "GPS fix acquired");
            first_fix_reported = true;
        }

        const uint64_t current_ms = now_ms();
        if (fix.utc_stamp() == last_logged_fix_stamp) {
            continue;
        }
        if (current_ms - last_logged_at_ms < config.log_interval_ms) {
            continue;
        }

        ESP_RETURN_ON_ERROR(storage.append_fix(fix), TAG, "failed to persist GPS fix");
        last_logged_at_ms = current_ms;
        last_logged_fix_stamp = fix.utc_stamp();
    }

    return ESP_OK;
}

}  // namespace gps_logger
