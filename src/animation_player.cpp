// animation_player.cpp — Frame-by-frame OLED bitmap animation player.

#include "animation_player.h"

#include <Arduino.h>

AnimationPlayer::AnimationPlayer(Adafruit_SSD1306& display)
    : display_(display),
      currentAnim_(nullptr),
      frameDelayMs_(200),
      currentFrame_(0),
      lastFrameTime_(0),
      active_(false) {}

void AnimationPlayer::start(const Animation& anim, int frameDelayMs) {
  currentAnim_ = &anim;
  frameDelayMs_ = frameDelayMs;
  currentFrame_ = 0;
  lastFrameTime_ = 0;
  active_ = true;
}

bool AnimationPlayer::update() {
  if (!active_ || currentAnim_ == nullptr || currentAnim_->frameCount <= 0) {
    return true;
  }

  const unsigned long now = millis();
  if (lastFrameTime_ != 0 && now - lastFrameTime_ < static_cast<unsigned long>(frameDelayMs_)) {
    return false;
  }

  lastFrameTime_ = now;

  display_.clearDisplay();
  display_.drawBitmap(0, 0, currentAnim_->frames[currentFrame_], 128, 64, SSD1306_WHITE);
  display_.display();

  currentFrame_++;
  if (currentFrame_ >= currentAnim_->frameCount) {
    active_ = false;
    return true;
  }

  return false;
}

bool AnimationPlayer::isActive() const { return active_; }
