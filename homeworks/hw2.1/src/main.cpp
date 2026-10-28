#include <Arduino.h>

namespace config{
  constexpr uint8_t LED_PIN = 6;
  constexpr uint16_t delayMs = 500;
}

uint32_t lastMs = 0;

enum class LedState : uint8_t {
  Off = LOW,
  On  = HIGH
};

class Led {
public:
  explicit Led(uint8_t pin) : pin_(pin) {}

  void init() {
    pinMode(pin_, OUTPUT);
    set(LedState::Off);
  }

  void set(LedState state) {
    digitalWrite(pin_, static_cast<uint8_t>(state));
    state_ = state;
  }

  void toggle() {
    set(state_ == LedState::On ? LedState::Off : LedState::On);
  }

  LedState state() const { return state_; }

private:
  uint8_t pin_;
  LedState state_ = LedState::Off;
};


Led led(config::LED_PIN);

void setup() {
  led.init();
}


void loop() {
  uint32_t now = millis();
  if (now - lastMs >= config::delayMs) {
    lastMs = now;
    led.toggle();
  }
}
