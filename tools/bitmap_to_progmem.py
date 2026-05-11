#!/usr/bin/env python3
"""Convert a bitmap image to PROGMEM C array (1-bit) for SSD1306.

Usage:
  python3 bitmap_to_progmem.py --input icon.png --name image_Sunny_bits --width 16 --height 16
"""

import argparse
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:
    raise SystemExit("Pillow is required. Run: pip install pillow") from exc


def image_to_bits(img, width, height, invert=False):
    img = img.convert("L").resize((width, height))
    pixels = img.load()

    bits = []
    for y in range(height):
        for x in range(width):
            val = pixels[x, y]
            bit = 1 if val < 128 else 0
            if invert:
                bit ^= 1
            bits.append(bit)

    # Pack bits into bytes, LSB first
    data = []
    for i in range(0, len(bits), 8):
        byte = 0
        for b in range(8):
            if i + b < len(bits):
                byte |= (bits[i + b] & 1) << b
        data.append(byte)
    return data


def format_c_array(name, data):
    lines = []
    lines.append(f"static const unsigned char PROGMEM {name}[] = {{")
    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        line = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"  {line},")
    lines.append("};")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True, help="Path to input image")
    parser.add_argument("--name", required=True, help="C array name")
    parser.add_argument("--width", type=int, default=16)
    parser.add_argument("--height", type=int, default=16)
    parser.add_argument("--invert", action="store_true")
    args = parser.parse_args()

    img_path = Path(args.input)
    if not img_path.exists():
        raise SystemExit(f"File not found: {img_path}")

    img = Image.open(img_path)
    data = image_to_bits(img, args.width, args.height, args.invert)
    print(format_c_array(args.name, data))


if __name__ == "__main__":
    main()
