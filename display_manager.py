# File: display_manager.py
import socket
import time
import struct
import zlib
from datetime import datetime
from PIL import Image, ImageChops
import os
import config
import weather
import ui_generator

class DeviceManager:
    """Manages low-level TCP communication with the Pico W device."""
    def __init__(self):
        self.sock = None

    def connect(self) -> bool:
        if self.sock: return True
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            print(f"Connecting to {config.PICO_IP}:{config.PICO_PORT}...")
            self.sock.connect((config.PICO_IP, config.PICO_PORT))
            print("Connected.")
            return True
        except (ConnectionRefusedError, OSError) as e:
            print(f"Connection error: {e}")
            self.sock = None
            return False

    def send_image_diff(self, new_image, previous_image):
        if not self.sock: return False, previous_image

        reconstructed_image = None
        if not previous_image:
            print("\n--- Sending Full Initial Image ---")
            bbox = (0, 0, config.LCD_WIDTH, config.LCD_HEIGHT)
            reconstructed_image = Image.new('RGB', (config.LCD_WIDTH, config.LCD_HEIGHT))
        else:
            quantized_new_image = ui_generator.reconstruct_image_from_rgb565(
                ui_generator.convert_image_to_rgb565(new_image), new_image.width, new_image.height
            )
            diff = ImageChops.difference(previous_image, quantized_new_image)
            bbox = diff.getbbox()
            if not bbox: return True, previous_image
            reconstructed_image = previous_image.copy()
        
        print(f"Update Bounding Box: {bbox}")
        
        sub_image = new_image.crop(bbox)
        pixel_data_full = ui_generator.convert_image_to_rgb565(sub_image)
        
        offset_x, offset_y = bbox[0], bbox[1]
        sub_width, sub_height = sub_image.width, sub_image.height
        
        bytes_per_row = sub_width * 2
        rows_per_tile = config.MAX_PIXEL_DATA_SIZE // bytes_per_row if bytes_per_row > 0 else 0
        if rows_per_tile == 0: return False, previous_image

        y = 0
        while y < sub_height:
            tile_height = min(rows_per_tile, sub_height - y)
            
            # --- THE BUG FIX: Correctly slice the binary data for each tile ---
            start_index = y * bytes_per_row
            end_index = (y + tile_height) * bytes_per_row
            tile_pixel_data = pixel_data_full[start_index:end_index]
            
            tile_x_global, tile_y_global = offset_x, offset_y + y
            
            print(f"  - Sending tile: Pos({tile_x_global},{tile_y_global}), Size({sub_width}x{tile_height})")
            
            if not self._send_tile(tile_x_global, tile_y_global, sub_width, tile_height, tile_pixel_data):
                return False, previous_image
            
            # Reconstruct the image from the data that was actually sent
            tile_img = ui_generator.reconstruct_image_from_rgb565(tile_pixel_data, sub_width, tile_height)
            reconstructed_image.paste(tile_img, (tile_x_global, tile_y_global))
                
            y += tile_height
            
        return True, reconstructed_image

    def _send_tile(self, x, y, w, h, pixel_data):
        try:
            # Simplified protocol without CRC
            tile_header = struct.pack(config.IMAGE_TILE_HEADER_FORMAT, x, y, w, h)
            payload = tile_header + pixel_data
            frame = pack_frame(config.FRAME_TYPE_IMAGE_TILE, payload)
            self.sock.sendall(frame)
            time.sleep(0.01) # Small delay for flow control
            return True
        except OSError as e:
            print(f"Socket error during send: {e}")
            self.close()
            return False
            
    # _wait_for_ack removed

    def close(self):
        if self.sock:
            self.sock.close()
            self.sock = None
            print("--- Device Disconnected ---")

def pack_frame(frame_type, payload):
    header = struct.pack(config.FRAME_HEADER_FORMAT, config.FRAME_MAGIC, frame_type, len(payload))
    return header + payload

def main():
    if os.path.exists(config.STATE_IMAGE_PATH):
        try:
            os.remove(config.STATE_IMAGE_PATH)
            print(f"Removed old state file: {config.STATE_IMAGE_PATH}")
        except OSError as e:
            print(f"Error removing old state file: {e}")

    manager = DeviceManager()
    previous_image = None
    previous_time_string = ""
    current_weather = None
    last_weather_check = 0

    while True:
        try:
            if not manager.connect():
                time.sleep(5)
                continue
            previous_image = None

            while True:
                if (time.time() - last_weather_check) > config.WEATHER_UPDATE_INTERVAL_SECONDS:
                    current_weather = weather.get_weather(config.LOCATION_LAT, config.LOCATION_LON)
                    last_weather_check = time.time()
                    previous_image = None
                
                now = datetime.now()
                time_string = now.strftime("%H:%M")
                
                if time_string == previous_time_string and previous_image is not None:
                    time.sleep(1)
                    continue
                
                date_string = now.strftime("%a, %b %d")
                new_image = ui_generator.create_ui_image(time_string, date_string, current_weather)

                # --- THE FIX: Quantize the new image BEFORE comparing it ---
                # This simulates the color information loss and makes the comparison fair.
                # We do this by converting to RGB565 and immediately back to RGB.
                new_image_binary = ui_generator.convert_image_to_rgb565(new_image)
                quantized_new_image = ui_generator.reconstruct_image_from_rgb565(new_image_binary, new_image.width, new_image.height)
                
                # The diff is now calculated against the quantized version
                success, resulting_image = manager.send_image_diff(quantized_new_image, previous_image)
                
                if success:
                    # The new "previous image" is the quantized one we just used for the diff.
                    previous_image = resulting_image 
                    previous_time_string = time_string
                    
                    previous_image.save(config.STATE_IMAGE_PATH)
                    print(f"Successfully updated display. State saved to {config.STATE_IMAGE_PATH}")
                
                time.sleep(1)

        except (ConnectionRefusedError, ConnectionResetError, OSError) as e:
            print(f"\nConnection error: {e}. Reconnecting in 5 seconds...")
            manager.close()
            previous_image = None
            time.sleep(5)
        except KeyboardInterrupt:
            print("\nExiting.")
            break
    
    manager.close()

if __name__ == "__main__":
    main()