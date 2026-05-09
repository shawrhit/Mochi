#include "touch_input.h"

#include <Arduino.h>

TouchInput::TouchInput()
    : pin_(-1),
      lastReadState_(HIGH),
      stableState_(HIGH),
      touchLatched_(false),
      lastDebounceMs_(0) {}

void TouchInput::begin(int pin) {
  pin_ = pin;
  pinMode(pin_, INPUT_PULLUP);
  lastReadState_ = digitalRead(pin_);
  stableState_ = lastReadState_;
  touchLatched_ = false;
  lastDebounceMs_ = millis();
}

bool TouchInput::wasTouched() {
  if (pin_ < 0) {
    return false;
  }

  const bool state = digitalRead(pin_);
  const unsigned long now = millis();

  if (state != lastReadState_) {
    lastDebounceMs_ = now;
    lastReadState_ = state;
  }

  if (now - lastDebounceMs_ < 30) {
    return false;
  }

  if (stableState_ != state) {
    stableState_ = state;
    if (stableState_ == HIGH) {
      touchLatched_ = false;
    }
  }

  if (stableState_ == LOW && !touchLatched_) {
    touchLatched_ = true;
    return true;
  }

  return false;
}
