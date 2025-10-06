# File: config.py
import os
from PIL import ImageFont
import struct
from datetime import datetime
from random import randrange

# -- Device & Network Configuration --
PICO_IP = "192.168.0.122"
PICO_PORT = 4242

# -- Display & Protocol Configuration --
LCD_WIDTH = 320
LCD_HEIGHT = 240
TILE_PAYLOAD_SIZE = 8192

# --- FINAL Protocol Definitions ---
FRAME_MAGIC = 0xAA
FRAME_HEADER_FORMAT = "<BBH"
FRAME_HEADER_SIZE = struct.calcsize(FRAME_HEADER_FORMAT)
FRAME_TYPE_IMAGE_TILE = 0x02
FRAME_TYPE_TILE_ACK = 0x03 # Re-enabled
FRAME_TYPE_TILE_NACK = 0x04 # Re-enabled
IMAGE_TILE_HEADER_FORMAT = "<HHHHI"  # x, y, width, height, crc32
IMAGE_TILE_HEADER_SIZE = struct.calcsize(IMAGE_TILE_HEADER_FORMAT)
MAX_PIXEL_DATA_SIZE = TILE_PAYLOAD_SIZE - IMAGE_TILE_HEADER_SIZE

# -- Location & Weather --
LOCATION_LAT = 49.4247
LOCATION_LON = 11.0896
WEATHER_UPDATE_INTERVAL_SECONDS = 15 * 60

# -- UI Layout and Fonts (Polished Sizes) --
try:
    _CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
    FONT_PATH_BOLD = os.path.join(_CURRENT_DIR, '..', "fonts", "FreeSansBold.ttf")
    FONT_PATH_REGULAR = os.path.join(_CURRENT_DIR, '..', "fonts", "Ubuntu-L.ttf")
    
    FONT_TIME = ImageFont.truetype(FONT_PATH_BOLD, 72)
    FONT_DATE = ImageFont.truetype(FONT_PATH_REGULAR, 24)
    FONT_TEMP = ImageFont.truetype(FONT_PATH_BOLD, 42)
    FONT_WEATHER = ImageFont.truetype(FONT_PATH_REGULAR, 20)
    FONT_INFO_HEADER = ImageFont.truetype(FONT_PATH_REGULAR, 16)
    FONT_INFO_VALUE = ImageFont.truetype(FONT_PATH_BOLD, 18)
except IOError:
    raise IOError("Error: Font files not found.")

# -- UI Color Theme --
def get_current_theme():
    """Selects a color theme based on the current hour of the day."""
    hour = datetime.now().hour
    
    # For testing purposes, assign a number between 0-23
    # hour = randrange(0, 24)
    print(f"Selected hour for theme: {hour}")

    if 5 <= hour < 12: # Morning
        return {
            "name": "Morning Sky",
            "gradient_start": (0, 10, 60),      # Deep Ocean Blue
            "gradient_end": (10, 80, 140),      # Bright Cyan
            "text_primary": (255, 255, 255),
            "text_secondary": (230, 230, 250)
        }
    elif 12 <= hour < 18: # Afternoon
        return {
            "name": "Daylight",
            "gradient_start": (10, 40, 100),    # Brighter Deep Ocean Blue
            "gradient_end": (30, 120, 200),     # Brighter Cyan
            "text_primary": (255, 255, 255),
            "text_secondary": (210, 230, 255)
        }
    elif 18 <= hour < 21: # Sunset
        return {
            "name": "Sunset",
            "gradient_start": (20, 0, 80),  # Deep Violet
            "gradient_end": (255, 127, 80), # Coral
            "text_primary": (255, 255, 255),
            "text_secondary": (255, 220, 220)
        }
    else: # Night
        return {
            "name": "Twilight",
            "gradient_start": (10, 20, 80),   # Deep Blue
            "gradient_end": (50, 10, 100),   # Deeper Violet/Indigo
            "text_primary": (255, 255, 255),
            "text_secondary": (200, 200, 220)
        }

# -- State File --
STATE_IMAGE_PATH = "current_display.png"