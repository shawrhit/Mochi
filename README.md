# DasaiMochi

A tiny ESP32-C3 desk companion with a 128x64 OLED, touch input, WiFi portal, and Chronos app notifications.

## Features
- Dual clock UIs with weather and connectivity icons
- Startup and certification splash screens
- Chronos app notifications with call/message distinction
- WiFi captive portal for setup and city selection
- Sound effects for startup, notifications, and calls

## Hardware
- ESP32-C3 dev kit
- 128x64 SSD1306 OLED
- Touch input (single button)
- Piezo buzzer

## Build and Flash
This project uses PlatformIO.

- Build: `platformio run --environment esp32-c3-devkitm-1`
- Upload: `platformio run --environment esp32-c3-devkitm-1 --target upload`
- Monitor: `platformio device monitor`

## WiFi Setup
- On first boot (or if WiFi fails), the device starts an AP named `shaws.systems`.
- Connect and open the portal to save SSID, password, and city.

## Chronos Pairing
- Pair from inside the Chronos app (not the OS Bluetooth menu).
- Notifications and call alerts come through the app.

## Notes
- WiFi TX power is fixed to 8.5 dBm for stability on ESP32-C3.
- The AP retries STA connection every 5 minutes when no one is connected.
