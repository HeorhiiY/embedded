#include <Arduino.h>

// ESP32-S3 has no DAC, so we use PWM on pin 9.
// Add an RC filter (e.g. 10k + 1uF) to turn it into a real voltage.

const int PIN = 9;
const int ADC_PIN = 7;
const int FLOOR = 800;
const int CEILING = 1000;
float volts = 0.0;

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
  // setVolts(volts);
  // Serial.println(volts);

  // delay(1000);

  // volts = volts + 0.3;
  // if (volts > 3.0) {
  //   volts = 0.0;
  // }

  int raw = analogRead(ADC_PIN);
  Serial.println(raw);

  if (raw < FLOOR) {
    setVolts(1.0);
    Serial.println("Switch on the transistor");
  } else if (raw > CEILING) {
    setVolts(0.0);
    Serial.println("Switch off the transistor");
  }
  // in between: leave the output as is

  delay(50);
}
