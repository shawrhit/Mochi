#include "touch_input.h"
#include <Arduino.h>

TouchInput::TouchInput()
    : pin_(-1),
      currentState_(HIGH),
      lastState_(HIGH),
      lastDebounceTime_(0),
      pressStartTime_(0),
      tapped_(false) {}

void TouchInput::begin(int pin) {
  pin_ = pin;
  pinMode(pin_, INPUT_PULLUP);
  currentState_ = digitalRead(pin_);
  lastState_ = currentState_;
  lastDebounceTime_ = millis();
  pressStartTime_ = 0;
  tapped_ = false;
}

void TouchInput::update() {
  if (pin_ < 0) return;

  int reading = digitalRead(pin_);
  unsigned long now = millis();

  if (reading != lastState_) {
    lastDebounceTime_ = now;
  }

  if ((now - lastDebounceTime_) > 50) {
    if (reading != currentState_) {
      currentState_ = reading;

      if (currentState_ == LOW) {
        pressStartTime_ = now;
        tapped_ = false;
      } else {
        if (pressStartTime_ > 0) {
          unsigned long duration = now - pressStartTime_;
          if (duration < 2000) {
            tapped_ = true;
          }
        }
        pressStartTime_ = 0;
      }
    }
  }

  lastState_ = reading;
}

bool TouchInput::wasTapped() {
  if (tapped_) {
    tapped_ = false;
    return true;
  }
  return false;
}

bool TouchInput::isPressed() const {
  return currentState_ == LOW;
}

unsigned long TouchInput::pressedMs() const {
  if (currentState_ != LOW || pressStartTime_ == 0) {
    return 0;
  }
  return millis() - pressStartTime_;
}
