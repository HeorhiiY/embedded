#include <Arduino.h>

#include "config.h"

volatile uint32_t edge_counter = 0;  // volatile: modified inside the ISR
uint32_t last_count = 0; // tracks changes of the counter in the main loop
uint32_t switch_time = 0; // time of the switching
uint32_t sense_time = 0; // time of the last sense pin trigger
uint32_t first_edge_time = 0; // time of the first sense pin trigger

float mean_sense_time = 0; // mean sense pin trigger time
float mean_first_edge_time = 0; // mean first sense pin trigger time

// ISR: counts falling edges on the sense pin (1 to 0 transition)
void IRAM_ATTR sense_isr() {
  edge_counter++;
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(CONTROL_PIN, OUTPUT);
  digitalWrite(CONTROL_PIN, LOW);
  pinMode(SENSE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSE_PIN), sense_isr, FALLING);
}

void run_sample() {
  edge_counter = 0;
  last_count = 0; 
  first_edge_time = 0;
  switch_time = micros(); 
  sense_time = switch_time; 
  
  digitalWrite(CONTROL_PIN, HIGH); 

  while (micros() - switch_time < DELAY_MICROS) {
    if (edge_counter != last_count) {
      last_count = edge_counter;
      sense_time = micros();
      if (first_edge_time == 0) {
        first_edge_time = sense_time;
      }
    }
  }



  Serial.print("First edge detected at: ");
  Serial.print(first_edge_time - switch_time);
  Serial.println(" us after switching on the transistor");

  Serial.print("Last edge detected at: ");
  Serial.print(sense_time - switch_time);
  Serial.println(" us after switching on the transistor");

  digitalWrite(CONTROL_PIN, LOW);
  delay(DELAY_MS);
}
void loop() {
  mean_sense_time = 0;
  mean_first_edge_time = 0; 
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    Serial.print("Sample ");
    Serial.println(i + 1);
    run_sample();
    mean_sense_time += sense_time - switch_time;
    mean_first_edge_time += first_edge_time - switch_time;
    delay(DELAY_MS);
  }
  mean_sense_time /= SAMPLE_COUNT;
  mean_first_edge_time /= SAMPLE_COUNT;
  // ------------------------------------- //
  Serial.print("Mean sense time: ");
  Serial.print(mean_sense_time);
  Serial.println(" us");
  // ------------------------------------- //
  Serial.print("Mean first edge time: ");
  Serial.print(mean_first_edge_time);
  Serial.println(" us");
}
