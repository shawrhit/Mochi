import os
import subprocess
import glob
import sys

try:
    from PIL import Image
except ImportError:
    print("Installing required package 'Pillow'...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "Pillow", "--break-system-packages"])
    from PIL import Image

INPUT_DIR = "videos"
OUTPUT_FILE = "src/custom_animations_master.h"
FPS = 20
WIDTH = 128
HEIGHT = 64

def generate_cpp_array(image_path, array_name):
    # Open image, resize, and convert to 1-bit pixels (dithered)
    img = Image.open(image_path)
    
    # We use Floyd-Steinberg dithering (default in PIL) to make the B&W look great
    img = img.convert('1')
    pixels = img.load()
    
    hex_data = []
    # Adafruit_GFX drawBitmap format: horizontal, 1 bit per pixel
    for y in range(HEIGHT):
        for x_byte in range(0, WIDTH, 8):
            byte_val = 0
            for bit in range(8):
                if x_byte + bit < WIDTH:
                    # '1' mode in PIL: 0 is black, 255 is white
                    # For OLED, 1 means light up (white), 0 means off
                    pixel_on = 1 if pixels[x_byte + bit, y] > 127 else 0
                    byte_val |= (pixel_on << (7 - bit))
            hex_data.append(f"0x{byte_val:02x}")
            
    # Format into C array string
    array_str = f"const unsigned char {array_name} [] PROGMEM = {{\n\t"
    for i in range(0, len(hex_data), 16):
        array_str += ", ".join(hex_data[i:i+16]) + ", \n\t"
    # Remove last comma and space
    array_str = array_str.rstrip("\n\t, ")
    array_str += "\n};\n"
    
    return array_str

def main():
    if not os.path.exists(INPUT_DIR):
        os.makedirs(INPUT_DIR)
        print(f"Created '{INPUT_DIR}' directory in your workspace.")
        print(f"Please place your MP4/MOV videos in the '{INPUT_DIR}' folder and run this script again.")
        return

    video_files = glob.glob(os.path.join(INPUT_DIR, "*.*"))
    videos = [f for f in video_files if f.lower().endswith(('.mp4', '.mov', '.avi', '.mkv'))]
    
    if not videos:
        print(f"No videos found in '{INPUT_DIR}'. Please add some and run again.")
        return

    # Check if FFmpeg is installed
    try:
        subprocess.run(["ffmpeg", "-version"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except FileNotFoundError:
        print("Error: FFmpeg is not installed. Please install FFmpeg to extract frames.")
        return

    with open(OUTPUT_FILE, "w") as out_f:
        out_f.write("#pragma once\n")
        out_f.write("#include <Arduino.h>\n\n")

        for video_path in videos:
            video_name = os.path.splitext(os.path.basename(video_path))[0]
            # Clean up name for C variable compatibility
            video_name = ''.join(c for c in video_name if c.isalnum() or c == '_')
            print(f"\nProcessing video: {video_name}")
            
            # Temp directory for frames
            temp_dir = f"temp_{video_name}"
            os.makedirs(temp_dir, exist_ok=True)
            
            # Extract frames using FFmpeg
            # fps=20, scale to maintain aspect ratio, then crop/pad to 128x64 with black bars to prevent stretching
            print(f"  Extracting frames at {FPS} FPS...")
            cmd = [
                "ffmpeg", "-y", "-i", video_path, 
                "-vf", f"fps={FPS},scale={WIDTH}:{HEIGHT}:force_original_aspect_ratio=decrease,pad={WIDTH}:{HEIGHT}:(ow-iw)/2:(oh-ih)/2", 
                f"{temp_dir}/frame_%04d.png"
            ]
            
            subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            
            frames = sorted(glob.glob(os.path.join(temp_dir, "*.png")))
            print(f"  Found {len(frames)} frames. Converting to PROGMEM with Floyd-Steinberg Dithering...")
            
            array_names = []
            for i, frame in enumerate(frames):
                frame_name = f"{video_name}_frame_{i:04d}"
                array_names.append(frame_name)
                
                c_array = generate_cpp_array(frame, frame_name)
                out_f.write(c_array)
                out_f.write("\n")
                
            # Write the pointer array and frame count for this specific video
            out_f.write(f"const int {video_name}_frames_count = {len(frames)};\n")
            out_f.write(f"const unsigned char* const {video_name}_allArray[] PROGMEM = {{\n\t")
            for i in range(0, len(array_names), 8):
                out_f.write(", ".join(array_names[i:i+8]) + ",\n\t")
            
            # Remove trailing comma
            out_f.seek(out_f.tell() - 3, os.SEEK_SET)
            out_f.write("\n};\n\n")
            
            # Cleanup temp dir
            for f in frames:
                os.remove(f)
            os.rmdir(temp_dir)
            
            print(f"  Finished {video_name}!")
            
    print(f"\nDone! Generated C arrays in '{OUTPUT_FILE}'.")
    print(f"You can now include '{os.path.basename(OUTPUT_FILE)}' in your main.cpp to play the videos!")

if __name__ == "__main__":
    main()
