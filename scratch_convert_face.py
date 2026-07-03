import sys
from PIL import Image

def image_to_c_array(image_path):
    # Open and convert to grayscale
    img = Image.open(image_path).convert('L')
    
    # Calculate proportional resize to fit within 128x64
    target_width, target_height = 128, 64
    width_ratio = target_width / img.width
    height_ratio = target_height / img.height
    ratio = min(width_ratio, height_ratio)
    
    new_width = int(img.width * ratio)
    new_height = int(img.height * ratio)
    img = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
    
    # Create a new black 128x64 image and paste the resized image in the center
    final_img = Image.new('1', (128, 64), 0)
    
    # Convert resized to 1-bit using a threshold
    # Assuming white face on black background. 
    # If it's black on white, we'd need to invert it. Let's use a threshold.
    img = img.point(lambda p: 1 if p > 128 else 0, mode='1')
    
    offset_x = (128 - new_width) // 2
    offset_y = (64 - new_height) // 2
    final_img.paste(img, (offset_x, offset_y))
    
    bytes_list = []
    for y in range(64):
        for x in range(0, 128, 8):
            byte = 0
            for bit in range(8):
                pixel = final_img.getpixel((x + bit, y))
                if pixel > 0:
                    # In Adafruit_GFX, LSB is the rightmost pixel or MSB?
                    # Usually MSB is the leftmost pixel for drawBitmap
                    byte |= (1 << (7 - bit))
            bytes_list.append(f"0x{byte:02x}")
            
    # Format to C array
    output = "static const unsigned char PROGMEM image_mochi_face_bits[] = {\n"
    for i in range(0, len(bytes_list), 12):
        output += "  " + ", ".join(bytes_list[i:i+12]) + ",\n"
    output = output.rstrip(",\n") + "\n};\n"
    return output

if __name__ == "__main__":
    print(image_to_c_array('mochi_face.jpg'))
