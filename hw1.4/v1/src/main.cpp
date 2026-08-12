#include <Arduino.h>

#define RED_LED_PIN 5
#define YELLOW_LED_PIN 15
#define BUTTON_PIN 18
#define BOOT_BUTTON_PIN 0
#define DEBOUNCE_MS 30

bool ledState = false; // true is LED ON, false is LED OFF
bool lastReading = HIGH; // last reading from the button pin
bool lastReadingBoot = HIGH; // last reading from the boot button pin
uint32_t lastEdgeTime = 0; // the last time the button pin was toggled
uint32_t lastEdgeTimeBoot = 0; // the last time the boot button pin was toggled


uint32_t blinkingDelayMs = 1000;

uint32_t delayButtonMs = 1000; 
uint32_t delayBootButtonMs = 50; 

void setup() {
  Serial.begin(115200);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  digitalWrite(YELLOW_LED_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
}

void checkButton(uint32_t delayMs) {
  for (uint32_t i = 0; i < delayMs; i += 2) {
    bool reading = digitalRead(BUTTON_PIN);
    bool readingBoot = digitalRead(BOOT_BUTTON_PIN);

    // check the external button state and debounce it
    if (reading != lastReading && (millis() - lastEdgeTime) >= DEBOUNCE_MS) {
      lastEdgeTime = millis();
      lastReading  = reading;

      if (reading == LOW) {              // HIGH -> LOW == press
          blinkingDelayMs = delayButtonMs; // set the delay to 1 second
          Serial.println("Button pressed");
        }
    }
    // check the boot button state and debounce it
    if (readingBoot != lastReadingBoot && (millis() - lastEdgeTimeBoot) >= DEBOUNCE_MS) {
      lastEdgeTimeBoot = millis();
      lastReadingBoot  = readingBoot;

      if (readingBoot == LOW) {              // HIGH -> LOW == press
          blinkingDelayMs = delayBootButtonMs; // set the delay to 50 ms
          Serial.println("Boot button pressed");
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

