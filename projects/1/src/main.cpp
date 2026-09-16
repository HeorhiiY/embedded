#include <Arduino.h>

const int PIN = 9;
const int ADC_PIN = 7;
// night values
// const int FLOOR = 800;
// const int CEILING = 1000;
// day values
const int FLOOR = 2500;
const int CEILING = 3000;
// base voltage applied to the transistor when switching on — tune this
float volts = 3.2;

void setVolts(float v) {
  int duty = (v / 3.3) * 255;   // 3.3 V = full scale
  analogWrite(PIN, duty);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN, OUTPUT);
  pinMode(ADC_PIN, INPUT);
  setVolts(0.0);
}

void loop() {
  int raw = analogRead(ADC_PIN);
  Serial.println(raw);

  if (raw < FLOOR) {
    setVolts(volts);
    Serial.println("Switch on the transistor");
  } else if (raw > CEILING) {
    setVolts(0.0);
    Serial.println("Switch off the transistor");
  }
  // in between: leave the output as is

  delay(50);
}
