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
  bool currentState_;
  bool lastState_;
  unsigned long lastDebounceTime_;
  unsigned long pressStartTime_;
  bool tapped_;
};
