# File: config.py
import os
import json
from PIL import ImageFont
import struct
from datetime import datetime
from random import randrange

# -- Device & Network Configuration --
PICO_IP = None  # Removed hardcoded IP. Discovery is now required.
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
FRAME_TYPE_TILE_ACK = 0x03
FRAME_TYPE_TILE_NACK = 0x04
IMAGE_TILE_HEADER_FORMAT = "<HHHHI"
IMAGE_TILE_HEADER_SIZE = struct.calcsize(IMAGE_TILE_HEADER_FORMAT)
MAX_PIXEL_DATA_SIZE = TILE_PAYLOAD_SIZE - IMAGE_TILE_HEADER_SIZE

# -- Location & Weather --
LOCATION_NAME = "Hasenbuck" # Default
LOCATION_LAT = 49.4247
LOCATION_LON = 11.0896
WEATHER_UPDATE_INTERVAL_SECONDS = 15 * 60

# --- Load Dynamic Config ---
# --- Load Dynamic Config ---
# Read from /tmp (Created by S99picodisplay from /mnt/data/config.json)
CONFIG_JSON_PATH = "/tmp/config.json"

if os.path.exists(CONFIG_JSON_PATH):
    try:
        with open(CONFIG_JSON_PATH, 'r') as f:
            data = json.load(f)
            LOCATION_NAME = data.get('name', LOCATION_NAME)
            LOCATION_LAT = data.get('lat', LOCATION_LAT)
            LOCATION_LON = data.get('lon', LOCATION_LON)
            print(f"Loaded config from {CONFIG_JSON_PATH}: {LOCATION_NAME}")
    except Exception as e:
        print(f"Error loading {CONFIG_JSON_PATH}: {e}")

# -- UI Layout and Fonts --
try:
    _CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
    FONT_PATH_BOLD = os.path.join(_CURRENT_DIR, "fonts", "FreeSansBold.ttf")
    FONT_PATH_REGULAR = os.path.join(_CURRENT_DIR, "fonts", "Ubuntu-L.ttf")
    
    FONT_TIME = ImageFont.truetype(FONT_PATH_BOLD, 72)
    FONT_DATE = ImageFont.truetype(FONT_PATH_REGULAR, 24)
    FONT_TEMP = ImageFont.truetype(FONT_PATH_BOLD, 42)
    FONT_WEATHER = ImageFont.truetype(FONT_PATH_REGULAR, 20)
    FONT_INFO_HEADER = ImageFont.truetype(FONT_PATH_REGULAR, 16)
    FONT_INFO_VALUE = ImageFont.truetype(FONT_PATH_BOLD, 18)
    FONT_DATA_AGE = ImageFont.truetype(FONT_PATH_REGULAR, 14)
    # Reuse Weather font for Location
    FONT_LOCATION = ImageFont.truetype(FONT_PATH_REGULAR, 20) 
except IOError:
    print("Warning: Custom fonts not found. Using default.")
    FONT_TIME = ImageFont.load_default()
    FONT_DATE = ImageFont.load_default()
    FONT_TEMP = ImageFont.load_default()
    FONT_WEATHER = ImageFont.load_default()
    FONT_INFO_HEADER = ImageFont.load_default()
    FONT_INFO_VALUE = ImageFont.load_default()
    FONT_DATA_AGE = ImageFont.load_default()
    FONT_LOCATION = ImageFont.load_default()

# -- UI Color Theme --
COLOR_STALE = (200, 200, 200) 

import ui_theme

def get_current_theme():
    return ui_theme.get_current_theme()

# -- State File --
STATE_IMAGE_PATH = "/tmp/current_display.png"