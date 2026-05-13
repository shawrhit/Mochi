#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "animation_catalog.h"
#include "animation_player.h"
#include <ChronosESP32.h>
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
constexpr unsigned long kNowPlayingDisplayMs = 5000;
constexpr unsigned long kCallMelodyGapMs = 1000;
constexpr unsigned long kLongPressMs = 1200;
constexpr unsigned long kBootTouchWindowMs = 3500;

const char kGreetingMelody[] =
  "G4 200 20, C5 200 20, E5 200 20, G5 200 20, C6 200 20, D6 200 20, E6 400 200";
const char kNotificationMelody[] =
  "G6 50 10, B6 100 10";
const char kCallMelody[] =
  "C6 150 50, E6 150 50, G6 150 50, C7 300 100, G6 150 50, E6 150 50, D6 150 50, G6 300 100, C6 150 50, E6 150 50, G6 150 50, C7 500 800";
constexpr int kCallMelodyPlays = 3;

AnimationPlayer animationPlayer(display);
UiRenderer ui(display);
SoundManager soundManager;
WifiPortal wifiPortal;
TimeService timeService;
WeatherService weatherService;
ChronosESP32 watch("shaws.systems");
TouchInput touchInput;

enum class ScreenMode {
  Animation,
  Clock,
  Notification,
  Portal,
  NowPlaying,
  Settings
};

ScreenMode screenMode = ScreenMode::Animation;
unsigned long lastClockDraw = 0;
unsigned long lastInteractionMs = 0;
unsigned long nextAnimationAtMs = 0;
unsigned long notificationUntilMs = 0;
unsigned long nowPlayingUntilMs = 0;
bool longPressHandled = false;
bool networkStarted = false;
bool hasNotification = false;
bool hasNowPlaying = false;
String latestNotification = "";
bool latestNotificationIsCall = false;
String nowPlayingTitle = "";
String nowPlayingArtist = "";
String nowPlayingApp = "";
bool nowPlayingAutoHide = false;
int settingsIndex = 0;
int callMelodyPlaysLeft = 0;
unsigned long nextCallMelodyAtMs = 0;
bool callMelodySequenceActive = false;
bool callMelodyToneActive = false;
bool callAlertActive = false;
bool callAlertEnded = false;

void chronosConnectionCallback(bool state) {
  Serial.print("Chronos: ");
  Serial.println(state ? "connected" : "disconnected");
}

void chronosNotificationCallback(Notification notification) {
  Serial.print("Chronos notif app: ");
  Serial.print(notification.app);
  Serial.print(" | title: ");
  Serial.print(notification.title);
  Serial.print(" | message: ");
  Serial.println(notification.message);

  String app = notification.app;
  app.toLowerCase();
  const bool isMusicApp = app.indexOf("spotify") >= 0 || app.indexOf("yt music") >= 0 ||
      app.indexOf("ytmusic") >= 0 || app.indexOf("youtube music") >= 0 ||
      app.indexOf("viber") >= 0 || app.indexOf("kakaotalk") >= 0;

  if (isMusicApp) {
    const String title = notification.title;
    const String message = notification.message;
    if (title == "Message") {
      const int splitIndex = message.indexOf(':');
      if (splitIndex > 0) {
        nowPlayingTitle = message.substring(splitIndex + 1);
        nowPlayingTitle.trim();
        nowPlayingArtist = message.substring(0, splitIndex);
        nowPlayingArtist.trim();
      } else {
        nowPlayingTitle = message;
        nowPlayingArtist = "";
      }
    } else {
      nowPlayingTitle = title;
      nowPlayingArtist = message;
      nowPlayingArtist.trim();
    }
    nowPlayingApp = notification.app;
    hasNowPlaying = true;
    return;
  }

  if (!notification.title.isEmpty()) {
    latestNotification = notification.title + ": " + notification.message;
  } else {
    latestNotification = notification.message;
  }
  soundManager.startMelody(kNotificationMelody);
  latestNotificationIsCall = false;
  hasNotification = true;
}

void chronosRingerCallback(String caller, bool state) {
  if (!state) {
    callAlertActive = false;
    callAlertEnded = true;
    callMelodyPlaysLeft = 0;
    callMelodySequenceActive = false;
    callMelodyToneActive = false;
    soundManager.stopMelody();
    return;
  }
  latestNotification = caller.isEmpty() ? "Incoming call" : "Call: " + caller;
  callMelodyPlaysLeft = kCallMelodyPlays;
  nextCallMelodyAtMs = millis();
  callMelodySequenceActive = true;
  callMelodyToneActive = false;
  callAlertActive = true;
  latestNotificationIsCall = true;
  hasNotification = true;
}

void updateCallMelody(unsigned long now) {
  if (!callMelodySequenceActive) {
    return;
  }

  const bool melodyActive = soundManager.isMelodyActive();
  if (callMelodyToneActive && !melodyActive) {
    callMelodyToneActive = false;
    nextCallMelodyAtMs = now + kCallMelodyGapMs;
    if (callMelodyPlaysLeft == 0) {
      callMelodySequenceActive = false;
    }
  }

  if (!melodyActive && !callMelodyToneActive && callMelodyPlaysLeft > 0 && now >= nextCallMelodyAtMs) {
    soundManager.startMelody(kCallMelody);
    callMelodyPlaysLeft--;
    callMelodyToneActive = true;
  }
}

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

  const String ampm = "";

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
      watch.isConnected());
}

void showNowPlaying() {
  const String title = nowPlayingTitle.isEmpty() ? "No music" : nowPlayingTitle;
  const String artist = nowPlayingArtist;
  const String appName = nowPlayingApp.isEmpty() ? "" : nowPlayingApp;
  String timeText = activeTimeString();
  if (timeText.length() > 5) {
    timeText = timeText.substring(0, 5);
  }
  ui.showNowPlayingScreen(title, artist, appName, timeText);
}

void showSettings() {
  ui.showSettingsScreen(settingsIndex);
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

void startNetworking() {
  if (networkStarted) {
    return;
  }

  wifiPortal.begin();
  if (wifiPortal.isApMode() && !wifiPortal.isWifiConnected()) {
    screenMode = ScreenMode::Portal;
    ui.showPortalScreen("shaws.systems", wifiPortal.localIp());
  }
  Serial.print("WiFi: status ");
  Serial.println(wifiPortal.isWifiConnected() ? "connected" : "not connected");
  timeService.setTimezone("IST-5:30");
  timeService.begin(wifiPortal.isWifiConnected());
  weatherService.begin(wifiPortal.city());
  watch.setConnectionCallback(chronosConnectionCallback);
  watch.setNotificationCallback(chronosNotificationCallback);
  watch.setRingerCallback(chronosRingerCallback);
  watch.begin();
  watch.setBattery(80);
  watch.set24Hour(true);
  Serial.print("Chronos: address ");
  Serial.println(watch.getAddress());
  networkStarted = true;
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

  ui.showStartupScreen();
  delay(3000);
  ui.showCertificationScreen();
  delay(1000);

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

  soundManager.begin(kBuzzerPin);
  soundManager.startMelody(kGreetingMelody);
  enterAnimationMode(kGreetingAnimationIndex);
  lastInteractionMs = millis();
  nextAnimationAtMs = lastInteractionMs + kIdleToFirstAnimationMs;
}

void loop() {
  const unsigned long now = millis();
  soundManager.updateMelody();
  updateCallMelody(now);
  if (networkStarted) {
    watch.loop();
    wifiPortal.handle();
    timeService.trySync(wifiPortal.isWifiConnected());
    weatherService.setCity(wifiPortal.city());
    weatherService.update(wifiPortal.isWifiConnected());
  }

  touchInput.update();
  const bool notificationLocked = (screenMode == ScreenMode::Notification) &&
    (callAlertActive || now < notificationUntilMs);

  if (touchInput.wasTapped() && !notificationLocked) {
    registerInteraction(now);
    if (screenMode == ScreenMode::Clock) {
      screenMode = ScreenMode::NowPlaying;
      nowPlayingAutoHide = false;
      showNowPlaying();
    } else if (screenMode == ScreenMode::NowPlaying) {
      screenMode = ScreenMode::Settings;
      showSettings();
    } else if (screenMode == ScreenMode::Settings) {
      screenMode = ScreenMode::Clock;
      drawClock();
    } else {
      screenMode = ScreenMode::Clock;
      drawClock();
    }
  }

  if (!notificationLocked && touchInput.isPressed() && !longPressHandled &&
      touchInput.pressedMs() >= kLongPressMs) {
    longPressHandled = true;
    registerInteraction(now);
    screenMode = ScreenMode::Portal;
    ui.showPortalScreen("shaws.systems", wifiPortal.localIp());
  }

  if (!touchInput.isPressed()) {
    longPressHandled = false;
  }

  if (hasNotification) {
    const String message = latestNotification;
    hasNotification = false;
    ui.showNotificationScreen(message, latestNotificationIsCall);
    screenMode = ScreenMode::Notification;
    if (latestNotificationIsCall) {
      notificationUntilMs = 0;
    } else {
      notificationUntilMs = now + kNotificationDisplayMs;
    }
    registerInteraction(now);
  }

  if (hasNowPlaying && !callAlertActive) {
    hasNowPlaying = false;
    showNowPlaying();
    screenMode = ScreenMode::NowPlaying;
    nowPlayingUntilMs = now + kNowPlayingDisplayMs;
    nowPlayingAutoHide = true;
    registerInteraction(now);
  }

  if (callAlertEnded) {
    callAlertEnded = false;
    if (screenMode == ScreenMode::Notification && latestNotificationIsCall && !callAlertActive) {
      screenMode = ScreenMode::Clock;
      drawClock();
    }
  }

  switch (screenMode) {
    case ScreenMode::Animation:
      if (animationPlayer.update()) {
        screenMode = ScreenMode::Clock;
        drawClock();
        nextAnimationAtMs = now + random(kMinIdleToAnimationMs, kMaxIdleToAnimationMs);
        startNetworking();
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
      if (!callAlertActive && now >= notificationUntilMs) {
        screenMode = ScreenMode::Clock;
        drawClock();
      }
      break;

    case ScreenMode::Portal:
      if (wifiPortal.isWifiConnected()) {
        screenMode = ScreenMode::Clock;
        drawClock();
        break;
      }
      if (now - lastInteractionMs >= kIdleToFirstAnimationMs) {
        screenMode = ScreenMode::Clock;
        drawClock();
      }
      break;

    case ScreenMode::NowPlaying:
      if (nowPlayingAutoHide && now >= nowPlayingUntilMs) {
        screenMode = ScreenMode::Clock;
        drawClock();
      }
      break;

    case ScreenMode::Settings:
      break;
  }
}
