# File: ui_theme.py
from datetime import datetime

def get_current_theme():
    """Selects a color theme based on the current hour of the day."""
    hour = datetime.now().hour
    
    if 5 <= hour < 12: # Morning
        return {
            "name": "Morning Sky",
            "gradient_start": (0, 10, 60),
            "gradient_end": (10, 80, 140),
            "text_primary": (255, 255, 255),
            "text_secondary": (230, 230, 250)
        }
    elif 12 <= hour < 18: # Afternoon
        return {
            "name": "Daylight",
            "gradient_start": (10, 40, 100),
            "gradient_end": (30, 120, 200),
            "text_primary": (255, 255, 255),
            "text_secondary": (210, 230, 255)
        }
    elif 18 <= hour < 21: # Sunset
        return {
            "name": "Sunset",
            "gradient_start": (20, 0, 80),
            "gradient_end": (255, 127, 80),
            "text_primary": (255, 255, 255),
            "text_secondary": (255, 220, 220)
        }
    else: # Night
        return {
            "name": "Twilight",
            "gradient_start": (10, 20, 80),
            "gradient_end": (50, 10, 100),
            "text_primary": (255, 255, 255),
            "text_secondary": (200, 200, 220)
        }
