// touch_input.cpp — Debounced single-button capacitive touch input.
// Short tap: release within 2 seconds. Long press: hold >= 2 seconds.

#include "touch_input.h"
#include <Arduino.h>

namespace {
constexpr unsigned long kDebounceMs = 35;
constexpr unsigned long kReleaseGraceMs = 120;
constexpr unsigned long kTapMaxMs = 2000;
}  // namespace

TouchInput::TouchInput()
    : pin_(-1),
      rawState_(LOW),
      currentState_(LOW),
      lastDebounceTime_(0),
      pressStartTime_(0),
      releaseStartTime_(0),
      tapped_(false) {}

void TouchInput::begin(int pin) {
  pin_ = pin;
  pinMode(pin_, INPUT);
  rawState_ = digitalRead(pin_);
  currentState_ = rawState_;
  lastDebounceTime_ = millis();
  pressStartTime_ = currentState_ == HIGH ? lastDebounceTime_ : 0;
  releaseStartTime_ = 0;
  tapped_ = false;
}

void TouchInput::update() {
  if (pin_ < 0) return;

  const bool reading = digitalRead(pin_) == HIGH;
  const unsigned long now = millis();

  if (reading != rawState_) {
    rawState_ = reading;
    lastDebounceTime_ = now;
    if (reading == HIGH) {
      releaseStartTime_ = 0;
    }
  }

  if (rawState_ == currentState_ || now - lastDebounceTime_ < kDebounceMs) {
    return;
  }

  if (rawState_ == HIGH) {
    currentState_ = HIGH;
    pressStartTime_ = lastDebounceTime_;
    releaseStartTime_ = 0;
    tapped_ = false;
    return;
  }

  if (releaseStartTime_ == 0) {
    releaseStartTime_ = lastDebounceTime_;
  }

  if (now - releaseStartTime_ < kReleaseGraceMs) {
    return;
  }

  currentState_ = LOW;
  if (pressStartTime_ > 0) {
    const unsigned long duration = releaseStartTime_ - pressStartTime_;
    if (duration < kTapMaxMs) {
      tapped_ = true;
    }
  }
  pressStartTime_ = 0;
  releaseStartTime_ = 0;
}

bool TouchInput::wasTapped() {
  if (tapped_) {
    tapped_ = false;
    return true;
  }
  return false;
}

bool TouchInput::isPressed() const {
  return currentState_ == HIGH;
}

unsigned long TouchInput::pressedMs() const {
  if (currentState_ != HIGH || pressStartTime_ == 0) {
    return 0;
  }
  return millis() - pressStartTime_;
}
