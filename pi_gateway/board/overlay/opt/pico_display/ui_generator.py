# File: ui_generator.py
from PIL import Image, ImageDraw
from math import sin, cos, radians
import config
import struct

# --- Data Conversion Functions (These are correct and unchanged) ---
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

# --- Drawing Helper Functions ---
def _draw_background(theme: dict) -> Image.Image:
    image = Image.new('RGB', (config.LCD_WIDTH, config.LCD_HEIGHT))
    draw = ImageDraw.Draw(image)
    r1, g1, b1 = theme["gradient_start"]
    r2, g2, b2 = theme["gradient_end"]
    for x in range(config.LCD_WIDTH):
        p = x / float(config.LCD_WIDTH - 1)
        r,g,b = [int(c1 + (c2-c1)*p) for c1,c2 in zip((r1,g1,b1), (r2,g2,b2))]
        draw.line([(x, 0), (x, config.LCD_HEIGHT)], fill=(r,g,b))
    return image

def _create_weather_icon(icon_name: str, size: tuple[int, int], is_stale: bool = False) -> Image.Image:
    # Handle transparency
    icon = Image.new('RGBA', size, (0,0,0,0)); draw = ImageDraw.Draw(icon); w,h=size
    
    # Define base colors. If stale, desaturate/dim them.
    if is_stale:
        C_SUN = (180, 160, 100) # Dim gold
        C_MOON = (180, 180, 180) # Grey
        C_CLOUD = (150, 150, 150) # Grey
    else:
        C_SUN = (255,204,0)
        C_MOON = (240,240,230)
        C_CLOUD = (220,220,220)
        
    SHADOW = (0,0,0,50)
    so = (4,4)
    def ds(o=(0,0),c=C_SUN): ce,r=(w/2+o[0],h/2+o[1]),22;draw.ellipse([(ce[0]-r,ce[1]-r),(ce[0]+r,ce[1]+r)],fill=c);[draw.line([(ce[0]+cos(radians(i*30))*(r+4),ce[1]+sin(radians(i*30))*(r+4)),(ce[0]+cos(radians(i*30))*(r+10),ce[1]+sin(radians(i*30))*(r+10))],fill=c,width=4) for i in range(12)]
    def dc(o=(0,0),c=C_MOON): cx,cy,r=w/2+o[0],h/2+o[1],22;draw.ellipse([(cx-r,cy-r),(cx+r,cy+r)],fill=c);draw.ellipse([(cx-r+10,cy-r-4),(cx+r+10,cy+r-4)],fill=(0,0,0,80))
    def dcl(o=(0,0),c=C_CLOUD): x,y=o[0]+w/2,o[1]+h/2;draw.ellipse([(x-40,y-5),(x+15,y+30)],fill=c);draw.ellipse([(x-20,y-25),(x+40,y+20)],fill=c)
    
    if icon_name=="sun": ds(so,SHADOW);ds()
    elif icon_name=="moon": dc(so,SHADOW);dc()
    elif icon_name=="sun_cloud": dcl((5,8),SHADOW);ds((-12,-12));dcl()
    elif icon_name=="moon_cloud": dcl((5,8),SHADOW);dc((-12,-12));dcl()
    else: dcl(so,SHADOW);dcl()
    return icon

def _draw_info_icon(icon_type: str, size: tuple, color: tuple) -> Image.Image:
    icon = Image.new('RGBA', size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(icon)
    w, h = size # Unpack the tuple here
    if icon_type == 'wind':
        draw.arc((0, 2, w, h + 2), 180, 270, fill=color, width=2)
        draw.arc((-5, 7, w - 5, h + 7), 160, 260, fill=color, width=2)
    elif icon_type == 'humidity':
        draw.pieslice((2, 2, w-2, h-2), -45, 225, fill=color)
        draw.polygon([(w/2, 0), (2, h * 0.7), (w-2, h * 0.7)], fill=color)
    elif icon_type == 'sunrise':
        draw.line((0, h, w, h), fill=color, width=2)
        draw.arc((0, h/2, w, h * 1.5), 190, 350, fill=color, width=2)
    elif icon_type == 'sunset':
        draw.line((0, h, w, h), fill=color, width=2)
        draw.arc((0, -h/2 + h, w, h/2 + h), 10, 170, fill=color, width=2)
    return icon

def create_ui_image(time_str: str, date_str: str, weather_info: dict | None, is_stale: bool = False, stale_age_str: str = "") -> Image.Image:
    """
    Composes the final, polished UI with a 3-zone landscape layout.
    """
    theme = config.get_current_theme()
    image = _draw_background(theme)
    draw = ImageDraw.Draw(image)
    
    # --- Define Colors based on Stale State ---
    primary_color = config.COLOR_STALE if is_stale else theme["text_primary"]
    secondary_color = config.COLOR_STALE if is_stale else theme["text_secondary"]
    
    # --- Define Layout Zones ---
    # Moved separator down to 160 to give top section more room
    separator_y = 160 
    
    # --- Visual Separator ---
    draw.line([(15, separator_y), (config.LCD_WIDTH - 15, separator_y)], fill=(255,255,255,50), width=1)
    
    # =========================================================================
    # --- Top Zone (Split into Left and Right)
    # =========================================================================
    top_center_y = separator_y // 2 # 80
    
    # --- Adjust the center point for the left column ---
    left_center_x = 90
    
    right_center_x = 240
    
    # --- Top-Left: Time and Date (Always Valid) ---
    draw.text((left_center_x, 65), time_str, font=config.FONT_TIME, fill=theme["text_primary"], anchor="ms")
    draw.text((left_center_x, 110), date_str, font=config.FONT_DATE, fill=theme["text_secondary"], anchor="ms")

    # --- Top-Right: Weather ---
    if weather_info:
        icon_size = (90, 70)
        icon_img = _create_weather_icon(weather_info['icon'], icon_size, is_stale)
        # Shifted icon up slightly due to new top_center_y
        image.paste(icon_img, (right_center_x - icon_size[0] // 2, top_center_y - 80), icon_img)
        
        temp_text = f"{weather_info['temperature']}°C"
        desc_text = weather_info['description']
        
        # Compacted vertical spacing slightly
        draw.text((right_center_x, top_center_y + 25), temp_text, font=config.FONT_TEMP, fill=primary_color, anchor="ms")
        draw.text((right_center_x, top_center_y + 50), desc_text, font=config.FONT_WEATHER, fill=secondary_color, anchor="ms")

        # --- Show Data Age if Stale ---
        if is_stale and stale_age_str:
             # Moved to +70 to fit above separator (160)
             draw.text((right_center_x, top_center_y + 70), f"Updated {stale_age_str} ago", font=config.FONT_DATA_AGE, fill=config.COLOR_STALE, anchor="ms")

    else:
        # --- Placeholder Mode (No Data) ---
        draw.text((right_center_x, top_center_y + 30), "--°C", font=config.FONT_TEMP, fill=config.COLOR_STALE, anchor="ms")
        draw.text((right_center_x, top_center_y + 60), "No Data", font=config.FONT_WEATHER, fill=config.COLOR_STALE, anchor="ms")
        
        # Draw a simple placeholder circle for icon
        r = 20
        cx, cy = right_center_x, top_center_y - 40
        draw.ellipse([(cx-r, cy-r), (cx+r, cy+r)], outline=config.COLOR_STALE, width=2)
        draw.text((cx, cy), "?", font=config.FONT_INFO_VALUE, fill=config.COLOR_STALE, anchor="mm")


    # =========================================================================
    # --- Bottom Zone: Additional Info
    # =========================================================================
    
    # Common layout calc
    num_cols = 4
    col_width = config.LCD_WIDTH / num_cols
    col_centers = [int(col_width * (i + 0.5)) for i in range(num_cols)]
    
    y_pos_icon = separator_y + 25
    y_pos_header = y_pos_icon + 20
    y_pos_value = y_pos_header + 22
    icon_size_small = 20

    # Define headers and logic for values
    labels = ["Wind", "Humidity", "Sunrise", "Sunset"]
    icon_types = ["wind", "humidity", "sunrise", "sunset"]
    
    if weather_info:
        values = [
            f"{weather_info['windspeed']} km/h",
            f"{weather_info['humidity']}%",
            weather_info['sunrise'],
            weather_info['sunset']
        ]
        active_icon_color = secondary_color
        active_val_color = primary_color
    else:
        values = ["...", "...", "--:--", "--:--"]
        active_icon_color = config.COLOR_STALE
        active_val_color = config.COLOR_STALE

    for i in range(4):
        # Draw Icon
        icon_img = _draw_info_icon(icon_types[i], (icon_size_small, icon_size_small), active_icon_color)
        image.paste(icon_img, (col_centers[i] - icon_size_small // 2, y_pos_icon - icon_size_small // 2), icon_img)
        
        # Draw Header
        draw.text((col_centers[i], y_pos_header), labels[i], font=config.FONT_INFO_HEADER, fill=active_icon_color, anchor="ms")
        
        # Draw Value
        draw.text((col_centers[i], y_pos_value), values[i], font=config.FONT_INFO_VALUE, fill=active_val_color, anchor="ms")
        
    return image.convert('RGB')