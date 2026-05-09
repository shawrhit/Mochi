#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "animation_catalog.h"
#include "animation_player.h"
#include "sound_manager.h"
#include "ui_renderer.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

constexpr int kBuzzerPin = 2;
constexpr int kAnimationFrameDelayMs = 200;  // 5 fps
constexpr int kGreetingAnimationIndex = 5;  // greeting
constexpr unsigned long kBootHoldMs = 2000;
constexpr unsigned long kClockRefreshMs = 500;

const char kGreetingMelody[] =
  "G4 200 40, C5 200 40, E5 200 40, G5 200 40, C6 200 40, D6 200 40, E6 400 400";

AnimationPlayer animationPlayer(display);
UiRenderer ui(display);
SoundManager soundManager;

enum class ScreenMode {
  Animation,
  Clock
};

ScreenMode screenMode = ScreenMode::Animation;
unsigned long lastClockDraw = 0;

String uptimeString() {
  const unsigned long totalSeconds = millis() / 1000;
  const unsigned long hours = (totalSeconds / 3600) % 24;
  const unsigned long minutes = (totalSeconds / 60) % 60;
  const unsigned long seconds = totalSeconds % 60;

  char buf[9];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", hours, minutes, seconds);
  return String(buf);
}

void drawClock() {
  ui.showClockScreen(uptimeString(), "UPTIME", false, false);
}

void enterAnimationMode(int playlistIndex) {
  if (playlistIndex < 0 || playlistIndex >= kPlaylistCount) {
    return;
  }

  animationPlayer.start(kPlaylist[playlistIndex], kAnimationFrameDelayMs);
  screenMode = ScreenMode::Animation;
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println("shaws.systems");

  delay(kBootHoldMs);

  Wire.begin(21, 20);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED failed to initialize! Check wiring."));
    for (;;) {}
  }

  display.clearDisplay();
  display.display();

  soundManager.begin(kBuzzerPin);
  soundManager.startMelody(kGreetingMelody);
  enterAnimationMode(kGreetingAnimationIndex);
}

void loop() {
  const unsigned long now = millis();
  soundManager.updateMelody();

  switch (screenMode) {
    case ScreenMode::Animation:
      if (animationPlayer.update()) {
        screenMode = ScreenMode::Clock;
        drawClock();
      }
      break;

    case ScreenMode::Clock:
      if (now - lastClockDraw >= kClockRefreshMs) {
        lastClockDraw = now;
        drawClock();
      }
      break;
  }
}
