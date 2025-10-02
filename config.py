# File: config.py
import os
from PIL import ImageFont
import struct
from datetime import datetime

# -- Device & Network Configuration --
PICO_IP = "192.168.0.122"
PICO_PORT = 4242

# -- Display & Protocol Configuration --
LCD_WIDTH = 320
LCD_HEIGHT = 240
TILE_PAYLOAD_SIZE = 8192

# --- Simplified Protocol Definitions ---
FRAME_MAGIC = 0xAA
FRAME_HEADER_FORMAT = "<BBH"
FRAME_HEADER_SIZE = struct.calcsize(FRAME_HEADER_FORMAT)
FRAME_TYPE_IMAGE_TILE = 0x02
IMAGE_TILE_HEADER_FORMAT = "<HHHH"  # x, y, width, height
IMAGE_TILE_HEADER_SIZE = struct.calcsize(IMAGE_TILE_HEADER_FORMAT)
MAX_PIXEL_DATA_SIZE = TILE_PAYLOAD_SIZE - IMAGE_TILE_HEADER_SIZE

# -- Location & Weather --
LOCATION_LAT = 49.4247
LOCATION_LON = 11.0896
WEATHER_UPDATE_INTERVAL_SECONDS = 15 * 60

# -- UI Layout and Fonts --
try:
    _CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
    FONT_PATH_BOLD = os.path.join(_CURRENT_DIR, "fonts", "FreeSansBold.ttf")
    FONT_PATH_REGULAR = os.path.join(_CURRENT_DIR, "fonts", "Ubuntu-L.ttf")
    
    FONT_TIME = ImageFont.truetype(FONT_PATH_BOLD, 48)
    FONT_DATE = ImageFont.truetype(FONT_PATH_REGULAR, 22)
    FONT_TEMP = ImageFont.truetype(FONT_PATH_BOLD, 36)
    FONT_WEATHER = ImageFont.truetype(FONT_PATH_REGULAR, 20)
    FONT_INFO_HEADER = ImageFont.truetype(FONT_PATH_REGULAR, 16)
    FONT_INFO_VALUE = ImageFont.truetype(FONT_PATH_REGULAR, 16)
except IOError:
    raise IOError("Error: Font files not found.")

# -- UI Color Theme --
def get_current_theme():
    """Selects a color theme based on the current hour of the day."""
    hour = datetime.now().hour
    if 5 <= hour < 12: # Morning
        return {
            "name": "Morning Sky",
            "gradient_start": (70, 130, 180), # Light Steel Blue
            "gradient_end": (135, 206, 235), # Sky Blue
            "text_primary": (255, 255, 255),
            "text_secondary": (230, 230, 250)
        }
    elif 12 <= hour < 18: # Afternoon
        return {
            "name": "Daylight",
            "gradient_start": (0, 119, 190),  # Deep Sky Blue
            "gradient_end": (0, 191, 255),    # Brighter Deep Sky Blue
            "text_primary": (255, 255, 255),
            "text_secondary": (210, 230, 255)
        }
    elif 18 <= hour < 21: # Sunset
        return {
            "name": "Sunset",
            "gradient_start": (255, 127, 80), # Coral
            "gradient_end": (20, 0, 80),      # Deep Violet
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