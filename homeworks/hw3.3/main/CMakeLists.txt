idf_component_register(SRCS "main.c" "adc.c" "led.c" "pwm.c" "motor.c"
                       PRIV_REQUIRES spi_flash esp_driver_gpio esp_adc esp_driver_ledc
                       INCLUDE_DIRS "")
