// ui_renderer.cpp — OLED screen rendering for all Mochi UI states.
// Handles clock, notifications, now-playing (with marquee scroll),
// settings menu, startup, certification, and WiFi portal screens.

#include "ui_renderer.h"

#include "FreeSerif9pt7b.h"
#include "FreeSerifItalic4pt7b.h"
#include "FreeSerifItalic5pt7b.h"
#include "FreeSerifItalic9pt7b.h"
#include "Picopixel.h"
#include "lopaka_assets.h"
#include "ui_assets.h"

namespace {
constexpr int kSettingsMenuCount = 4;
constexpr int kMarqueeMaxWidth = 100;
constexpr int kMarqueeGap = 40;
constexpr int kMarqueeSpeed = 100;
}  // namespace

UiRenderer::UiRenderer(Adafruit_SSD1306& display) : display_(display) {}

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
  display_.setTextWrap(false);
  
  const int maxLineWidth = 112; 
  String line1 = "";
  String line2 = "";
  int lineIdx = 1;
  bool truncated = false;
  
  int lastSpaceIdx = -1;
  String currentLineStr = "";
  
  for (int i = 0; i < (int)text.length(); i++) {
    char c = text[i];
    
    if (c == '\n') {
       if (lineIdx == 1) {
         line1 = currentLineStr;
         currentLineStr = "";
         lineIdx = 2;
         lastSpaceIdx = -1;
       } else {
         truncated = true;
         break;
       }
       continue;
    }
    
    currentLineStr += c;
    if (c == ' ') lastSpaceIdx = currentLineStr.length() - 1;
    
    int16_t x1, y1;
    uint16_t w, h;
    display_.getTextBounds(currentLineStr, 0, 0, &x1, &y1, &w, &h);
    
    if (w > maxLineWidth) {
      if (lineIdx == 1) {
         if (lastSpaceIdx != -1 && lastSpaceIdx > 0) {
           line1 = currentLineStr.substring(0, lastSpaceIdx);
           currentLineStr = currentLineStr.substring(lastSpaceIdx + 1);
         } else {
           line1 = currentLineStr.substring(0, currentLineStr.length() - 1);
           currentLineStr = String(c);
         }
         lineIdx = 2;
         lastSpaceIdx = -1;
         
         display_.getTextBounds(currentLineStr, 0, 0, &x1, &y1, &w, &h);
         if (w > maxLineWidth) {
           truncated = true;
           break;
         }
      } else {
         currentLineStr.remove(currentLineStr.length() - 1);
         truncated = true;
         break;
      }
    }
  }
  
  if (lineIdx == 1) {
    line1 = currentLineStr;
  } else {
    line2 = currentLineStr;
  }
  
  if (truncated) {
    int16_t x1, y1;
    uint16_t w, h;
    while (line2.length() > 0) {
      display_.getTextBounds(line2 + "..", 0, 0, &x1, &y1, &w, &h);
      if (w <= maxLineWidth) break;
      line2.remove(line2.length() - 1);
    }
    line2 += "..";
  }

  display_.setCursor(8, 30);
  display_.print(line1);
  if (line2.length() > 0) {
    display_.setCursor(8, 40);
    display_.print(line2);
  }

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
  display_.setTextWrap(false);
  display_.setFont(&Picopixel);
  display_.setTextSize(3);
  display_.setCursor(10, 30);
  display_.print(timeText);

  if (!ampmText.isEmpty()) {
    int16_t x1, y1;
    uint16_t w, h;
    display_.getTextBounds(timeText, 10, 30, &x1, &y1, &w, &h);
    display_.setTextSize(1);
    display_.setCursor(10 + w + 4, 30);
    display_.print(ampmText);
  }

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

void UiRenderer::showNowPlayingScreen(
    const String& title,
    const String& artist,
    const String& appName,
    const String& timeText,
    unsigned long now) {
  display_.clearDisplay();
  display_.setTextColor(SSD1306_WHITE);
  display_.setTextWrap(false);

  // Buttons
  display_.drawBitmap(74, 52, image_ButtonRight_bits, 4, 7, 1);
  display_.drawBitmap(53, 52, image_ButtonLeft_bits, 4, 7, 1);
  display_.fillCircle(65, 55, 4, 1);
  display_.drawLine(64, 53, 64, 57, 0);
  display_.drawLine(66, 53, 66, 57, 0);

  // Rectangle
  display_.fillRect(45, 9, 39, 23, 1);
  display_.drawBitmap(55, 13, image_Lines_bits, 19, 15, 0);

  // Time
  display_.setFont(&Picopixel);
  display_.setCursor(109, 6);
  display_.print(timeText);

  // Icons
  display_.drawBitmap(1, 2, image_Music_bits, 7, 8, 1);

  const int maxTextWidth = kMarqueeMaxWidth;
  
  // Title (Scrolling 5pt font)
  display_.setFont(&FreeSerifItalic5pt7b);
  display_.setTextSize(1);
  int16_t x1, y1;
  uint16_t w, h;
  display_.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);
  
  if (w <= maxTextWidth) {
    display_.setCursor((128 - w) / 2, 41);
    display_.print(title);
  } else {
    int offset = (now / kMarqueeSpeed) % (w + kMarqueeGap);
    int x_pos = 12 - offset;
    display_.setCursor(x_pos, 41);
    display_.print(title);
    if (x_pos + (int)w < 128) {
      display_.setCursor(x_pos + w + kMarqueeGap, 41);
      display_.print(title);
    }
  }

  // Artist (Scrolling 4pt font)
  display_.setFont(&FreeSerifItalic4pt7b);
  display_.getTextBounds(artist, 0, 0, &x1, &y1, &w, &h);
  
  if (w <= maxTextWidth) {
    display_.setCursor((128 - w) / 2, 49);
    display_.print(artist);
  } else {
    int offset = (now / kMarqueeSpeed) % (w + kMarqueeGap);
    int x_pos = 12 - offset;
    display_.setCursor(x_pos, 49);
    display_.print(artist);
    if (x_pos + (int)w < 128) {
      display_.setCursor(x_pos + w + kMarqueeGap, 49);
      display_.print(artist);
    }
  }

  // Clipping trick: Hide text that spills into the margins
  display_.fillRect(0, 33, 12, 31, 0);   // Left side margin
  display_.fillRect(108, 33, 20, 31, 0); // Right side (Waves area)
  
  // Waves icon over the masked area
  display_.drawBitmap(110, 43, image_Waves_bits, 18, 21, 1);

  display_.display();
}

void UiRenderer::showSettingsScreen(int selectedIndex, bool isMuted, bool is24H) {
  display_.clearDisplay();
  display_.setTextWrap(false);
  display_.setTextSize(1);
  display_.setFont();

  // Draw header
  display_.setTextColor(SSD1306_WHITE);
  display_.setCursor(0, 4);
  display_.print("Settings");
  display_.drawLine(0, 14, 128, 14, SSD1306_WHITE);

  String options[4];
  options[0] = String("Sound: ") + (isMuted ? "OFF" : "ON");
  options[1] = String("Clock: ") + (is24H ? "24H" : "12H");
  options[2] = "WiFi Setup";
  options[3] = "Exit";
  
  int startY = 18;
  int itemHeight = 11; 
  
  for (int i = 0; i < kSettingsMenuCount; i++) {
    int y = startY + i * itemHeight;
    if (i == selectedIndex) {
      display_.fillRect(0, y - 1, 128, itemHeight, SSD1306_WHITE);
      display_.setTextColor(SSD1306_BLACK); 
    } else {
      display_.setTextColor(SSD1306_WHITE);
    }
    display_.setCursor(4, y);
    display_.print(options[i]);
  }

  display_.display();
}
