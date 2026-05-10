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
  bool lastReadState_;
  bool stableState_;
  bool touchLatched_;
  unsigned long lastDebounceMs_;
  unsigned long pressStartMs_;
  bool tapped_;
};
