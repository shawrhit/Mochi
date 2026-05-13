#pragma once

#include <Adafruit_SSD1306.h>
#include <WString.h>

class UiRenderer {
 public:
  explicit UiRenderer(Adafruit_SSD1306& display);

  void showBootBrand();
  void showStartupScreen();
  void showCertificationScreen();
  void showPortalScreen(const String& apName, const String& ip);
  void showClockScreen(
      const String& timeText,
      const String& dateText,
      const String& weatherText,
      const String& cityText,
      const String& ampmText,
      int weatherCode,
      bool isNight,
      bool wifiOk,
      bool bleOk);
  void showNowPlayingScreen(
      const String& title,
      const String& artist,
      const String& appName,
      const String& timeText,
      unsigned long now);
  void showSettingsScreen(int selectedIndex, bool isMuted, bool is24H);
  void showNotificationScreen(const String& text, bool isCall);

 private:
  Adafruit_SSD1306& display_;
};
