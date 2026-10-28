#pragma once
#include "driver/gpio.h"
#include "driver/spi_master.h"

/* BMP280 on SPI2 (FSPI), 4-wire mode. These are not the native SPI2 pins, so the
signals are routed through the GPIO matrix — that costs nothing at 1 MHz. */
#define BMP_HOST SPI2_HOST
#define BMP_SCLK_GPIO GPIO_NUM_8  /* sensor SCK  */
#define BMP_MOSI_GPIO GPIO_NUM_9  /* sensor SDI, master out */
#define BMP_MISO_GPIO GPIO_NUM_10 /* sensor SDO, master in */
#define BMP_CS_GPIO GPIO_NUM_11   /* sensor CSB, active low */
#define BMP_HZ 1000000            /* datasheet allows up to 10 MHz */
#define BMP_MODE 0                /* CPOL=0 CPHA=0; the BMP280 also accepts mode 3 */
