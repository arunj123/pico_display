import socket
import time
import os

# --- Configuration ---
# IMPORTANT: Get this from your Pico's serial monitor output on boot
PICO_IP = "192.168.0.122"  # <--- CHANGE THIS TO YOUR PICO'S IP
#PICO_IP = "192.168.1.123"  # <--- CHANGE THIS TO YOUR PICO'S IP
PICO_PORT = 4242

# Test parameters
DATA_SIZE_BYTES = 8 * 1024
NUM_ITERATIONS = 20

def main():
    timings = []
    total_bytes_transferred = 0

    for i in range(NUM_ITERATIONS):
        print(f"\n--- Iteration {i + 1}/{NUM_ITERATIONS} ---")
        
        try:
            # 1. Connect to the Pico
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                print(f"Connecting to {PICO_IP}:{PICO_PORT}...")
                s.connect((PICO_IP, PICO_PORT))
                print("Connected.")

                # 2. Prepare and send data
                data_to_send = os.urandom(DATA_SIZE_BYTES)
                start_time = time.perf_counter()
                
                print(f"Sending {len(data_to_send)} bytes...")
                s.sendall(data_to_send)
                print("Data sent. Waiting for echo...")

                # 3. Receive the echo back
                received_data = bytearray()
                while len(received_data) < DATA_SIZE_BYTES:
                    chunk = s.recv(4096) # Read in chunks
                    if not chunk:
                        # Connection closed prematurely
                        break
                    received_data.extend(chunk)
                
                end_time = time.perf_counter()
                print(f"Received {len(received_data)} bytes back.")

                # 4. Verify and record
                if data_to_send == received_data:
                    duration = end_time - start_time
                    timings.append(duration)
                    total_bytes_transferred += 2 * DATA_SIZE_BYTES
                    print(f"SUCCESS: Data verified. Round trip took {duration:.4f} seconds.")
                else:
                    print("ERROR: Data mismatch!")
                    break
        
        except ConnectionRefusedError:
            print("Error: Connection refused. Is the Pico W running the server?")
            break
        except Exception as e:
            print(f"An error occurred: {e}")
            break

    # --- Print final results ---
    if timings:
        avg_time = sum(timings) / len(timings)
        total_data_kb = total_bytes_transferred / 1024
        avg_throughput_kbs = (2 * DATA_SIZE_BYTES / 1024) / avg_time
        
        print("\n--- Test Complete ---")
        print(f"Iterations:          {len(timings)}")
        print(f"Total data transfer: {total_data_kb:.2f} KB")
        print(f"Average round-trip:  {avg_time:.4f} seconds")
        print(f"Average throughput:  {avg_throughput_kbs:.2f} KB/s")

if __name__ == "__main__":
    main()
