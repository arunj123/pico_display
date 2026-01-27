# File: ui_components.py
from PIL import Image, ImageDraw
from math import sin, cos, radians
import os
import config

# Load humidity emoji PNGs
_EMOJI_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'emoji')
_EMOJI_CACHE = {}

def _load_emoji(name, size=16):
    """Loads and caches a resized emoji PNG."""
    cache_key = f"{name}_{size}"
    if cache_key not in _EMOJI_CACHE:
        try:
            path = os.path.join(_EMOJI_DIR, f"{name}.png")
            img = Image.open(path).convert('RGBA')
            img = img.resize((size, size), Image.Resampling.LANCZOS)
            _EMOJI_CACHE[cache_key] = img
        except Exception:
            _EMOJI_CACHE[cache_key] = None
    return _EMOJI_CACHE[cache_key]

def _get_humidity_emoji(hum):
    """Returns emoji name based on humidity level (face icons like TP357)."""
    if hum < 30:
        return "sad"      # Too dry 😟
    elif hum <= 60:
        return "happy"    # Good 😊
    else:
        return "neutral"  # Too humid 😐

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
    """Loads a weather emoji icon from the emoji directory."""
    emoji_img = _load_emoji(icon_name, size=max(size))
    if emoji_img:
        # Resize to exact size if needed
        if emoji_img.size != size:
            emoji_img = emoji_img.resize(size, Image.Resampling.LANCZOS)
        if is_stale:
            # Convert to grayscale for stale data
            emoji_img = emoji_img.convert('LA').convert('RGBA')
        return emoji_img
    
    # Fallback: return a simple placeholder if emoji not found
    icon = Image.new('RGBA', size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(icon)
    draw.text((size[0]/2, size[1]/2), "?", fill=(200, 200, 200, 255), anchor="mm")
    return icon


def draw_info_icon(icon_type: str, size: tuple, color: tuple) -> Image.Image:
    """Loads an info icon emoji from the emoji directory."""
    # Map icon types to emoji filenames
    icon_map = {
        'wind': 'wind',
        'humidity': 'humidity_icon',
        'sunrise': 'sunrise',
        'sunset': 'sunset'
    }
    emoji_name = icon_map.get(icon_type)
    if emoji_name:
        emoji_img = _load_emoji(emoji_name, size=max(size))
        if emoji_img:
            if emoji_img.size != size:
                emoji_img = emoji_img.resize(size, Image.Resampling.LANCZOS)
            return emoji_img
    
    # Fallback: empty icon
    return Image.new('RGBA', size, (0, 0, 0, 0))

def draw_location_pin(size: tuple, color: tuple) -> Image.Image:
    """Loads location pin emoji from the emoji directory."""
    emoji_img = _load_emoji('location', size=max(size))
    if emoji_img:
        if emoji_img.size != size:
            emoji_img = emoji_img.resize(size, Image.Resampling.LANCZOS)
        return emoji_img
    # Fallback: empty icon
    return Image.new('RGBA', size, (0, 0, 0, 0))

def _get_temp_color(temp):
    """Returns color based on temperature: cold=blue, comfortable=green, hot=red."""
    if temp < 16:
        return (100, 150, 255, 255)  # Cold - Blue
    elif temp < 20:
        return (100, 200, 255, 255)  # Cool - Light Blue
    elif temp <= 24:
        return (100, 255, 150, 255)  # Comfortable - Green
    elif temp <= 28:
        return (255, 200, 100, 255)  # Warm - Orange
    else:
        return (255, 100, 100, 255)  # Hot - Red

def _get_hum_color(hum):
    """Returns color based on humidity: low=yellow, good=green, high=cyan."""
    if hum < 30:
        return (255, 220, 100, 255)  # Dry - Yellow
    elif hum <= 60:
        return (100, 255, 150, 255)  # Good - Green
    else:
        return (100, 220, 255, 255)  # Humid - Cyan

def _get_comfort_indicator(temp, hum):
    """Returns a symbol and color based on overall comfort (deprecated - using emoji now)."""
    if 19 <= temp <= 24 and 30 <= hum <= 60:
        return ("*", (100, 255, 150, 255))  # Green - comfortable
    elif temp < 16 or temp > 28:
        return ("*", (255, 100, 100, 255))  # Red - too cold/hot
    elif hum < 25 or hum > 70:
        return ("*", (255, 200, 100, 255))  # Orange - too dry/humid
    else:
        return ("*", (200, 200, 200, 255))  # Gray - neutral

def draw_glass_card(draw_base, draw_overlay, image, x, y, width, height, title, data, theme, current_time):
    """Draws a translucent 'glass' card with color-coded sensor data."""
    # Determine status
    is_stale = True
    if data:
        age = current_time - data.get('timestamp', 0)
        if age < 300: # 5 minutes
            is_stale = False
    
    s_color = (120, 120, 120, 255) if is_stale else (*theme["text_secondary"], 255)
    
    # Draw Glassmorphism Card (subtle translucency)
    box = [x + 4, y + 4, x + width - 4, y + height - 4]
    draw_overlay.rounded_rectangle(box, radius=8, fill=(255, 255, 255, 10), outline=(255, 255, 255, 20))
    
    # Draw Sensor Name (Left, baseline aligned)
    draw_base.text((x + 14, y + 22), title, font=config.FONT_WEATHER, fill=s_color, anchor="lb")
    
    if data:
        temp = data['temp']
        hum = data['hum']
        
        # Last Seen (Right, baseline aligned to match name)
        age = current_time - data.get('timestamp', 0)
        age_str = f"{int(age)}s" if age < 60 else f"{int(age/60)}m"
        draw_base.text((x + width - 14, y + 22), age_str, font=config.FONT_INFO_HEADER, fill=s_color, anchor="rb")
        
        # Get colors based on values
        temp_color = (150, 150, 150, 255) if is_stale else _get_temp_color(temp)
        hum_color = (150, 150, 150, 255) if is_stale else _get_hum_color(hum)
        
        # Main Values with color coding
        temp_str = f"{temp:.1f}°"
        hum_str = f"{hum}%"
        
        # Draw Temperature (bottom left, large) - color coded only
        draw_base.text((x + 12, y + height - 8), temp_str, font=config.FONT_SENSOR_TEMP, fill=temp_color, anchor="ld")
        # Draw Humidity (bottom right, slightly smaller) - with emoji
        draw_base.text((x + width - 12, y + height - 8), hum_str, font=config.FONT_SENSOR_HUM, fill=hum_color, anchor="rd")
        
        # Draw humidity emoji (top center, between name and age)
        if not is_stale:
            emoji_name = _get_humidity_emoji(hum)
            emoji_img = _load_emoji(emoji_name, size=22)
            if emoji_img:
                # Position emoji in the center of the header row
                emoji_x = int(x + width/2 - 4)
                emoji_y = int(y + 6)
                image.paste(emoji_img, (emoji_x, emoji_y), emoji_img)
    else:
        # Default placeholder if no data
        draw_base.text((x + 12, y + height - 8), "--.-°", font=config.FONT_SENSOR_TEMP, fill=(100, 100, 100, 255), anchor="ld")
        draw_base.text((x + width - 12, y + height - 8), "--%", font=config.FONT_SENSOR_HUM, fill=(100, 100, 100, 255), anchor="rd")
