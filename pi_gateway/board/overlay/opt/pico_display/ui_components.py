# File: ui_components.py
from PIL import Image, ImageDraw
from math import sin, cos, radians
import config

def draw_background(theme: dict) -> Image.Image:
    """Creates a vertical gradient background."""
    image = Image.new('RGBA', (config.LCD_WIDTH, config.LCD_HEIGHT))
    draw = ImageDraw.Draw(image)
    r1, g1, b1 = theme["gradient_start"]
    r2, g2, b2 = theme["gradient_end"]
    
    for y in range(config.LCD_HEIGHT):
        p = y / float(config.LCD_HEIGHT - 1)
        r, g, b = [int(c1 + (c2 - c1) * p) for c1, c2 in zip((r1, g1, b1), (r2, g2, b2))]
        draw.line([(0, y), (config.LCD_WIDTH, y)], fill=(r, g, b, 255))
    return image

def create_weather_icon(icon_name: str, size: tuple[int, int], is_stale: bool = False) -> Image.Image:
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

def draw_info_icon(icon_type: str, size: tuple, color: tuple) -> Image.Image:
    icon = Image.new('RGBA', size, (0,0,0,0)); draw = ImageDraw.Draw(icon); w,h = size
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

def draw_location_pin(size: tuple, color: tuple) -> Image.Image:
    icon = Image.new('RGBA', size, (0, 0, 0, 0)); draw = ImageDraw.Draw(icon); w,h = size
    draw.ellipse((w/4, 0, 3*w/4, h/2), fill=color)
    draw.polygon([(w/4, h/4), (3*w/4, h/4), (w/2, h)], fill=color)
    draw.ellipse((w/2-2, h/4-2, w/2+2, h/4+2), fill=(0,0,0,100))
    return icon

def draw_glass_card(draw_base, draw_overlay, x, y, width, height, title, data, theme, current_time):
    """Draws a translucent 'glass' card with sensor data."""
    # Determine status
    is_stale = True
    if data:
        age = current_time - data.get('timestamp', 0)
        if age < 300: # 5 minutes
            is_stale = False
    
    p_color = (150, 150, 150, 255) if is_stale else (*theme["text_primary"], 255)
    s_color = (120, 120, 120, 255) if is_stale else (*theme["text_secondary"], 255)
    
    # Draw Glassmorphism Card (subtle translucency)
    # Using very low alpha for 'glass' effect
    box = [x + 4, y + 4, x + width - 4, y + height - 4]
    draw_overlay.rounded_rectangle(box, radius=8, fill=(255, 255, 255, 10), outline=(255, 255, 255, 20))
    
    # Draw Sensor Name (Left, baseline aligned)
    draw_base.text((x + 14, y + 22), title, font=config.FONT_WEATHER, fill=s_color, anchor="lb")
    
    if data:
        # Last Seen (Right, baseline aligned to match name)
        age = current_time - data.get('timestamp', 0)
        age_str = f"{int(age)}s" if age < 60 else f"{int(age/60)}m"
        draw_base.text((x + width - 14, y + 22), age_str, font=config.FONT_INFO_HEADER, fill=s_color, anchor="rb")
        
        # Main Values
        temp_str = f"{data['temp']:.1f}°C"
        hum_str = f"{data['hum']}%"
        
        # Draw Temp (Bottom Left)
        draw_base.text((x + 14, y + height - 6), temp_str, font=config.FONT_INFO_VALUE, fill=p_color, anchor="ld")
        # Draw Humidity (Bottom Right)
        draw_base.text((x + width - 14, y + height - 6), hum_str, font=config.FONT_INFO_VALUE, fill=p_color, anchor="rd")
    else:
        # Default placeholder if no data
        draw_base.text((x + 14, y + height - 6), "--.-°C", font=config.FONT_INFO_VALUE, fill=(100, 100, 100, 255), anchor="ld")
        draw_base.text((x + width - 14, y + height - 6), "--%", font=config.FONT_INFO_VALUE, fill=(100, 100, 100, 255), anchor="rd")
