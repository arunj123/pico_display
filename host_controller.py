# File: host_controller.py

import socket
import time
import os
import struct
from PIL import Image, ImageDraw

# --- Configuration ---
PICO_IP = "192.168.0.122"  # <-- CHANGE THIS TO YOUR PICO'S IP
PICO_PORT = 4242
DISPLAY_WIDTH = 240
DISPLAY_HEIGHT = 320
TILE_PAYLOAD_SIZE = 8192 # Max 8KB data payload for a TCP frame

# --- Frame Protocol Definitions ---
FRAME_MAGIC = 0xAA
FRAME_TYPE_THROUGHPUT_TEST = 0x01
FRAME_TYPE_IMAGE_TILE = 0x02
FRAME_HEADER_FORMAT = "<BBH" # MAGIC (B), TYPE (B), LENGTH (H)
FRAME_HEADER_SIZE = struct.calcsize(FRAME_HEADER_FORMAT)
IMAGE_TILE_HEADER_FORMAT = "<HHHH" # x, y, width, height (unsigned shorts)
IMAGE_TILE_HEADER_SIZE = struct.calcsize(IMAGE_TILE_HEADER_FORMAT)
MAX_PIXEL_DATA_SIZE = TILE_PAYLOAD_SIZE - IMAGE_TILE_HEADER_SIZE

def create_gradient_image(width, height):
    """Generates a Pillow Image with a diagonal color gradient."""
    img = Image.new('RGB', (width, height))
    draw = ImageDraw.Draw(img)
    for x in range(width):
        for y in range(height):
            r = int(255 * (x / width))
            g = int(255 * (y / height))
            b = int(255 * (1 - (x / width)))
            draw.point((x, y), (r, g, b))
    return img

def convert_image_to_rgb565(img):
    """Converts a Pillow Image to a raw RGB565 byte buffer."""
    img_rgb = img.convert('RGB')
    pixels = bytearray(img.width * img.height * 2)
    i = 0
    for y in range(img.height):
        for x in range(img.width):
            r, g, b = img_rgb.getpixel((x, y))
            # Convert 8-bit RGB to 16-bit RGB565 and pack as little-endian
            rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
            struct.pack_into("<H", pixels, i, rgb565)
            i += 2
    return bytes(pixels)

def tile_image_data(width, height, pixel_data):
    """Generator function that yields image tiles with their metadata."""
    bytes_per_row = width * 2
    # Calculate how many full rows of pixels can fit in one tile payload
    rows_per_tile = MAX_PIXEL_DATA_SIZE // bytes_per_row
    
    y = 0
    while y < height:
        tile_height = min(rows_per_tile, height - y)
        start_index = y * bytes_per_row
        end_index = (y + tile_height) * bytes_per_row
        
        tile_data = pixel_data[start_index:end_index]
        
        # Yield the tile's metadata and its pixel data
        yield (0, y, width, tile_height, tile_data)
        
        y += tile_height

def pack_frame(frame_type, payload):
    """Packs data into the defined frame structure."""
    # --- THE FIX: Use the correct format string for 3 arguments ---
    header = struct.pack(FRAME_HEADER_FORMAT, FRAME_MAGIC, frame_type, len(payload))
    return header + payload

def send_image_to_pico(sock, image):
    """Converts, tiles, and sends an image to the Pico W."""
    print(f"\n--- Sending {image.width}x{image.height} Image ---")
    
    # 1. Convert image to the device's native format
    rgb565_data = convert_image_to_rgb565(image)
    print(f"Image converted to {len(rgb565_data)} bytes of RGB565 data.")
    
    # 2. Tile the image data and send each tile
    tiles = list(tile_image_data(image.width, image.height, rgb565_data))
    num_tiles = len(tiles)
    print(f"Image divided into {num_tiles} tiles.")

    for i, (x, y, w, h, pixel_data) in enumerate(tiles):
        print(f"Sending tile {i+1}/{num_tiles}: (x={x}, y={y}, w={w}, h={h}), size={len(pixel_data)} bytes")
        
        # Create the tile-specific header and the full payload
        tile_header = struct.pack(IMAGE_TILE_HEADER_FORMAT, x, y, w, h)
        payload = tile_header + pixel_data
        
        # Pack the payload into a final frame
        frame = pack_frame(FRAME_TYPE_IMAGE_TILE, payload)
        
        # Send it!
        sock.sendall(frame)
        time.sleep(0.01) # Small delay to allow Pico to process

    print("--- Image Send Complete ---")

def run_throughput_test(sock, num_iterations=5, data_size=8192):
    """Runs the throughput test by sending and echoing data."""
    print(f"\n--- Running Throughput Test ({num_iterations} iterations) ---")
    timings = []
    
    for i in range(num_iterations):
        data_to_send = os.urandom(data_size)
        payload = data_to_send
        frame = pack_frame(FRAME_TYPE_THROUGHPUT_TEST, payload)
        
        start_time = time.perf_counter()
        sock.sendall(frame)
        
        # Wait for the echo
        received_data = bytearray()
        while len(received_data) < len(frame):
            chunk = sock.recv(4096)
            if not chunk:
                break
            received_data.extend(chunk)
        
        end_time = time.perf_counter()
        
        # --- THE FIX: Unpack using the correct format and size ---
        magic, rcv_type, rcv_len = struct.unpack(FRAME_HEADER_FORMAT, received_data[:FRAME_HEADER_SIZE])
        rcv_payload = received_data[FRAME_HEADER_SIZE : FRAME_HEADER_SIZE + rcv_len]

        if magic == FRAME_MAGIC and rcv_payload == data_to_send:
            duration = end_time - start_time
            timings.append(duration)
            throughput = (data_size * 2 / 1024) / duration
            print(f"Iter {i+1}: OK. Round trip took {duration:.4f}s ({throughput:.2f} KB/s)")
        else:
            print(f"Iter {i+1}: ERROR! Data mismatch or bad frame.")
            break
    
    if timings:
        avg_time = sum(timings) / len(timings)
        avg_throughput = (data_size * 2 / 1024) / avg_time
        print(f"Average throughput: {avg_throughput:.2f} KB/s")
    print("--- Throughput Test Complete ---")


def main():
    # 1. Generate gradient image and save a preview
    print("Generating gradient image...")
    gradient_img = create_gradient_image(DISPLAY_WIDTH, DISPLAY_HEIGHT)
    gradient_img.save("gradient_preview.png")
    print("Saved 'gradient_preview.png'")

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            print(f"Connecting to {PICO_IP}:{PICO_PORT}...")
            s.connect((PICO_IP, PICO_PORT))
            print("Connected.")

            # 2. Send the generated image
            send_image_to_pico(s, gradient_img)
            
            # Give the device a moment before the next test
            time.sleep(2)
            
            # 3. Run the classic throughput test
            run_throughput_test(s)

    except ConnectionRefusedError:
        print("Error: Connection refused. Is the Pico W running the server?")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
