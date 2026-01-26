import config
from PIL import Image, ImageDraw
import time

def _draw_background(theme):
    image = Image.new('RGB', (config.LCD_WIDTH, config.LCD_HEIGHT))
    draw = ImageDraw.Draw(image)
    r1, g1, b1 = theme["gradient_start"]
    r2, g2, b2 = theme["gradient_end"]
    for y in range(config.LCD_HEIGHT):
        p = y / float(config.LCD_HEIGHT - 1)
        r,g,b = [int(c1 + (c2-c1)*p) for c1,c2 in zip((r1,g1,b1), (r2,g2,b2))]
        draw.line([(0, y), (config.LCD_WIDTH, y)], fill=(r,g,b))
    return image

def create_sensor_ui_image(sensor_data: dict) -> Image.Image:
    theme = config.get_current_theme()
    image = _draw_background(theme)
    draw = ImageDraw.Draw(image)
    
    # Header
    draw.text((config.LCD_WIDTH/2, 25), "HOME SENSORS", font=config.FONT_LOCATION, fill=theme["text_secondary"], anchor="ms")
    draw.line([(20, 35), (config.LCD_WIDTH-20, 35)], fill=(255,255,255,60), width=1)

    # Sensors Grid (2x3)
    margin_x = 15; margin_y = 50
    col_w = (config.LCD_WIDTH - 2*margin_x) / 2
    row_h = (config.LCD_HEIGHT - margin_y - 10) / 3
    
    names = ["Kindr", "Wohn", "Flur", "Bad", "Kuche", "Schlf"]
    
    for i, name in enumerate(names):
        col = i % 2
        row = i // 2
        x = int(margin_x + col * col_w)
        y = int(margin_y + row * row_h)
        
        # Draw Glassmorphism Card (subtle)
        draw.rectangle([x+2, y+2, x+col_w-2, y+row_h-2], fill=(255,255,255,20), outline=(255,255,255,40))
        
        data = sensor_data.get(name)
        
        # Determine status
        is_stale = True
        if data:
            age = time.time() - data.get('timestamp', 0)
            if age < 300: # 5 minutes
                is_stale = False
        
        p_color = (150, 150, 150) if is_stale else theme["text_primary"]
        s_color = (120, 120, 120) if is_stale else theme["text_secondary"]
        
        # Draw Name
        draw.text((x + 10, y + 8), name, font=config.FONT_WEATHER, fill=s_color)
        
        if data:
            # Temperature
            temp_str = f"{data['temp']:.1f}C"
            draw.text((x + 10, y + 30), temp_str, font=config.FONT_INFO_VALUE, fill=p_color)
            
            # Humidity
            hum_str = f"{data['hum']}%"
            draw.text((x + col_w - 45, y + 30), hum_str, font=config.FONT_INFO_VALUE, fill=p_color)
            
            # Last Seen
            age = time.time() - data.get('timestamp', 0)
            age_str = f"{int(age)}s" if age < 60 else f"{int(age/60)}m"
            draw.text((x + col_w - 45, y + 8), age_str, font=config.FONT_INFO_HEADER, fill=s_color)
        else:
            draw.text((x + 10, y + 30), "--.-C", font=config.FONT_INFO_VALUE, fill=(100, 100, 100))
            draw.text((x + col_w - 45, y + 30), "--%", font=config.FONT_INFO_VALUE, fill=(100, 100, 100))

    return image.convert('RGB')
