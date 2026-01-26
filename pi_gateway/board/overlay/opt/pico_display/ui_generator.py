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
    # Change to Vertical Gradient (Top to Bottom) for more premium feel
    for y in range(config.LCD_HEIGHT):
        p = y / float(config.LCD_HEIGHT - 1)
        r,g,b = [int(c1 + (c2-c1)*p) for c1,c2 in zip((r1,g1,b1), (r2,g2,b2))]
        draw.line([(0, y), (config.LCD_WIDTH, y)], fill=(r,g,b))
    return image

def _create_weather_icon(icon_name: str, size: tuple[int, int], is_stale: bool = False) -> Image.Image:
    icon = Image.new('RGBA', size, (0,0,0,0)); draw = ImageDraw.Draw(icon); w,h=size
    if is_stale:
        C_MAIN = (180, 180, 180); C_SUN = (180, 160, 100); C_CLOUD = (150, 150, 150)
    else:
        C_MAIN = (255, 255, 255); C_SUN = (255, 204, 0); C_CLOUD = (220, 220, 220)
    
    def ds(o=(0,0),c=C_SUN): # Enhanced Sun
        cx,cy,r = w/2+o[0], h/2+o[1], 20
        draw.ellipse([(cx-r,cy-r),(cx+r,cy+r)], fill=c)
        for i in range(12):
            ang = radians(i*30); x1=cx+cos(ang)*(r+3); y1=cy+sin(ang)*(r+3); x2=cx+cos(ang)*(r+8); y2=cy+sin(ang)*(r+8)
            draw.line([(x1,y1),(x2,y2)], fill=c, width=3)

    def dc(o=(0,0),c=C_MAIN): # Smooth Moon
        cx,cy,r = w/2+o[0], h/2+o[1], 20
        draw.ellipse([(cx-r,cy-r),(cx+r,cy+r)], fill=c)
        draw.ellipse([(cx-r+10,cy-r-4),(cx+r+10,cy+r-4)], fill=(0,0,0,0)) # Masking circle for transparent cutout
        # Instead of transparency mask with fill, we'll draw it as a crescent
        # Actually, to avoid background bleed issues in simplified draw, we'll use a darker version of the gradient color or black for the cutout
        draw.ellipse([(cx-r+12,cy-r-4),(cx+r+10,cy+r-4)], fill=(20, 10, 50, 255)) 

    def dcl(o=(0,0),c=C_CLOUD):
        x,y = o[0]+w/2, o[1]+h/2
        draw.ellipse([(x-35,y-5),(x+10,y+25)], fill=c)
        draw.ellipse([(x-15,y-20),(x+35,y+20)], fill=c)

    if icon_name=="sun": ds()
    elif icon_name=="moon": dc()
    elif icon_name=="sun_cloud": ds((-8,-8)); dcl((10,12))
    elif icon_name=="moon_cloud": dc((-8,-8)); dcl((10,12))
    else: dcl()
    return icon

def _draw_info_icon(icon_type: str, size: tuple, color: tuple) -> Image.Image:
    icon = Image.new('RGBA', size, (0, 0, 0, 0)); draw = ImageDraw.Draw(icon); w,h = size
    if icon_type == 'wind':
        draw.arc((0, 4, w-4, h), 180, 270, fill=color, width=2)
        draw.line((w-4, h/2, w, h/2), fill=color, width=2)
    elif icon_type == 'humidity':
        draw.ellipse((4, 8, w-4, h-2), fill=color)
        draw.polygon([(w/2, 2), (4, h*0.6), (w-4, h*0.6)], fill=color)
    elif icon_type == 'sunrise':
        draw.line((2, h-2, w-2, h-2), fill=color, width=2)
        draw.arc((2, 4, w-2, h+4), 210, 330, fill=color, width=2)
    elif icon_type == 'sunset':
        draw.line((2, h-2, w-2, h-2), fill=color, width=2)
        draw.arc((2, -2, w-2, h-2), 30, 150, fill=color, width=2)
    return icon

def _draw_location_pin(size: tuple, color: tuple) -> Image.Image:
    icon = Image.new('RGBA', size, (0, 0, 0, 0)); draw = ImageDraw.Draw(icon); w,h = size
    draw.ellipse((w/4, 0, 3*w/4, h/2), fill=color)
    draw.polygon([(w/4, h/4), (3*w/4, h/4), (w/2, h)], fill=color)
    draw.ellipse((w/2-2, h/4-2, w/2+2, h/4+2), fill=(0,0,0,100))
    return icon

def create_ui_image(time_str: str, date_str: str, weather_info: dict | None, is_stale: bool = False, stale_age_str: str = "") -> Image.Image:
    theme = config.get_current_theme()
    image = _draw_background(theme)
    draw = ImageDraw.Draw(image)
    primary_color = config.COLOR_STALE if is_stale else theme["text_primary"]
    secondary_color = config.COLOR_STALE if is_stale else theme["text_secondary"]
    
    # Visual Separator
    draw.line([(20, 160), (config.LCD_WIDTH - 20, 160)], fill=(255,255,255,60), width=1)
    
    # Left Zone
    lx = 105
    draw.text((lx, 60), time_str, font=config.FONT_TIME, fill=primary_color, anchor="ms")
    draw.text((lx, 105), date_str, font=config.FONT_DATE, fill=secondary_color, anchor="ms")
    
    # Location with Pin
    pin_size = (16, 20)
    pin_img = _draw_location_pin(pin_size, secondary_color)
    loc_name = config.LOCATION_NAME[:18] 
    loc_w = draw.textlength(loc_name, font=config.FONT_LOCATION)
    px = int(lx - (loc_w / 2) - 12)
    image.paste(pin_img, (px, 122), pin_img)
    draw.text((lx + 6, 134), loc_name, font=config.FONT_LOCATION, fill=secondary_color, anchor="ms")

    # Right Zone
    rx = 262; cy = 45
    if weather_info:
        icon_img = _create_weather_icon(weather_info['icon'], (120, 100), is_stale)
        image.paste(icon_img, (rx - 60, cy - 50), icon_img)
        draw.text((rx, cy + 65), f"{weather_info['temperature']}°C", font=config.FONT_TEMP, fill=primary_color, anchor="ms")
        draw.text((rx, cy + 90), weather_info['description'].title(), font=config.FONT_WEATHER, fill=secondary_color, anchor="ms")
    
    # Bottom Zone
    cols = 4; col_w = config.LCD_WIDTH / cols
    labels = ["WIND", "HUMIDITY", "SUNRISE", "SUNSET"]
    keys = ["windspeed", "humidity", "sunrise", "sunset"]
    for i in range(cols):
        cx = int(col_w * (i + 0.5)); y = 180
        icon = _draw_info_icon(keys[i].split('speed')[0], (22, 22), secondary_color)
        image.paste(icon, (cx-11, y), icon)
        draw.text((cx, y+35), labels[i], font=config.FONT_INFO_HEADER, fill=secondary_color, anchor="ms")
        val = f"{weather_info[keys[i]]}{' km/h' if i==0 else '%' if i==1 else ''}" if weather_info else "..."
        draw.text((cx, y+55), val, font=config.FONT_INFO_VALUE, fill=primary_color, anchor="ms")

    return image.convert('RGB')