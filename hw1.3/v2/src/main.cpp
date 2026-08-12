#include <Arduino.h>

#define LED_RED_PIN 5
#define LED_BLUE_PIN 18


void setup() {
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  // set both pins to low
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_BLUE_PIN, LOW);

}

void loop() {
  // blink with varying frequency, gradually from 1 second to 50 milliseconds, then from 50 to 10 ms
  for (int delayMs = 1000; delayMs >= 50; delayMs -= 50) {
    digitalWrite(LED_RED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_BLUE_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_BLUE_PIN, LOW);
  }
  for (int delayMs = 50; delayMs >= 10; delayMs -= 5) {
    digitalWrite(LED_RED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_RED_PIN, LOW);
    digitalWrite(LED_BLUE_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_BLUE_PIN, LOW);
  }
}

