import sys
import os

# Add the overlay path to sys.path so we can import ui_generator and config
sys.path.append(os.path.abspath("pi_gateway/board/overlay/opt/pico_display"))

import ui_generator
import config

# Mock some data for the preview
weather = {
    "icon": "moon", 
    "temperature": 27, 
    "description": "Clear Sky",
    "windspeed": 9, 
    "humidity": 87, 
    "sunrise": "06:44", 
    "sunset": "18:27"
}

# Generate the image
try:
    img = ui_generator.create_ui_image("21:10", "Mon, Jan 26 2026", weather)
    img.save("local_ui_preview.png")
    print("SUCCESS: local_ui_preview.png generated.")
except Exception as e:
    print(f"ERROR: {e}")
