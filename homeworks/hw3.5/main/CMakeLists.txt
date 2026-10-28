idf_component_register(SRCS "main.c" "adc.c" "pwm.c"
                       PRIV_REQUIRES spi_flash esp_driver_gpio esp_adc esp_driver_ledc
                       INCLUDE_DIRS "")
