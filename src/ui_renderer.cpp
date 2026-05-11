#include "ui_renderer.h"
#include "Picopixel.h"
#include "lopaka_assets.h"

//Agent STOP resetting the font, ALL are same font! 

UiRenderer::UiRenderer(Adafruit_SSD1306& display) : display_(display) {}

void UiRenderer::showBootBrand() {
  display_.clearDisplay();
  display_.setTextSize(2);
  display_.setTextColor(SSD1306_WHITE);
  display_.setCursor(10, 20);
  display_.print("Mochi");
  display_.display();
}

void UiRenderer::showPortalScreen(const String& apName, const String& ip) {
  display_.clearDisplay();
  display_.setTextSize(1);
  display_.setTextColor(SSD1306_WHITE);
  display_.setCursor(0, 10);
  display_.println("WiFi Setup");
  display_.println("AP: " + apName);
  display_.println("IP: " + ip);
  display_.display();
}

void UiRenderer::showNotificationScreen(const String& text) {
  display_.clearDisplay();
  display_.setTextSize(1);
  display_.setTextColor(SSD1306_WHITE);
  display_.setCursor(0, 10);
  display_.println("Notice:");
  display_.println(text);
  display_.display();
}

void UiRenderer::showClockScreen(
    const String& timeText,
    const String& dateText,
    const String& weatherText,
    const String& cityText,
    const String& ampmText,
    int weatherCode,
    bool isNight,
    bool wifiOk,
    bool bleOk) {
  display_.clearDisplay();

  // Time
  display_.setTextColor(1);
  display_.setTextSize(3);
  display_.setTextWrap(false);
  display_.setFont(&Picopixel);
  display_.setCursor(10, 30);
  display_.print(timeText);

  // Date
  display_.setTextSize(1);
  display_.setCursor(14, 41);
  display_.print(dateText);

  // Weather
  display_.setCursor(15, 51);
  display_.print(weatherText);

  // arrow_diagonal_left_up
  display_.drawBitmap(2, 57, image_arrow_diagonal_left_up_bits, 5, 5, 1);

  if (isNight) {
    // Sleepy Mochi
    display_.drawBitmap(61, 7, image_sleepy_mochi_bits, 66, 50, 1);
  } else {
    // Sunny Mochi
    display_.drawBitmap(61, 15, image_Sunny_Mochi_bits, 53, 32, 1);
  }

  if (bleOk) {
    // Bluetooth
    display_.drawBitmap(6, 3, image_Bluetooth_bits, 5, 8, 1);
  }

  if (wifiOk) {
    // WiFi
    display_.drawBitmap(12, 2, image_WiFi_bits, 7, 10, 1);
    // Part Of WiFi
    display_.drawBitmap(19, 5, image_Part_Of_WiFi_bits, 1, 1, 1);
  }

  // City
  display_.setCursor(50, 8);
  display_.print(cityText);

  // line 13
  display_.drawLine(28, 47, 28, 51, 1);

  display_.display();
}
