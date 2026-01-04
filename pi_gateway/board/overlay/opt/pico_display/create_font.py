# File: create_font.py

import argparse
import math
from PIL import Image, ImageDraw, ImageFont

def generate_font_data(ttf_path, size, first_char=32, last_char=126):
    try:
        font = ImageFont.truetype(ttf_path, size)
    except IOError:
        print(f"Error: Could not load font file at '{ttf_path}'")
        return None, None, 0, 0, 0

    max_width = 0
    max_height = 0
    char_widths = []

    # First pass: find max dimensions and individual widths
    for i in range(first_char, last_char + 1):
        char = chr(i)
        try:
            bbox = font.getbbox(char)
            char_width = bbox[2] - bbox[0]
            char_height = bbox[3] - bbox[1]
        except AttributeError: # Fallback for older Pillow versions
            char_width, char_height = font.getsize(char)

        char_widths.append(char_width if char_width > 0 else 1) # Store individual width
        if char_width > max_width:
            max_width = char_width
        if char_height > max_height:
            max_height = char_height
    
    if max_width == 0 or max_height == 0:
        print("Warning: Font could not be rendered.")
        return None, None, 0, 0, 0

    bytes_per_row = math.ceil(max_width / 8.0)
    font_data = bytearray()
    
    print(f"Generating font. Max character size: {max_width}x{max_height} pixels")

    # Second pass: render characters into the bitmap
    for char_code in range(first_char, last_char + 1):
        char = chr(char_code)
        image = Image.new('1', (max_width, max_height), 0)
        draw = ImageDraw.Draw(image)
        draw.text((0, 0), char, font=font, fill=1)

        for y in range(max_height):
            for byte_idx in range(bytes_per_row):
                byte = 0
                for bit_idx in range(8):
                    x = byte_idx * 8 + bit_idx
                    if x >= max_width:
                        break
                    if image.getpixel((x, y)) > 0:
                        byte |= (1 << bit_idx)
                font_data.append(byte)

    return font_data, bytearray(char_widths), max_width, max_height, bytes_per_row

def write_c_header(output_path, var_name, font_data, char_widths, width, height, bytes_per_row, first_char, last_char):
    with open(output_path, 'w') as f:
        header_guard = f"FONT_{var_name.upper()}_H"
        f.write(f"#ifndef {header_guard}\n#define {header_guard}\n\n")
        f.write('#include "CustomFont.h"\n\n')

        # Write bitmap data
        f.write(f"// Font data for {var_name} ({width}x{height})\n")
        f.write(f"static const uint8_t {var_name}_data[] = {{\n")
        for i, byte in enumerate(font_data):
            f.write("    ") if i % 16 == 0 else ""
            f.write(f"0x{byte:02x}, ")
            f.write("\n") if (i + 1) % 16 == 0 else ""
        f.write("\n};\n\n")

        # Write character widths data
        f.write(f"// Character widths for {var_name}\n")
        f.write(f"static const uint8_t {var_name}_widths[] = {{\n")
        for i, w in enumerate(char_widths):
            f.write("    ") if i % 16 == 0 else ""
            f.write(f"{w}, ")
            f.write("\n") if (i + 1) % 16 == 0 else ""
        f.write("\n};\n\n")

        # Write the font struct
        f.write(f"const custom_font_t {var_name} = {{\n")
        f.write(f"    .data = {var_name}_data,\n")
        f.write(f"    .widths = {var_name}_widths,\n")
        f.write(f"    .width = {width},\n")
        f.write(f"    .height = {height},\n")
        f.write(f"    .bytes_per_row = {bytes_per_row},\n")
        f.write(f"    .first_char = {first_char},\n")
        f.write(f"    .last_char = {last_char}\n")
        f.write("};\n\n")

        f.write(f"#endif // {header_guard}\n")
    
    print(f"Successfully created font header: {output_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert TTF font to a C header.")
    parser.add_argument("ttf_file", help="Path to the input .ttf font file.")
    parser.add_argument("size", type=int, help="Font size in pixels.")
    parser.add_argument("output_file", help="Path for the output .h header file.")
    parser.add_argument("--var_name", default="custom_font", help="C variable name for the font (e.g., 'font_my_cool_font').")
    parser.add_argument("--first_char", type=int, default=32, help="ASCII code of the first character to include (default: 32 - space).")
    parser.add_argument("--last_char", type=int, default=126, help="ASCII code of the last character to include (default: 126 - ~).")

    args = parser.parse_args()

    data, widths, w, h, bpr = generate_font_data(args.ttf_file, args.size, args.first_char, args.last_char)
    
    if data:
        write_c_header(args.output_file, args.var_name, data, widths, w, h, bpr, args.first_char, args.last_char)