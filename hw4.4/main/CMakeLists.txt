idf_component_register(SRCS "main.c" "bme280.c"
                       PRIV_REQUIRES esp_driver_gpio esp_driver_spi
                       INCLUDE_DIRS "")
