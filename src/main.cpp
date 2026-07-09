// main.cpp — Mochi firmware entry point and state machine.
// Handles boot sequence, screen transitions, touch input, notifications,
// weather updates, and settings management.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ChronosESP32.h>
#include <esp_sleep.h>
#include "esp_system.h"
#include "esp_system.h"

#include "animation_catalog.h"
#include "animation_player.h"
#include "sound_manager.h"
#include "time_service.h"
#include "touch_input.h"
#include "ui_renderer.h"
#include "weather_service.h"
#include "wifi_portal.h"

constexpr int kScreenWidth = 128;
constexpr int kScreenHeight = 64;
constexpr int kOledReset = -1;

Adafruit_SSD1306 display(kScreenWidth, kScreenHeight, &Wire, kOledReset);

constexpr int kBuzzerPin = 2;
constexpr int kTouchPin = 10;
constexpr int kAnimationFrameDelayMs = 200;
constexpr int kWelcomeAnimationIndex = 7;
constexpr unsigned long kBootHoldMs = 2000;
constexpr unsigned long kClockRefreshMs = 500;
constexpr unsigned long kNowPlayingRefreshMs = 50;
constexpr unsigned long kPomodoroRefreshMs = 250;
constexpr unsigned long kIdleToFirstAnimationMs = 180000;
constexpr unsigned long kMinIdleToAnimationMs = 180000;
constexpr unsigned long kMaxIdleToAnimationMs = 300000;
constexpr unsigned long kNotificationDisplayMs = 5000;
constexpr unsigned long kNowPlayingDisplayMs = 5000;
constexpr unsigned long kCallMelodyGapMs = 1000;
constexpr unsigned long kLongPressMs = 2000;
constexpr unsigned long kBootTouchWindowMs = 3500;
constexpr int kSettingsMenuCount = 5;
constexpr unsigned long kPomodoroWorkMs = 25UL * 60UL * 1000UL;
constexpr unsigned long kPomodoroBreakMs = 5UL * 60UL * 1000UL;

const char kGreetingMelody[] =
  "G4 200 20, C5 200 20, E5 200 20, G5 200 20, C6 200 20, D6 200 20, E6 400 200";
const char kNotificationMelody[] =
  "G6 50 10, B6 100 10";
const char kCallMelody[] =
  "C6 150 50, E6 150 50, G6 150 50, C7 300 100, G6 150 50, E6 150 50, D6 150 50, G6 300 100, C6 150 50, E6 150 50, G6 150 50, C7 500 800";
const char kShutdownMelody[] =
  "E6 150 20, C6 150 20, G5 150 20, C5 400 0";
constexpr int kCallMelodyPlays = 3;

AnimationPlayer animationPlayer(display);
UiRenderer ui(display);
SoundManager soundManager;
WifiPortal wifiPortal;
TimeService timeService;
WeatherService weatherService;
ChronosESP32 watch("Mochi");
TouchInput touchInput;

enum class ScreenMode {
  Animation,
  Clock,
  Face,
  Notification,
  Portal,
  NowPlaying,
  Pomodoro,
  Settings
};

ScreenMode screenMode = ScreenMode::Animation;
ScreenMode idleScreenMode = ScreenMode::Clock;
unsigned long lastClockDraw = 0;
unsigned long lastNowPlayingDraw = 0;
unsigned long lastPomodoroDraw = 0;
unsigned long lastInteractionMs = 0;
unsigned long nextAnimationAtMs = 0;
unsigned long notificationUntilMs = 0;
unsigned long nowPlayingUntilMs = 0;
bool longPressHandled = false;
bool extraLongPressHandled = false;
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
bool isMuted = false;
int callMelodyPlaysLeft = 0;
unsigned long nextCallMelodyAtMs = 0;
bool callMelodySequenceActive = false;
bool callMelodyToneActive = false;
bool callAlertActive = false;
bool callAlertEnded = false;

enum class PomodoroPhase {
  Work,
  Break
};

PomodoroPhase pomodoroPhase = PomodoroPhase::Work;
bool pomodoroRunning = false;
bool pomodoroComplete = false;
unsigned long pomodoroRemainingMs = kPomodoroWorkMs;
unsigned long pomodoroLastTickMs = 0;
int pomodoroSession = 1;
constexpr int kPomodoroTotalSessions = 4;

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
  if (!isMuted) {
    soundManager.startMelody(kNotificationMelody);
  }
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
    if (!isMuted) {
      soundManager.startMelody(kCallMelody);
    }
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

bool isNightTime() {
  if (!timeService.isSynced()) {
    return false;
  }
  const int hour = timeService.hour24();
  return hour < 6 || hour >= 22;
}

String activeAmpmString() {
  if (timeService.isSynced()) {
    return timeService.ampmString();
  }
  return "";
}

void drawClock() {
  const int tempC = weatherService.temperatureC();
  const String tempText = tempC >= 0 ? String(tempC) + "C" : String("--C");

  const String ampm = activeAmpmString();

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
  ui.showNowPlayingScreen(title, artist, appName, timeText, millis());
}

unsigned long pomodoroTotalMs() {
  return pomodoroPhase == PomodoroPhase::Work ? kPomodoroWorkMs : kPomodoroBreakMs;
}

String pomodoroPhaseText() {
  return pomodoroPhase == PomodoroPhase::Work ? "FOCUS" : "BREAK";
}

void showPomodoro() {
  String timeTxt = activeTimeString();
  if (timeTxt.length() > 5) {
    timeTxt = timeTxt.substring(0, 5);
  }
  ui.showPomodoroScreen(
      pomodoroPhaseText(),
      pomodoroRemainingMs,
      pomodoroTotalMs(),
      pomodoroRunning,
      pomodoroComplete,
      pomodoroSession,
      kPomodoroTotalSessions,
      timeTxt);
}

void togglePomodoro(unsigned long now) {
  if (pomodoroComplete) {
    pomodoroComplete = false;
  }

  if (pomodoroRemainingMs == 0) {
    pomodoroRemainingMs = pomodoroTotalMs();
  }

  pomodoroRunning = !pomodoroRunning;
  pomodoroLastTickMs = now;
  showPomodoro();
}

void startPomodoro(unsigned long now) {
  pomodoroComplete = false;
  pomodoroRunning = true;
  pomodoroLastTickMs = now;
}

void resetPomodoro(unsigned long now) {
  pomodoroPhase = PomodoroPhase::Work;
  pomodoroComplete = false;
  pomodoroRemainingMs = kPomodoroWorkMs;
  pomodoroRunning = true;
  pomodoroLastTickMs = now;
  pomodoroSession = 1;
  showPomodoro();
}

void enterPomodoroMode(unsigned long now) {
  screenMode = ScreenMode::Pomodoro;
  nowPlayingAutoHide = false;
  if (!pomodoroRunning) {
    startPomodoro(now);
  }
  showPomodoro();
}

void advancePomodoroPhase() {
  if (pomodoroPhase == PomodoroPhase::Work) {
    pomodoroPhase = PomodoroPhase::Break;
    pomodoroRemainingMs = kPomodoroBreakMs;
  } else {
    pomodoroPhase = PomodoroPhase::Work;
    pomodoroRemainingMs = kPomodoroWorkMs;
    pomodoroSession++;
    if (pomodoroSession > kPomodoroTotalSessions) {
      pomodoroSession = 1;
    }
  }
  pomodoroRunning = false;
  pomodoroComplete = true;
}

void updatePomodoro(unsigned long now) {
  if (!pomodoroRunning) {
    return;
  }

  const unsigned long elapsed = now - pomodoroLastTickMs;
  if (elapsed == 0) {
    return;
  }
  pomodoroLastTickMs = now;

  if (elapsed < pomodoroRemainingMs) {
    pomodoroRemainingMs -= elapsed;
    return;
  }

  advancePomodoroPhase();
  if (!isMuted) {
    soundManager.startMelody(kNotificationMelody);
  }
}

void returnToIdleScreen() {
  screenMode = idleScreenMode;
  if (idleScreenMode == ScreenMode::Face) {
    ui.showFaceScreen();
  } else {
    drawClock();
  }
}

void showSettings() {
  ui.showSettingsScreen(settingsIndex, isMuted, timeService.is24Hour());
}

void enterAnimationMode(int playlistIndex) {
  if (playlistIndex < 0 || playlistIndex >= kPlaylistCount) {
    return;
  }

  int delayMs = kAnimationFrameDelayMs;
  if (playlistIndex == kWelcomeAnimationIndex) {
    delayMs = 100; // Fast framerate for rev animation
  }

  animationPlayer.start(kPlaylist[playlistIndex], delayMs);
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
    ui.showPortalScreen("Mochi", wifiPortal.localIp());
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
  esp_reset_reason_t reason = esp_reset_reason();
  if (reason == ESP_RST_BROWNOUT) {
    Wire.begin(21, 20);
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(20, 16);
      display.print("BATTERY");
      display.setCursor(40, 36);
      display.print("LOW");
      display.display();
      delay(2000);
      display.ssd1306_command(SSD1306_DISPLAYOFF);
    }

    pinMode(kTouchPin, INPUT);
    gpio_wakeup_enable((gpio_num_t)kTouchPin, GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_light_sleep_start();
    
    while (digitalRead(kTouchPin) == HIGH) {
      delay(10);
    }
    esp_restart();
  }

  delay(1000);
  Serial.begin(115200);
  Serial.println("Mochi v1.2 by shaws.systems");

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
  if (!isMuted) {
    soundManager.startMelody(kGreetingMelody);
  }
  enterAnimationMode(kWelcomeAnimationIndex);
  lastInteractionMs = millis();
  nextAnimationAtMs = lastInteractionMs + kIdleToFirstAnimationMs;
}

void loop() {
  const unsigned long now = millis();
  soundManager.updateMelody();
  updateCallMelody(now);
  updatePomodoro(now);
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
      screenMode = ScreenMode::Face;
      idleScreenMode = ScreenMode::Face;
      ui.showFaceScreen();
    } else if (screenMode == ScreenMode::Face) {
      screenMode = ScreenMode::NowPlaying;
      nowPlayingAutoHide = false;
      showNowPlaying();
    } else if (screenMode == ScreenMode::NowPlaying) {
      enterPomodoroMode(now);
    } else if (screenMode == ScreenMode::Pomodoro) {
      screenMode = ScreenMode::Clock;
      idleScreenMode = ScreenMode::Clock;
      drawClock();
    } else if (screenMode == ScreenMode::Settings) {
      settingsIndex = (settingsIndex + 1) % kSettingsMenuCount;
      showSettings();
    } else {
      returnToIdleScreen();
    }
  }

  if (!notificationLocked && touchInput.isPressed()) {
    if (touchInput.pressedMs() >= 4000 && screenMode == ScreenMode::Pomodoro) {
      if (!extraLongPressHandled) {
        extraLongPressHandled = true;
        registerInteraction(now);
        resetPomodoro(now);
      }
    } else if (touchInput.pressedMs() >= kLongPressMs && !longPressHandled) {
      longPressHandled = true;
      registerInteraction(now);
      
      if (screenMode == ScreenMode::Pomodoro) {
        togglePomodoro(now);
      } else if (screenMode == ScreenMode::Settings) {
      if (settingsIndex == 0) {
        isMuted = !isMuted;
        showSettings();
      } else if (settingsIndex == 1) {
        timeService.set24Hour(!timeService.is24Hour());
        watch.set24Hour(timeService.is24Hour());
        showSettings();
      } else if (settingsIndex == 2) {
        screenMode = ScreenMode::Portal;
        ui.showPortalScreen("Mochi", wifiPortal.localIp());
      } else if (settingsIndex == 3) {
        if (!isMuted) {
          soundManager.startMelody(kShutdownMelody);
          while (soundManager.isMelodyActive()) {
            soundManager.updateMelody();
            delay(5);
          }
        }
        
        display.ssd1306_command(SSD1306_DISPLAYOFF);
        digitalWrite(kBuzzerPin, LOW);

        // Wait for user to release their finger before sleeping
        while (digitalRead(kTouchPin) == HIGH) {
          delay(10);
        }
        delay(100);

        gpio_wakeup_enable((gpio_num_t)kTouchPin, GPIO_INTR_HIGH_LEVEL);
        esp_sleep_enable_gpio_wakeup();
        esp_light_sleep_start();

        // System wakes up here!
        // Wait for user to release their finger so we don't trigger a tap immediately
        while (digitalRead(kTouchPin) == HIGH) {
          delay(10);
        }
        delay(100);

        display.ssd1306_command(SSD1306_DISPLAYON);
        screenMode = idleScreenMode;
        if (screenMode == ScreenMode::Clock) drawClock();
        else ui.showFaceScreen();
        lastInteractionMs = millis();
        longPressHandled = true;
      } else if (settingsIndex == 4) {
        screenMode = idleScreenMode;
        if (screenMode == ScreenMode::Clock) drawClock();
        else ui.showFaceScreen();
      }
    } else {
      screenMode = ScreenMode::Settings;
      settingsIndex = 0;
      showSettings();
    }
  }
}

  if (!touchInput.isPressed()) {
    longPressHandled = false;
    extraLongPressHandled = false;
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

  if (hasNowPlaying && !callAlertActive && screenMode != ScreenMode::Pomodoro) {
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
      returnToIdleScreen();
    }
  }

  switch (screenMode) {
    case ScreenMode::Animation:
      if (animationPlayer.update()) {
        returnToIdleScreen();
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

    case ScreenMode::Face:
      if (now - lastInteractionMs >= kIdleToFirstAnimationMs && now >= nextAnimationAtMs) {
        enterRandomAnimation();
      }
      break;

    case ScreenMode::Notification:
      if (!callAlertActive && now >= notificationUntilMs) {
        returnToIdleScreen();
      }
      break;

    case ScreenMode::Portal:
      if (wifiPortal.isWifiConnected()) {
        returnToIdleScreen();
        break;
      }
      if (now - lastInteractionMs >= kIdleToFirstAnimationMs) {
        returnToIdleScreen();
      }
      break;

    case ScreenMode::NowPlaying:
      if (now - lastNowPlayingDraw >= kNowPlayingRefreshMs) {
        lastNowPlayingDraw = now;
        showNowPlaying();
      }
      if (nowPlayingAutoHide && now >= nowPlayingUntilMs) {
        returnToIdleScreen();
      }
      break;

    case ScreenMode::Pomodoro:
      if (now - lastPomodoroDraw >= kPomodoroRefreshMs) {
        lastPomodoroDraw = now;
        showPomodoro();
      }
      break;

    case ScreenMode::Settings:
      break;
  }
}
