#include "ui_renderer.h"
#include "Picopixel.h"
#include "FreeSerif9pt7b.h"
#include "lopaka_assets.h"
#include "ui_assets.h"

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

void UiRenderer::showStartupScreen() {
  display_.clearDisplay();

  display_.drawBitmap(0, 0, image_BLE_Pairing_bits, 128, 64, 1);

  display_.setTextColor(SSD1306_WHITE);
  display_.setTextWrap(false);
  display_.setCursor(26, 55);
  display_.print("shaws.systems");

  display_.fillRect(57, 28, 15, 25, 0);
  display_.drawCircle(13, 7, 4, 0);
  display_.drawCircle(22, 5, 2, 0);

  display_.display();
}

void UiRenderer::showCertificationScreen() {
  display_.clearDisplay();
  display_.drawBitmap(12, 4, image_Certification1_bits, 103, 56, 1);
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

void UiRenderer::showNotificationScreen(const String& text, bool isCall) {
  display_.clearDisplay();

  display_.setTextColor(SSD1306_WHITE);
  display_.setTextWrap(false);
  display_.setFont(&FreeSerif9pt7b);
  display_.setCursor(21, 14);
  display_.print("Notification");

  display_.drawRect(0, 19, 127, 45, 1);

  display_.setFont();
  display_.setTextSize(1);
  display_.setTextWrap(true);
  display_.setCursor(8, 31);
  display_.print(text);

  if (isCall) {
    display_.drawBitmap(2, 2, image_phone_call_in_out_bits, 15, 16, 1);
  } else {
    display_.drawBitmap(2, 3, image_message_bits, 16, 16, 1);
  }
  display_.drawBitmap(0, 54, image_bottom_notif_bits, 128, 10, 1);
  display_.drawBitmap(0, 18, image_frame_notif_bits, 128, 46, 0);
  display_.drawBitmap(122, 20, image_menu_arrow_down_left_bits, 4, 4, 1);
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

  const bool isThunder = weatherCode == 95;
  const bool isRain = (weatherCode >= 51 && weatherCode <= 65) || (weatherCode >= 80 && weatherCode <= 82);
  const bool isFog = weatherCode == 45 || weatherCode == 48;
  const bool isCloudy = weatherCode >= 1 && weatherCode <= 3;

  if (isNight) {
    // Sleepy Mochi
    display_.drawBitmap(61, 10, image_sleepy_mochi_bits, 66, 50, 1);
  } else if (isThunder) {
    // Thunderstorm Mochi
    display_.drawBitmap(72, 12, image_Thunderstorm_Mochi_bits, 48, 50, 1);
  } else if (isRain) {
    // Rainy Mochi
    display_.drawBitmap(70, 12, image_Rainy_Mochi_bits, 56, 50, 1);
  } else if (isFog) {
    // Fog Mochi
    display_.drawBitmap(70, 12, image_Fog_Mochi_bits, 55, 48, 1);
  } else if (isCloudy) {
    // Cloudy Mochi
    display_.drawBitmap(74, 8, image_Cloudy_Mochi_bits, 40, 50, 1);
  } else {
    // Sunny Mochi
    display_.drawBitmap(70, 20, image_Sunny_Mochi_bits, 53, 32, 1);
  }

  if (bleOk) {
    // Bluetooth
    display_.drawBitmap(6, 3, image_Bluetooth_bits, 5, 8, 1);
  }

  if (wifiOk) {
    // WiFi
    display_.drawBitmap(14, 2, image_wifi_bits, 11, 9, 1);
  }

  // City
  display_.setCursor(50, 8);
  display_.print(cityText);

  // line 13
  display_.drawLine(28, 47, 28, 51, 1);

  display_.display();
}
