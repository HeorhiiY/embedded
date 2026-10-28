#include <Arduino.h>

const int RED_PIN = 4;
const int YELLOW_PIN = 7;
const int BLUE_PIN = 8;

const uint32_t RED_BLINK_MS = 200;
const uint32_t YELLOW_BLINK_MS = 500;
const uint32_t BLUE_BLINK_MS = 1000;

uint32_t last_blink_time_red = 0;
uint32_t last_blink_time_yellow = 0;
uint32_t last_blink_time_blue = 0;
uint32_t now = 0;

void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  digitalWrite(RED_PIN, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
}

void loop() {
  now = millis();
  if (now - last_blink_time_red >= RED_BLINK_MS) {
    last_blink_time_red = now;
    digitalWrite(RED_PIN, !digitalRead(RED_PIN));
  }
  if (now - last_blink_time_yellow >= YELLOW_BLINK_MS) {
    last_blink_time_yellow = now;
    digitalWrite(YELLOW_PIN, !digitalRead(YELLOW_PIN));
  }
  if (now - last_blink_time_blue >= BLUE_BLINK_MS) {
    last_blink_time_blue = now;
    digitalWrite(BLUE_PIN, !digitalRead(BLUE_PIN));
  }
}

