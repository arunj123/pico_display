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
    """Manages robust, flow-controlled TCP communication with the Pico W device."""
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
        """
        Calculates the difference, sends it in tiles with ACK/CRC flow control,
        and returns the reconstructed image upon success.
        """
        if not self.sock: return False, previous_image

        reconstructed_image = None
        if not previous_image:
            print("\n--- Sending Full Initial Image ---")
            bbox = (0, 0, config.LCD_WIDTH, config.LCD_HEIGHT)
            reconstructed_image = Image.new('RGB', (config.LCD_WIDTH, config.LCD_HEIGHT))
        else:
            # First, quantize the new image to match the color precision of the old one
            quantized_new_image = ui_generator.reconstruct_image_from_rgb565(
                ui_generator.convert_image_to_rgb565(new_image), new_image.width, new_image.height
            )
            diff = ImageChops.difference(previous_image, quantized_new_image)
            bbox = diff.getbbox()
            if not bbox: return True, previous_image # No changes
            reconstructed_image = previous_image.copy()
        
        print(f"Update Bounding Box: {bbox}")
        
        sub_image = new_image.crop(bbox)
        pixel_data_full = ui_generator.convert_image_to_rgb565(sub_image)
        
        offset_x, offset_y = bbox[0], bbox[1]
        sub_width, sub_height = sub_image.width, sub_image.height
        
        bytes_per_row = sub_width * 2
        rows_per_tile = config.MAX_PIXEL_DATA_SIZE // bytes_per_row if bytes_per_row > 0 else 0
        if rows_per_tile == 0:
            print("Error: Tile is too narrow.")
            return False, previous_image

        y = 0
        tile_count = 0
        while y < sub_height:
            tile_height = min(rows_per_tile, sub_height - y)
            
            # Correctly slice the binary data for the current tile
            start_index = y * bytes_per_row
            end_index = (y + tile_height) * bytes_per_row
            tile_pixel_data = pixel_data_full[start_index:end_index]
            
            # Pad the data for CRC calculation
            padding = (4 - (len(tile_pixel_data) % 4)) % 4
            padded_pixel_data = tile_pixel_data + (b'\x00' * padding)
            
            # Calculate CRC on the padded data
            crc = zlib.crc32(padded_pixel_data)
            
            tile_x_global, tile_y_global = offset_x, offset_y + y

            # --- Retry loop for network resilience ---
            MAX_RETRIES = 3
            ack_received = False
            for attempt in range(MAX_RETRIES):
                print(f"  - Sending tile {tile_count+1}: Pos({tile_x_global},{tile_y_global}), Size({sub_width}x{tile_height}), CRC(0x{crc:08X}) (Attempt {attempt+1})")
                
                # Action 1: Send the tile
                if not self._send_tile(tile_x_global, tile_y_global, sub_width, tile_height, crc, padded_pixel_data):
                    # If sendall() fails, the connection is truly dead.
                    return False, previous_image

                # Action 2: Wait for the ACK
                if self._wait_for_ack():
                    ack_received = True
                    break # Success! Exit the retry loop.
                else:
                    print(f"  - WARNING: Timed out waiting for ACK for tile {tile_count+1}. Retrying...")
            
            if not ack_received:
                print(f"  - FATAL: Failed to get ACK for tile {tile_count+1} after {MAX_RETRIES} attempts.")
                return False, previous_image

            # Reconstruct the image locally upon success
            tile_img = ui_generator.reconstruct_image_from_rgb565(tile_pixel_data, sub_width, tile_height)
            reconstructed_image.paste(tile_img, (tile_x_global, tile_y_global))
                
            y += tile_height
            tile_count += 1
            
        return True, reconstructed_image

    def _send_tile(self, x, y, w, h, crc, pixel_data):
        """Packs and sends a single image tile frame without waiting for an ACK."""
        try:
            tile_header = struct.pack(config.IMAGE_TILE_HEADER_FORMAT, x, y, w, h, crc)
            payload = tile_header + pixel_data
            frame = pack_frame(config.FRAME_TYPE_IMAGE_TILE, payload)
            self.sock.sendall(frame)
            return True
        except OSError as e:
            print(f"Socket error during send: {e}")
            self.close()
            return False
            
    def _wait_for_ack(self):
        try:
            self.sock.settimeout(15.0)
            data = self.sock.recv(config.FRAME_HEADER_SIZE)
            if not data or len(data) < config.FRAME_HEADER_SIZE:
                print("ERROR: Did not receive a valid response from Pico.")
                return False
            magic, rcv_type, _ = struct.unpack(config.FRAME_HEADER_FORMAT, data)
            if magic == config.FRAME_MAGIC and rcv_type == config.FRAME_TYPE_TILE_ACK:
                return True
            else:
                print(f"ERROR: Received NACK or invalid response (type: {rcv_type}).")
                return False
        except socket.timeout:
            print("ERROR: Timed out waiting for ACK from Pico.")
            return False
        finally:
            self.sock.settimeout(None)

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

                new_image_binary = ui_generator.convert_image_to_rgb565(new_image)
                quantized_new_image = ui_generator.reconstruct_image_from_rgb565(new_image_binary, new_image.width, new_image.height)
                
                success, resulting_image = manager.send_image_diff(quantized_new_image, previous_image)
                
                if success:
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