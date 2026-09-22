#pragma once

#include <Arduino.h>

#define CONTROL_PIN 5
#define SENSE_PIN 9

#define SERIAL_BAUD 115200

const int SAMPLE_COUNT = 10;
const uint32_t DELAY_MS = 500;
const uint32_t DELAY_MICROS = DELAY_MS * 1000UL;
