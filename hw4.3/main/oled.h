#pragma once

#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

/* SSD1306 128x64, frame buffer of 8 pages, 128 bytes each. */

/* Init sequence; the leading byte 0x00 is the control byte. */
esp_err_t oled_init(i2c_master_dev_handle_t dev);

/* Draws into the buffer only, the bus stays untouched until oled_flush(). */
void oled_fb_clear(void);
void oled_pixel(int x, int y, int on);
void oled_fill(int x, int y, int w, int h, int on);

/* 5x7, 6 px pitch; a...z are raised to A...Z, anything else becomes a space. */
void oled_text_xy(int x, int y, const char* s);

/* Pushes the buffer to the display, 8 pages of 129 bytes. */
void oled_flush(void);
