idf_component_register(SRCS "bmp280.c" "main.c"
                       PRIV_REQUIRES esp_driver_gpio esp_driver_spi
                       INCLUDE_DIRS "")
