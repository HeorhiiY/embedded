#include <Arduino.h>

const int PIN = 9;
const int ADC_PIN = 7;
// night values
// const int FLOOR = 800;
// const int CEILING = 1000;
// day values
const int FLOOR = 2500;
const int CEILING = 3000;
float volts = 0.0;

void setup() {
  Serial.begin(115200);
  pinMode(PIN, OUTPUT);
  pinMode(ADC_PIN, INPUT);
  digitalWrite(PIN, LOW);
}

void loop() {
  int raw = analogRead(ADC_PIN);
  Serial.println(raw);

  if (raw < FLOOR) {
    digitalWrite(PIN, HIGH);
    Serial.println("Switch on the transistor");
  } else if (raw > CEILING) {
    digitalWrite(PIN, LOW);
    Serial.println("Switch off the transistor");
  }
  // in between: leave the output as is

  delay(50);
}
