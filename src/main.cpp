#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "animation_catalog.h"
#include "animation_player.h"
#include "ble_notifier.h"
#include "sound_manager.h"
#include "time_service.h"
#include "touch_input.h"
#include "ui_renderer.h"
#include "weather_service.h"
#include "wifi_portal.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

constexpr int kBuzzerPin = 2;
constexpr int kTouchPin = 10;
constexpr int kAnimationFrameDelayMs = 200;  // 5 fps
constexpr int kGreetingAnimationIndex = 5;  // greeting
constexpr unsigned long kBootHoldMs = 2000;
constexpr unsigned long kClockRefreshMs = 500;
constexpr unsigned long kIdleToFirstAnimationMs = 10000;
constexpr unsigned long kMinIdleToAnimationMs = 10000;
constexpr unsigned long kMaxIdleToAnimationMs = 30000;
constexpr unsigned long kNotificationDisplayMs = 5000;
constexpr unsigned long kLongPressMs = 1200;
constexpr unsigned long kBootTouchWindowMs = 3500;

const char kGreetingMelody[] =
  "G4 200 40, C5 200 40, E5 200 40, G5 200 40, C6 200 40, D6 200 40, E6 400 400";

AnimationPlayer animationPlayer(display);
UiRenderer ui(display);
SoundManager soundManager;
WifiPortal wifiPortal;
TimeService timeService;
WeatherService weatherService;
BleNotifier bleNotifier;
TouchInput touchInput;

enum class ScreenMode {
  Animation,
  Clock,
  Notification,
  Portal
};

ScreenMode screenMode = ScreenMode::Animation;
unsigned long lastClockDraw = 0;
unsigned long lastInteractionMs = 0;
unsigned long nextAnimationAtMs = 0;
unsigned long notificationUntilMs = 0;
bool longPressHandled = false;

String uptimeString() {
  const unsigned long totalSeconds = millis() / 1000;
  const unsigned long hours = (totalSeconds / 3600) % 24;
  const unsigned long minutes = (totalSeconds / 60) % 60;
  const unsigned long seconds = totalSeconds % 60;

  char buf[9];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", hours, minutes, seconds);
  return String(buf);
}

String activeTimeString() {
  if (timeService.isSynced()) {
    return timeService.timeString();
  }
  return uptimeString();
}

String activeDateString() {
  if (timeService.isSynced()) {
    return timeService.dateString();
  }
  return "Syncing";
}

String activeDayString() {
  if (timeService.isSynced()) {
    return timeService.dayString();
  }
  return "--";
}

String activeDateShortString() {
  if (timeService.isSynced()) {
    return timeService.dateShortString();
  }
  return "-- ---";
}

String activeYearString() {
  if (timeService.isSynced()) {
    return timeService.yearString();
  }
  return "----";
}

bool isNightTime() {
  if (!timeService.isSynced()) {
    return false;
  }
  const int hour = timeService.hour24();
  return hour < 6 || hour >= 22;
}

void drawClock() {
  const int tempC = weatherService.temperatureC();
  const int windKph = weatherService.windKph();
  const int humidity = weatherService.humidityPercent();
  const String tempText = tempC >= 0 ? String(tempC) + "C" : String("--C");
  const String windText = windKph >= 0 ? String(windKph) + " km/h" : String("-- km/h");
  const String humidityText = humidity >= 0 ? String(humidity) + "%" : String("--%");

  String ampm = " AM";
  if (timeService.isSynced()) {
    ampm = timeService.hour24() >= 12 ? " PM" : " AM";
  }

  String weatherDesc = "Sunny";
  int code = weatherService.weatherCode();
  if (code >= 1 && code <= 3) weatherDesc = "Cloudy";
  else if (code >= 45) weatherDesc = "Rain";
  
  const String weatherText = tempText + " | " + weatherDesc;
  const String dateText = activeDayString() + ", " + activeDateShortString();

  String timeTxt = activeTimeString();
  if (timeTxt.length() > 5) {
    timeTxt = timeTxt.substring(0, 5);
  }

  ui.showClockScreen(
      timeTxt,
      dateText,
      weatherText,
      wifiPortal.city(),
      ampm,
      code,
      isNightTime(),
      wifiPortal.isWifiConnected(),
      bleNotifier.isConnected());
}

void enterAnimationMode(int playlistIndex) {
  if (playlistIndex < 0 || playlistIndex >= kPlaylistCount) {
    return;
  }

  animationPlayer.start(kPlaylist[playlistIndex], kAnimationFrameDelayMs);
  screenMode = ScreenMode::Animation;
}

void enterRandomAnimation() {
  const int index = random(0, kPlaylistCount);
  enterAnimationMode(index);
}

void registerInteraction(unsigned long now) {
  lastInteractionMs = now;
  nextAnimationAtMs = now + random(kMinIdleToAnimationMs, kMaxIdleToAnimationMs);
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

  touchInput.begin(kTouchPin);
  const unsigned long bootStart = millis();
  while (millis() - bootStart < kBootTouchWindowMs) {
    touchInput.update();
    if (touchInput.isPressed() && touchInput.pressedMs() >= kLongPressMs) {
      wifiPortal.markSkipWifi();
      break;
    }
    delay(10);
  }

  wifiPortal.begin();
  if (wifiPortal.isApMode()) {
    screenMode = ScreenMode::Portal;
    ui.showPortalScreen("shaws.systems", wifiPortal.localIp());
  }
  Serial.print("WiFi: status ");
  Serial.println(wifiPortal.isWifiConnected() ? "connected" : "not connected");
  timeService.setTimezone("IST-5:30");
  timeService.begin(wifiPortal.isWifiConnected());
  weatherService.begin(wifiPortal.city());
  bleNotifier.begin();

  soundManager.begin(kBuzzerPin);
  soundManager.startMelody(kGreetingMelody);
  enterAnimationMode(kGreetingAnimationIndex);
  lastInteractionMs = millis();
  nextAnimationAtMs = lastInteractionMs + kIdleToFirstAnimationMs;
}

void loop() {
  const unsigned long now = millis();
  soundManager.updateMelody();
  wifiPortal.handle();
  timeService.trySync(wifiPortal.isWifiConnected());
  weatherService.setCity(wifiPortal.city());
  weatherService.update(wifiPortal.isWifiConnected());

  touchInput.update();
  if (touchInput.wasTapped()) {
    registerInteraction(now);
    if (screenMode == ScreenMode::Clock) {
      enterRandomAnimation();
    } else {
      screenMode = ScreenMode::Clock;
      drawClock();
    }
  }

  if (touchInput.isPressed() && !longPressHandled && touchInput.pressedMs() >= kLongPressMs) {
    longPressHandled = true;
    registerInteraction(now);
    screenMode = ScreenMode::Portal;
    ui.showPortalScreen("shaws.systems", wifiPortal.localIp());
  }

  if (!touchInput.isPressed()) {
    longPressHandled = false;
  }

  if (bleNotifier.hasNewNotification()) {
    const String message = bleNotifier.takeLatestNotification();
    ui.showNotificationScreen(message);
    screenMode = ScreenMode::Notification;
    notificationUntilMs = now + kNotificationDisplayMs;
    registerInteraction(now);
  }

  switch (screenMode) {
    case ScreenMode::Animation:
      if (animationPlayer.update()) {
        screenMode = ScreenMode::Clock;
        drawClock();
        nextAnimationAtMs = now + random(kMinIdleToAnimationMs, kMaxIdleToAnimationMs);
      }
      break;

    case ScreenMode::Clock:
      if (now - lastClockDraw >= kClockRefreshMs) {
        lastClockDraw = now;
        drawClock();
      }
      if (now - lastInteractionMs >= kIdleToFirstAnimationMs && now >= nextAnimationAtMs) {
        enterRandomAnimation();
      }
      break;

    case ScreenMode::Notification:
      if (now >= notificationUntilMs) {
        screenMode = ScreenMode::Clock;
        drawClock();
      }
      break;

    case ScreenMode::Portal:
      if (now - lastInteractionMs >= kIdleToFirstAnimationMs) {
        screenMode = ScreenMode::Clock;
        drawClock();
      }
      break;
  }
}
