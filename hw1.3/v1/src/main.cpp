#include <Arduino.h>

#define LED_PIN 5
#define delayMs 1000

void setup() {
  pinMode(LED_PIN, OUTPUT);
  // set the pin to low
  digitalWrite(LED_PIN, LOW);

}

void loop() {

  digitalWrite(LED_PIN, HIGH);
  delay(delayMs);
  digitalWrite(LED_PIN, LOW);
  delay(delayMs);


}

