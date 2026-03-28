#include "esp_err.h"
#include "gps_logger.hpp"

extern "C" void app_main(void)
{
    gps_logger::logger_config config{};
    ESP_ERROR_CHECK(gps_logger::run_logger(config));
}
