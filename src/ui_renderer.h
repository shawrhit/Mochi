#pragma once

#include <Adafruit_SSD1306.h>
#include <WString.h>

class UiRenderer {
 public:
  explicit UiRenderer(Adafruit_SSD1306& display);

  void showBootBrand();
  void showPortalScreen(const String& apName, const String& ip);
  void showClockScreen(const String& timeText, const String& dateText, bool wifiOk, bool bleOk, int batteryPercent = -1);
  void showNotificationScreen(const String& text);

 private:
  Adafruit_SSD1306& display_;
};
