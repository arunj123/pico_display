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
    """Manages robust, fire-and-forget TCP communication with the Pico W device."""
    def __init__(self):
        self.sock = None
        self.device_ip = config.PICO_IP # Start with config, but override via discovery
        self.device_port = config.PICO_PORT

    def find_device(self, timeout=3.0) -> bool:
        """Broadcasts UDP packet to find the Pico W automatically."""
        print(f"Scanning for Pico W on local network (UDP 4243)...")
        udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        udp_sock.settimeout(timeout)
        
        try:
            message = b"PICO_DISCOVER"
            # Broadcast to all
            udp_sock.sendto(message, ('<broadcast>', 4243))
            
            while True:
                data, addr = udp_sock.recvfrom(1024)
                if data == b"PICO_HERE":
                    print(f"Found Device at {addr[0]}!")
                    self.device_ip = addr[0]
                    return True
        except socket.timeout:
            print("Discovery timed out. No device found.")
        except Exception as e:
            print(f"Discovery error: {e}")
        finally:
            udp_sock.close()
        
        return False

    def connect(self) -> bool:
        if self.sock: return True
        
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(5.0) 
            
            print(f"Connecting to {self.device_ip}:{self.device_port}...")
            self.sock.connect((self.device_ip, self.device_port))
            
            self.sock.settimeout(15.0) 
            print("Connected.")
            return True
        except (ConnectionRefusedError, OSError, socket.timeout) as e:
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
        if rows_per_tile == 0:
            print("Error: Tile is too narrow.")
            return False, previous_image

        y = 0
        tile_count = 0
        while y < sub_height:
            tile_height = min(rows_per_tile, sub_height - y)
            
            start_index = y * bytes_per_row
            end_index = (y + tile_height) * bytes_per_row
            tile_pixel_data = pixel_data_full[start_index:end_index]
            
            crc = zlib.crc32(tile_pixel_data)
            
            tile_x_global, tile_y_global = offset_x, offset_y + y
            print(f"  - Sending tile {tile_count+1}: Pos({tile_x_global},{tile_y_global}), Size({sub_width}x{tile_height}), CRC(0x{crc:08X})")
            
            tile_header = struct.pack(config.IMAGE_TILE_HEADER_FORMAT, tile_x_global, tile_y_global, sub_width, tile_height, crc)
            payload = tile_header + tile_pixel_data
            
            if not self._send_frame_and_wait_for_ack(config.FRAME_TYPE_IMAGE_TILE, payload):
                print("  - FAILED to get ACK. Aborting transfer.")
                return False, previous_image
            
            tile_img = ui_generator.reconstruct_image_from_rgb565(tile_pixel_data, sub_width, tile_height)
            reconstructed_image.paste(tile_img, (tile_x_global, tile_y_global))
                
            y += tile_height
            tile_count += 1
            
        return True, reconstructed_image

    def _send_frame_and_wait_for_ack(self, frame_type, payload):
        try:
            frame = pack_frame(frame_type, payload)
            self.sock.sendall(frame)
            response = self.sock.recv(config.FRAME_HEADER_SIZE)
            if len(response) < config.FRAME_HEADER_SIZE:
                print("  - Error: Incomplete ACK response from device.")
                return False
            
            magic, rcv_type, _ = struct.unpack(config.FRAME_HEADER_FORMAT, response)
            
            if magic != config.FRAME_MAGIC:
                print("  - Error: Bad magic byte in ACK response.")
                return False
            
            if rcv_type == config.FRAME_TYPE_TILE_ACK:
                return True
            elif rcv_type == config.FRAME_TYPE_TILE_NACK:
                print("  - Error: Received NACK from device (checksum mismatch).")
                return False
            else:
                print(f"  - Error: Received unexpected frame type {rcv_type} in response.")
                return False

        except socket.timeout:
            print("  - Error: Timed out waiting for ACK from device.")
            self.close()
            return False
        except OSError as e:
            print(f"Socket error during send/receive: {e}")
            self.close()
            return False

    def close(self):
        if self.sock:
            self.sock.close()
            self.sock = None
            print("--- Device Disconnected ---")

def pack_frame(frame_type, payload):
    header = struct.pack(config.FRAME_HEADER_FORMAT, config.FRAME_MAGIC, frame_type, len(payload))
    return header + payload

def format_time_diff(diff_seconds):
    """Formats seconds into a short readable string (5m, 1h, etc)."""
    if diff_seconds < 60:
        return "< 1m"
    elif diff_seconds < 3600:
        minutes = int(diff_seconds / 60)
        return f"{minutes}m"
    else:
        hours = int(diff_seconds / 3600)
        return f"{hours}h"

def main():
    if os.path.exists(config.STATE_IMAGE_PATH):
        try: os.remove(config.STATE_IMAGE_PATH)
        except OSError as e: print(f"Error removing old state file: {e}")

    manager = DeviceManager()
    previous_image = None
    previous_time_string = ""
    
    # Persistent weather state
    current_weather = None
    last_weather_check = 0
    # Force initial update
    force_update = True 

    # --- DISCOVERY PHASE ---
    # Attempt to find device. If not found, use default from config.
    if not manager.find_device():
        print(f"Using default IP from config: {manager.device_ip}")

    while True:
        try:
            if not manager.connect():
                # On connection failure, try discovery again before retrying connect
                print("Retrying discovery...")
                manager.find_device()
                time.sleep(5)
                continue
            
            previous_image = None

            while True:
                # --- Weather Update Logic ---
                # Check weather if interval passed OR if we never had weather data (force update)
                time_since_last = time.time() - last_weather_check
                
                if force_update or time_since_last > config.WEATHER_UPDATE_INTERVAL_SECONDS:
                    print("Fetching weather data...")
                    # Set force_update to False immediately to prevent rapid loops on failure
                    force_update = False 
                    
                    new_weather = weather.get_weather(config.LOCATION_LAT, config.LOCATION_LON)
                    
                    if new_weather:
                        current_weather = new_weather
                        last_weather_check = time.time()
                    else:
                        print("Weather fetch failed. Keeping previous data (if any). Retrying in 1 minute.")
                        # Schedule retry in 60 seconds by manipulating last_weather_check
                        # We want (now - last) to be (INTERVAL - 60), so we wait 60s to reach INTERVAL
                        last_weather_check = time.time() - config.WEATHER_UPDATE_INTERVAL_SECONDS + 60
                
                # --- Stale Data Calculation ---
                is_stale = False
                stale_age_str = ""
                
                if current_weather:
                    # Calculate how old the data actually is based on its internal timestamp
                    data_age = time.time() - current_weather.get('timestamp', time.time())
                    
                    # Consider data stale if it's older than the interval + small buffer
                    if data_age > (config.WEATHER_UPDATE_INTERVAL_SECONDS + 60):
                        is_stale = True
                        stale_age_str = format_time_diff(data_age)

                # --- UI Rendering ---
                now = datetime.now()
                time_string = now.strftime("%H:%M")
                
                if time_string == previous_time_string and previous_image is not None:
                    # If time hasn't changed, we usually sleep. 
                    # BUT if weather just went stale or updated, we need to redraw.
                    # For simplicity, we redraw if weather changed or time changed.
                    # Checking deeper logic is complex, so strict 1-min sleep is fine for clock
                    # but might miss immediate weather updates.
                    # Current logic: Redraw on time change.
                    time.sleep(1)
                    continue
                
                date_string = now.strftime("%a, %b %d")
                
                new_image = ui_generator.create_ui_image(
                    time_string, 
                    date_string, 
                    current_weather,
                    is_stale=is_stale,
                    stale_age_str=stale_age_str
                )

                new_image_binary = ui_generator.convert_image_to_rgb565(new_image)
                quantized_new_image = ui_generator.reconstruct_image_from_rgb565(new_image_binary, new_image.width, new_image.height)
                
                success, resulting_image = manager.send_image_diff(quantized_new_image, previous_image)
                
                if success:
                    previous_image = resulting_image
                    previous_time_string = time_string
                    if previous_image:
                        previous_image.save(config.STATE_IMAGE_PATH)
                        print(f"Successfully updated display. State saved to {config.STATE_IMAGE_PATH}")
                else:
                    break
                
                time.sleep(1)

        except (ConnectionResetError, BrokenPipeError, OSError) as e:
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