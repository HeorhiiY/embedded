# HW4.4: Reading data from SPI - BME280

This project reads temperature, pressure and humidity from a BME280 over SPI and prints them to the log once a second. The sensor does the measuring on its own (normal mode), the ESP32-S3 only picks up the latest sample.

## Wiring

| BME280 | ESP32-S3 | SPI name |
| ------ | -------- | -------- |
| SCL    | GPIO8    | SCLK     |
| SDA    | GPIO9    | MOSI     |
| SDO    | GPIO10   | MISO     |
| CSB    | GPIO11   | CS       |

1 MHz, mode 0 (CPOL=0, CPHA=0), SPI2 host.

## Log format

Roughly what a boot looks like (the calibration numbers differ per chip):

```
I (316) bme280: SPI SCLK=GPIO8 MOSI=GPIO9 MISO=GPIO10 CS=GPIO11, 1000000 Hz mode 0
I (326) bme280: chip id 0x60
I (336) bme280: calibration read: T1=28386 P1=37654 H1=75
I (356) bme280: normal mode, oversampling x1, 500 ms standby
I (1366) 4.4-bme280: T = 23.41 C   P = 1008.72 hPa   H = 41.30 %
```

## Setup

![setup](docs/setup.jpg)

## Demo
