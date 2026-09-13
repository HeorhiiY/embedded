#include <Arduino.h>

#define LDR_PIN 9

const uint32_t ADC_MAX = 4095; 
const uint32_t U_REF = 3100;

void setupAdc() { 
  analogReadResolution(12);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);
}

void setup() {
  Serial.begin(115200);
  setupAdc();
}

void loop() {
  uint32_t raw = analogRead(LDR_PIN);
  
  float voltage = (raw * U_REF) / ADC_MAX*1000; // in mV
  
  uint32_t voltage_naive_mv = raw * U_REF / ADC_MAX;

  uint32_t voltage_cal_mv = analogReadMilliVolts(LDR_PIN);
  int32_t diff_mv   = (int32_t)voltage_cal_mv - (int32_t)voltage_naive_mv;
  float   error_pct = (float)diff_mv / (float)voltage_cal_mv * 100.0f;

  Serial.printf("raw=%4d  naive=%4u mV  cal=%4u mV  err=%+6.2f %%\n",
                raw, voltage_naive_mv, voltage_cal_mv, error_pct);
  delay(500);
}