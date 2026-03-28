idf_component_register(
    SRCS "gps_logger.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer
    PRIV_REQUIRES
        esp_tinygpsplusplus
        esp_driver_gpio
        esp_driver_spi
        esp_driver_sdspi
        esp_driver_uart
        fatfs
        sdmmc
)
