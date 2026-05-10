#include "touch_input.h"

#include <Arduino.h>

TouchInput::TouchInput()
    : pin_(-1),
      lastReadState_(HIGH),
      stableState_(HIGH),
      touchLatched_(false),
  lastDebounceMs_(0),
  pressStartMs_(0),
  tapped_(false) {}

void TouchInput::begin(int pin) {
  pin_ = pin;
  pinMode(pin_, INPUT_PULLUP);
  lastReadState_ = digitalRead(pin_);
  stableState_ = lastReadState_;
  touchLatched_ = false;
  lastDebounceMs_ = millis();
  pressStartMs_ = 0;
  tapped_ = false;
}

void TouchInput::update() {
  if (pin_ < 0) {
    return;
  }

  const bool state = digitalRead(pin_);
  const unsigned long now = millis();

  if (state != lastReadState_) {
    lastDebounceMs_ = now;
    lastReadState_ = state;
  }

  if (now - lastDebounceMs_ < 30) {
    return;
  }

  if (stableState_ != state) {
    stableState_ = state;
    if (stableState_ == LOW) {
      pressStartMs_ = now;
      touchLatched_ = false;
      tapped_ = true;
    } else {
      pressStartMs_ = 0;
      touchLatched_ = false;
    }
  }
}

bool TouchInput::wasTapped() {
  if (tapped_) {
    tapped_ = false;
    return true;
  }
  return false;
}

bool TouchInput::isPressed() const { return stableState_ == LOW; }

unsigned long TouchInput::pressedMs() const {
  if (stableState_ != LOW || pressStartMs_ == 0) {
    return 0;
  }
  return millis() - pressStartMs_;
}
