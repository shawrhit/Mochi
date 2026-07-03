// touch_input.h — Debounced single-button capacitive touch input.

#pragma once

class TouchInput {
 public:
  TouchInput();

  void begin(int pin);
  void update();
  bool wasTapped();
  bool isPressed() const;
  unsigned long pressedMs() const;

 private:
  int pin_;
  bool rawState_;
  bool currentState_;
  unsigned long lastDebounceTime_;
  unsigned long pressStartTime_;
  unsigned long releaseStartTime_;
  bool tapped_;
};
