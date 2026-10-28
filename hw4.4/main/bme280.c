#include "bme280.h"
#include <stdint.h>
#include <string.h>
#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "bme280";

/* The longest burst is the 26-byte calibration block. Without DMA the SPI
hardware FIFO caps a frame at 64 bytes, which is plenty. */
#define MAX_READ 26

static spi_device_handle_t s_dev;

/* Factory calibration, read once at startup. The raw ADC values mean nothing
without it — every formula below is built from these coefficients. */
static struct {
  uint16_t t1;
  int16_t t2, t3;
  uint16_t p1;
  int16_t p2, p3, p4, p5, p6, p7, p8, p9;
  uint8_t h1, h3;
  int16_t h2, h4, h5;
  int8_t h6;
} s_cal;

static esp_err_t reg_read(uint8_t reg, uint8_t* buf, size_t n) {
  /* Frame: CS low · address byte with bit 7 = 1 · n clocked-out bytes · CS high.
  SPI is full duplex, so one byte comes back opposite the address — it is junk
  and gets dropped. The address counter inside the sensor advances by itself,
  which is what makes a burst possible. */
  uint8_t tx[1 + MAX_READ] = {0};
  uint8_t rx[1 + MAX_READ] = {0};
  spi_transaction_t t = {.length = (1 + n) * 8, .tx_buffer = tx, .rx_buffer = rx};

  if (n == 0 || n > MAX_READ) {
    return ESP_ERR_INVALID_ARG;
  }

  tx[0] = (uint8_t)(reg | 0x80u);
  esp_err_t err = spi_device_transmit(s_dev, &t);
  if (err != ESP_OK) {
    return err;
  }

  memcpy(buf, &rx[1], n);
  return ESP_OK;
}

static esp_err_t reg_write(uint8_t reg, uint8_t val) {
  /* Write: bit 7 of the address cleared, then the value. Nothing to read back. */
  uint8_t tx[2] = {(uint8_t)(reg & 0x7Fu), val};
  spi_transaction_t t = {.length = 16, .tx_buffer = tx};

  return spi_device_transmit(s_dev, &t);
}

static uint16_t u16le(const uint8_t* p) {
  return (uint16_t)(p[0] | (p[1] << 8));
}

static int16_t s16le(const uint8_t* p) {
  return (int16_t)u16le(p);
}

static esp_err_t read_calibration(void) {
  uint8_t tp[26]; /* 0x88...0xA1 */
  uint8_t h[7];   /* 0xE1...0xE7 */
  esp_err_t err = reg_read(BME280_REG_CALIB_T_P, tp, sizeof(tp));
  if (err != ESP_OK) {
    return err;
  }

  err = reg_read(BME280_REG_CALIB_H, h, sizeof(h));
  if (err != ESP_OK) {
    return err;
  }

  /* Every word is little-endian, T1 and P1 unsigned, the rest signed. */
  s_cal.t1 = u16le(&tp[0]);
  s_cal.t2 = s16le(&tp[2]);
  s_cal.t3 = s16le(&tp[4]);
  s_cal.p1 = u16le(&tp[6]);
  s_cal.p2 = s16le(&tp[8]);
  s_cal.p3 = s16le(&tp[10]);
  s_cal.p4 = s16le(&tp[12]);
  s_cal.p5 = s16le(&tp[14]);
  s_cal.p6 = s16le(&tp[16]);
  s_cal.p7 = s16le(&tp[18]);
  s_cal.p8 = s16le(&tp[20]);
  s_cal.p9 = s16le(&tp[22]);
  s_cal.h1 = tp[25]; /* 0xA0 is reserved, H1 sits in the last byte */

  s_cal.h2 = s16le(&h[0]);
  s_cal.h3 = h[2];
  /* H4 and H5 are signed 12-bit values sharing the nibbles of 0xE5. */
  s_cal.h4 = (int16_t)(((int16_t)(int8_t)h[3] * 16) | (h[4] & 0x0F));
  s_cal.h5 = (int16_t)(((int16_t)(int8_t)h[5] * 16) | (h[4] >> 4));
  s_cal.h6 = (int8_t)h[6];

  ESP_LOGI(TAG, "calibration read: T1=%u P1=%u H1=%u", s_cal.t1, s_cal.p1, s_cal.h1);
  return ESP_OK;
}

/* The three functions below are the integer compensation formulas from
datasheet 4.2.3, only the names are shortened. Temperature has to be done
first: it produces t_fine, the shared fine-resolution temperature that both
pressure and humidity need. */

/* Returns hundredths of a degree Celsius. */
static int32_t compensate_temp(int32_t adc_t, int32_t* t_fine) {
  int32_t var1 = ((((adc_t >> 3) - ((int32_t)s_cal.t1 << 1))) * ((int32_t)s_cal.t2)) >> 11;
  int32_t var2 = (((((adc_t >> 4) - ((int32_t)s_cal.t1)) * ((adc_t >> 4) - ((int32_t)s_cal.t1))) >>
                   12) *
                  ((int32_t)s_cal.t3)) >>
                 14;

  *t_fine = var1 + var2;
  return (*t_fine * 5 + 128) >> 8;
}

/* Returns pressure in Q24.8 pascals: the value divided by 256 is Pa. */
static uint32_t compensate_press(int32_t adc_p, int32_t t_fine) {
  int64_t var1, var2, p;

  var1 = (int64_t)t_fine - 128000;
  var2 = var1 * var1 * (int64_t)s_cal.p6;
  var2 = var2 + ((var1 * (int64_t)s_cal.p5) << 17);
  var2 = var2 + (((int64_t)s_cal.p4) << 35);
  var1 = ((var1 * var1 * (int64_t)s_cal.p3) >> 8) + ((var1 * (int64_t)s_cal.p2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)s_cal.p1) >> 33;
  if (var1 == 0) {
    return 0; /* would divide by zero */
  }

  p = 1048576 - adc_p;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)s_cal.p9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)s_cal.p8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((int64_t)s_cal.p7) << 4);
  return (uint32_t)p;
}

/* Returns humidity in Q22.10 percent: the value divided by 1024 is %RH. */
static uint32_t compensate_hum(int32_t adc_h, int32_t t_fine) {
  int32_t v = t_fine - 76800;

  v = (((((adc_h << 14) - (((int32_t)s_cal.h4) << 20) - (((int32_t)s_cal.h5) * v)) + 16384) >> 15) *
       (((((((v * ((int32_t)s_cal.h6)) >> 10) *
            (((v * ((int32_t)s_cal.h3)) >> 11) + 32768)) >>
           10) +
          2097152) *
             ((int32_t)s_cal.h2) +
         8192) >>
        14));
  v = v - ((((((v >> 15) * (v >> 15)) >> 7) * ((int32_t)s_cal.h1)) >> 4));
  if (v < 0) {
    v = 0;
  }
  if (v > 419430400) {
    v = 419430400; /* 100 %RH in this fixed-point format */
  }

  return (uint32_t)(v >> 12);
}

esp_err_t bme280_init(void) {
  uint8_t id = 0;
  spi_bus_config_t bus = {
      .sclk_io_num = BME_SCLK_GPIO,
      .mosi_io_num = BME_MOSI_GPIO,
      .miso_io_num = BME_MISO_GPIO,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
  };
  spi_device_interface_config_t dev = {
      .mode = BME_MODE,
      .clock_speed_hz = BME_HZ,
      .spics_io_num = BME_CS_GPIO, /* the driver drives CS for every frame */
      .queue_size = 1,
  };

  /* Frames are a few bytes long, so no DMA: the driver pushes them through the FIFO. */
  esp_err_t err = spi_bus_initialize(BME_HOST, &bus, SPI_DMA_DISABLED);
  if (err != ESP_OK) {
    return err;
  }

  err = spi_bus_add_device(BME_HOST, &dev, &s_dev);
  if (err != ESP_OK) {
    return err;
  }

  ESP_LOGI(TAG, "SPI SCLK=GPIO%d MOSI=GPIO%d MISO=GPIO%d CS=GPIO%d, %d Hz mode %d",
           (int)BME_SCLK_GPIO, (int)BME_MOSI_GPIO, (int)BME_MISO_GPIO, (int)BME_CS_GPIO, BME_HZ,
           BME_MODE);

  err = reg_read(BME280_REG_ID, &id, 1);
  if (err != ESP_OK) {
    return err;
  }

  if (id != BME280_CHIP_ID) {
    ESP_LOGE(TAG, "chip id 0x%02X, expected 0x%02X — check the wiring", id, BME280_CHIP_ID);
    return ESP_ERR_NOT_FOUND;
  }
  ESP_LOGI(TAG, "chip id 0x%02X", id);

  /* Soft reset, then wait while the chip copies calibration out of its NVM. */
  err = reg_write(BME280_REG_RESET, BME280_RESET_WORD);
  if (err != ESP_OK) {
    return err;
  }
  vTaskDelay(pdMS_TO_TICKS(10));

  err = read_calibration();
  if (err != ESP_OK) {
    return err;
  }

  /* Humidity oversampling x1. Takes effect only on the next ctrl_meas write,
  so ctrl_meas must come last. */
  err = reg_write(BME280_REG_CTRL_HUM, 0x01);
  if (err != ESP_OK) {
    return err;
  }

  /* 500 ms standby between samples, IIR filter off. */
  err = reg_write(BME280_REG_CONFIG, 0x80);
  if (err != ESP_OK) {
    return err;
  }

  /* Temperature x1, pressure x1, normal mode — the sensor now measures on its own. */
  err = reg_write(BME280_REG_CTRL_MEAS, 0x27);
  if (err != ESP_OK) {
    return err;
  }

  /* At x1 one measurement takes under 10 ms; wait so the first read is real data. */
  vTaskDelay(pdMS_TO_TICKS(20));
  ESP_LOGI(TAG, "normal mode, oversampling x1, 500 ms standby");
  return ESP_OK;
}

esp_err_t bme280_read(bme280_data_t* out) {
  uint8_t d[8];
  int32_t t_fine = 0;
  esp_err_t err = reg_read(BME280_REG_DATA, d, sizeof(d));
  if (err != ESP_OK) {
    return err;
  }

  /* Pressure and temperature are 20-bit, left-aligned over three bytes;
  humidity is 16-bit over two. */
  int32_t adc_p = (int32_t)(((uint32_t)d[0] << 12) | ((uint32_t)d[1] << 4) | (d[2] >> 4));
  int32_t adc_t = (int32_t)(((uint32_t)d[3] << 12) | ((uint32_t)d[4] << 4) | (d[5] >> 4));
  int32_t adc_h = (int32_t)(((uint32_t)d[6] << 8) | d[7]);

  out->temperature = compensate_temp(adc_t, &t_fine) / 100.0f;
  out->pressure = compensate_press(adc_p, t_fine) / 25600.0f; /* /256 → Pa, /100 → hPa */
  out->humidity = compensate_hum(adc_h, t_fine) / 1024.0f;
  return ESP_OK;
}
