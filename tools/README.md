# Bitmap Tools

Convert PNG/JPG weather icons to 1-bit PROGMEM arrays for SSD1306.

## Install

```bash
pip install pillow
```

## Usage

```bash
python3 bitmap_to_progmem.py --input sunny.png --name image_Sunny_bits --width 16 --height 16
```

Use the printed C array in `src/ui_renderer.cpp` (or a dedicated assets header).
