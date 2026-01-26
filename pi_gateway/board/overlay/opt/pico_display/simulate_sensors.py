import os
import time
from PIL import Image
import config
import ui_generator

def simulate():
    print("Starting UI Simulation...")
    
    # Mock sensor data
    mock_data = {
        "Kindr": {"temp": 23.4, "hum": 45, "timestamp": time.time() - 10},
        "Wohn": {"temp": 21.8, "hum": 52, "timestamp": time.time() - 45},
        "Flur": {"temp": 19.5, "hum": 60, "timestamp": time.time() - 320}, # Stale (> 5m)
        "Bad": {"temp": 24.1, "hum": 65, "timestamp": time.time() - 2},
        "Kuche": {"temp": 20.2, "hum": 48, "timestamp": time.time() - 600}, # Very Stale
        # "Schlf" is missing to test the "--" case
    }
    
    print("Generating sensor UI image...")
    img = ui_generator.create_sensor_ui_image(mock_data)
    
    output_path = "test_sensors.png"
    img.save(output_path)
    print(f"Simulation saved to: {os.path.abspath(output_path)}")

if __name__ == "__main__":
    # Ensure we are in the right directory for relative font paths if needed
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    simulate()
