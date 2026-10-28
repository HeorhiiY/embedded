# HW4.4: Reading data from SPI - BMP280

This project reads temperature and pressure from a BMP280 over SPI and prints them to the log once a second. The sensor does the measuring on its own (normal mode), the ESP32-S3 only picks up the latest sample.

## Wiring

| BMP280 | ESP32-S3 | SPI name |
| ------ | -------- | -------- |
| SCL    | GPIO8    | SCLK     |
| SDA    | GPIO9    | MOSI     |
| SDO    | GPIO10   | MISO     |
| CSB    | GPIO11   | CS       |

1 MHz, mode 0 (CPOL=0, CPHA=0), SPI2 host.

## Log format

Roughly what a boot looks like (the calibration numbers differ per chip):

```
I (316) bmp280: SPI SCLK=GPIO8 MOSI=GPIO9 MISO=GPIO10 CS=GPIO11, 1000000 Hz mode 0
I (326) bmp280: chip id 0x58
I (336) bmp280: calibration read: T1=28386 P1=37654
I (356) bmp280: normal mode, oversampling x1, 500 ms standby
I (1366) 4.4-bmp280: T = 23.41 C   P = 1008.72 hPa
```

## Setup

![setup](docs/setup.jpg)

## Demo
