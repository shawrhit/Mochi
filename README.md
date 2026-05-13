# 🍡 Mochi

A tiny ESP32-C3 desk companion with a 128×64 OLED display, single-button capacitive touch, and smartphone notifications via the [Chronos](https://play.google.com/store/apps/details?id=com.fbiego.chronos) companion app.

![ESP32-C3](https://img.shields.io/badge/ESP32--C3-PlatformIO-blue?logo=espressif)
![License](https://img.shields.io/badge/license-MIT-green)

---

## ✨ Features

| Category | Details |
|----------|---------|
| **Clock** | Live clock with 12H/24H toggle, date, and weather-reactive Mochi character |
| **Weather** | Real-time temperature and conditions via [Open-Meteo](https://open-meteo.com/) with geocoding |
| **Notifications** | Phone notifications and incoming call alerts via Chronos BLE |
| **Now Playing** | Song title and artist with smooth horizontal marquee scrolling |
| **Animations** | 7 idle animations (sleepy, love, evil, yawn, playful, greeting, ping-pong) |
| **Settings** | Single-button menu: Sound ON/OFF, Clock 12H/24H, WiFi Setup, Exit |
| **Sound** | Non-blocking async melody engine for greeting, notification, and call tones |
| **WiFi Portal** | Captive portal AP for first-time WiFi + city configuration |

## 🎮 Controls

Mochi uses a **single capacitive touch button** for all interaction:

| Input | Action |
|-------|--------|
| **Short tap** | Cycle between Clock ↔ Now Playing screens |
| **Short tap** (in Settings) | Move to next menu option |
| **Long press** (2s) | Open Settings menu |
| **Long press** (2s, in Settings) | Select / toggle the highlighted option |
| **Long press** (at boot) | Skip WiFi and boot offline |

## 🔧 Hardware

| Component | Specification |
|-----------|---------------|
| MCU | ESP32-C3-DevKitM-1 |
| Display | 128×64 SH1106/SSD1306 OLED (I²C, `0x3C`) |
| Touch | Capacitive touch on GPIO 10 |
| Buzzer | Passive piezo on GPIO 2 |
| I²C | SDA: GPIO 21, SCL: GPIO 20 |

## 🏗️ Project Structure

```
src/
├── main.cpp                  # State machine, boot sequence, touch handling
├── ui_renderer.cpp/h         # All OLED screen layouts and rendering
├── touch_input.cpp/h         # Debounced capacitive touch driver
├── sound_manager.cpp/h       # Non-blocking async melody player
├── time_service.cpp/h        # HTTP time sync with 12/24H support
├── weather_service.cpp/h     # Open-Meteo weather + geocoding
├── wifi_portal.cpp/h         # Captive portal AP + credential storage
├── ble_notifier.cpp/h        # BLE GATT notification server
├── animation_player.cpp/h    # Frame-by-frame bitmap animation engine
├── animation_catalog.cpp/h   # Animation playlist registry
├── lopaka_assets.h           # Clock screen bitmaps (Mochi characters)
├── ui_assets.h               # UI element bitmaps
├── FreeSerif*.h / Picopixel.h # Custom fonts (4pt, 5pt, 9pt)
└── animations.h / custom_*   # PROGMEM animation frame data
```

## 🚀 Build & Flash

This project uses [PlatformIO](https://platformio.org/).

```bash
# Build
pio run

# Upload
pio run --target upload

# Serial Monitor
pio device monitor
```

## 📡 WiFi Setup

1. On first boot (or when no saved network is found), Mochi creates a WiFi AP:
   - **SSID:** `Mochi`
   - **Password:** `mochisetup`
2. Connect to the AP and a captive portal will open automatically.
3. Enter your home WiFi SSID, password, and city name (for weather).
4. The device saves credentials and reboots into station mode.

> **Tip:** To skip WiFi at boot, hold the touch button for 2 seconds during the startup screen.

## 📱 Chronos Pairing

1. Install the [Chronos](https://play.google.com/store/apps/details?id=com.fbiego.chronos) app on your phone.
2. Open the app and scan for **"Mochi"**.
3. Pair from **inside the Chronos app** (not from the OS Bluetooth settings).
4. Phone notifications and call alerts will now appear on the OLED.

## 🎵 Melody Format

Melodies are defined as comma-separated note triplets:

```
"Note Duration Rest, Note Duration Rest, ..."
```

Example: `"G4 200 20, C5 200 20, E5 400 200"`

- **Note:** Letter + octave (e.g., `C5`, `G4`, `A3S` for sharp)
- **Duration:** Tone length in milliseconds
- **Rest:** Silence after note in milliseconds

## ⚠️ Notes

- WiFi TX power is fixed at **8.5 dBm** for ESP32-C3 stability — do not change.
- The AP automatically retries STA connection every 5 minutes when idle.
- Animation frame data lives in PROGMEM to conserve RAM.
- Font files were generated using the Adafruit GFX `fontconvert` tool from FreeSerif TTF.

## 📜 License

MIT — Built with 🍡 by [shaws.systems](https://shaws.systems)
