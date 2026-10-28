#include <Arduino.h>

#define RED_LED_PIN 5
#define YELLOW_LED_PIN 15
#define BUTTON_PIN 18
#define DEBOUNCE_MS 30

bool ledState = false; // true is LED ON, false is LED OFF
bool lastReading = HIGH; // last reading from the button pin
uint32_t lastEdgeTime = 0; // the last time the button pin was toggled


const uint32_t blinkDelays[] = {1000, 500, 200, 50};
const uint8_t numModes = sizeof(blinkDelays) / sizeof(blinkDelays[0]);
uint8_t modeIndex = 0;

uint32_t blinkingDelayMs = blinkDelays[modeIndex];

void setup() {
  Serial.begin(115200);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  digitalWrite(YELLOW_LED_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void checkButton(uint32_t delayMs) {
  for (uint32_t i = 0; i < delayMs; i += 2) {
    bool reading = digitalRead(BUTTON_PIN);

    // check the external button state and debounce it
    if (reading != lastReading && (millis() - lastEdgeTime) >= DEBOUNCE_MS) {
      lastEdgeTime = millis();
      lastReading  = reading;

      if (reading == LOW) {              // HIGH -> LOW == press
          modeIndex = (modeIndex + 1) % numModes;
          blinkingDelayMs = blinkDelays[modeIndex];
          Serial.println("Button pressed");
        }
    }

    delay(2); // wait 2 ms before next reading
  }

}
  

void loop() {

  digitalWrite(YELLOW_LED_PIN, HIGH);
  // delay(delayMs);
  checkButton(blinkingDelayMs);
  digitalWrite(YELLOW_LED_PIN, LOW);

  digitalWrite(RED_LED_PIN, HIGH);
  // delay(delayMs);
  checkButton(blinkingDelayMs);
  digitalWrite(RED_LED_PIN, LOW);                                                   

}

