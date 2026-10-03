idf_component_register(SRCS "lock_leds.c" "led.c" "main.c" "pwm.c" "pcnt.c" "lock_fsm.c"
                       PRIV_REQUIRES spi_flash esp_driver_gpio esp_adc esp_driver_ledc esp_driver_pcnt esp_driver_uart vfs
                       INCLUDE_DIRS "")
