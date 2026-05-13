// animation_player.h — Frame-by-frame OLED bitmap animation player.

#pragma once

#include <Adafruit_SSD1306.h>
#include "animation_catalog.h"

class AnimationPlayer {
 public:
  explicit AnimationPlayer(Adafruit_SSD1306& display);

  void start(const Animation& anim, int frameDelayMs);
  bool update();
  bool isActive() const;

 private:
  Adafruit_SSD1306& display_;
  const Animation* currentAnim_;
  int frameDelayMs_;
  int currentFrame_;
  unsigned long lastFrameTime_;
  bool active_;
};
