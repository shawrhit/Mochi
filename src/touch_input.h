#pragma once

class TouchInput {
 public:
  TouchInput();

  void begin(int pin);
  bool wasTouched();

 private:
  int pin_;
  bool lastReadState_;
  bool stableState_;
  bool touchLatched_;
  unsigned long lastDebounceMs_;
};
