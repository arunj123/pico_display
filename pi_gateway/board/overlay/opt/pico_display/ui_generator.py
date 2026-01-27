# File: ui_generator.py
from PIL import Image, ImageDraw
import config
import struct
import ui_theme
import ui_components

# --- Data Conversion Functions (These are core protocol, kept here) ---
def convert_image_to_rgb565(img: Image.Image) -> bytes:
    img_rgb = img.convert('RGB')
    pixels = bytearray(img.width * img.height * 2)
    i = 0
    for y in range(img.height):
        for x in range(img.width):
            r, g, b = img_rgb.getpixel((x, y))
            rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            struct.pack_into("<H", pixels, i, rgb565)
            i += 2
    return bytes(pixels)

def reconstruct_image_from_rgb565(pixel_data: bytes, width: int, height: int) -> Image.Image:
    img = Image.new('RGB', (width, height))
    pixels = img.load()
    i = 0
    for y in range(height):
        for x in range(width):
            rgb565 = struct.unpack_from('<H', pixel_data, i)[0]
            r5, g6, b5 = (rgb565 >> 11) & 0x1F, (rgb565 >> 5) & 0x3F, rgb565 & 0x1F
            r8, g8, b8 = (r5 * 255 + 15) // 31, (g6 * 255 + 31) // 63, (b5 * 255 + 15) // 31
            pixels[x, y] = (r8, g8, b8)
            i += 2
    return img

def create_ui_image(time_str: str, date_str: str, weather_info: dict | None, is_stale: bool = False, stale_age_str: str = "") -> Image.Image:
    """Generates the main weather and time UI screen."""
    theme = ui_theme.get_current_theme()
    image = ui_components.draw_background(theme)
    draw = ImageDraw.Draw(image)
    primary_color = config.COLOR_STALE if is_stale else theme["text_primary"]
    secondary_color = config.COLOR_STALE if is_stale else theme["text_secondary"]
    
    # Visual Separator
    draw.line([(20, 160), (config.LCD_WIDTH - 20, 160)], fill=(255, 255, 255, 60), width=1)
    
    # Left Zone (Time and Date)
    lx = 105
    draw.text((lx, 60), time_str, font=config.FONT_TIME, fill=primary_color, anchor="ms")
    draw.text((lx, 105), date_str, font=config.FONT_DATE, fill=secondary_color, anchor="ms")
    
    # Location with Pin
    pin_size = (16, 20)
    pin_img = ui_components.draw_location_pin(pin_size, secondary_color)
    loc_name = config.LOCATION_NAME[:18] 
    loc_w = draw.textlength(loc_name, font=config.FONT_LOCATION)
    px = int(lx - (loc_w / 2) - 12)
    image.paste(pin_img, (px, 122), pin_img)
    draw.text((lx + 6, 134), loc_name, font=config.FONT_LOCATION, fill=secondary_color, anchor="ms")

    # Right Zone (Weather Icon and Temp)
    rx = 262; cy = 45
    if weather_info:
        icon_img = ui_components.create_weather_icon(weather_info['icon'], (60, 60), is_stale)
        image.paste(icon_img, (rx - 30, cy - 30), icon_img)
        draw.text((rx, cy + 65), f"{weather_info['temperature']}°C", font=config.FONT_TEMP, fill=primary_color, anchor="ms")
        draw.text((rx, cy + 90), weather_info['description'].title(), font=config.FONT_WEATHER, fill=secondary_color, anchor="ms")
    
    # Bottom Zone (Details)
    cols = 4; col_w = config.LCD_WIDTH / cols
    labels = ["Wind", "Humidity", "Sunrise", "Sunset"]
    keys = ["windspeed", "humidity", "sunrise", "sunset"]
    for i in range(cols):
        cx = int(col_w * (i + 0.5)); y = 172
        icon = ui_components.draw_info_icon(keys[i].split('speed')[0], (22, 22), secondary_color)
        image.paste(icon, (cx-11, y), icon)
        draw.text((cx, y+37), labels[i], font=config.FONT_INFO_HEADER, fill=secondary_color, anchor="ms")
        val = f"{weather_info[keys[i]]}{' km/h' if i==0 else '%' if i==1 else ''}" if weather_info else "..."
        draw.text((cx, y+57), val, font=config.FONT_INFO_VALUE, fill=primary_color, anchor="ms")

    return image.convert('RGB')

def create_sensor_ui_image(sensor_data: dict) -> Image.Image:
    """Generates the home sensor dashboard UI screen."""
    import time
    theme = ui_theme.get_current_theme()
    image = ui_components.draw_background(theme)
    # Overlay layer for alpha-blended elements (cards)
    overlay = Image.new('RGBA', image.size, (0,0,0,0))
    d_overlay = ImageDraw.Draw(overlay)
    draw = ImageDraw.Draw(image)
    
    # No header - use full space for sensor cards
    # Grid constants
    margin_x = 10; margin_y = 10
    col_w = (config.LCD_WIDTH - 2*margin_x) / 2
    row_h = (config.LCD_HEIGHT - margin_y - 6) / 3
    
    sensor_names = ["Kindr", "Wohn", "Flur", "Bad", "Kuche", "Schlf"]
    current_time = time.time()
    
    for i, name in enumerate(sensor_names):
        col, row = i % 2, i // 2
        x = int(margin_x + col * col_w)
        y = int(margin_y + row * row_h)
        
        ui_components.draw_glass_card(draw, d_overlay, image, x, y, col_w, row_h, name, sensor_data.get(name), theme, current_time)

    # Composite layers
    combined = Image.alpha_composite(image, overlay)
    return combined.convert('RGB')
