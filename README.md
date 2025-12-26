# Pico W BLE Media Controller & Dynamic Weather Station

This project transforms a Raspberry Pi Pico W into a versatile, multi-function desktop device. It features a robust cooperative multitasking architecture that handles Bluetooth Low Energy (BLE), Wi-Fi networking, and real-time hardware interrupts simultaneously.

## Features

### 1. Media Controller & Weather Display (`media_app`)
The flagship firmware target. It operates as a hybrid IoT device:
*   **BLE Media HID:**
    *   **Rotary Encoder:** Controls host volume (Clockwise/Counter-Clockwise).
    *   **Push Button:** Toggles Mute.
    *   **Battery Service:** Simulates battery level for BLE testing.
*   **Smart TCP Display:**
    *   Connects to Wi-Fi and listens on a TCP port.
    *   Receives optimized "image tiles" from a host Python script.
    *   Renders a high-speed, tear-free UI on an ST7789 IPS LCD.
    *   **Zero-Blocking:** Uses a custom "Diff & Tile" protocol with ACK flow control to prevent network buffer overflows.

### 2. Host Weather Application (`scripts/display_manager.py`)
A Python application running on your PC/Mac/Raspberry Pi:
*   **Live Data:** Fetches real-time weather from the Open-Meteo API.
*   **Dynamic UI:** Generates a beautiful gradient background that changes based on the time of day (Morning/Day/Sunset/Night).
*   **Efficient Transport:** Only sends changed pixels (diffs) to the Pico W to minimize latency.

### 3. BLE Keyboard (`keyboard_app`)
A standalone firmware example:
*   Turns the Pico W into a standard Bluetooth Keyboard.
*   Supports typing via Serial input (stdin) or an automatic demo mode.

---

## Hardware Wiring

### Components
*   Raspberry Pi Pico W
*   ST7789 Display (240x320 SPI)
*   EC11 Rotary Encoder (with Push Button)
*   (Optional) Raspberry Pi Debug Probe for SWD debugging

### Pin Connections

#### Rotary Encoder
| Encoder Pin | Pico GPIO | Physical Pin |
|:------------|:----------|:-------------|
| `A` (CLK)   | `GPIO 10` | 14           |
| `B` (DT)    | `GPIO 11` | 15           |
| `KEY` (SW)  | `GPIO 12` | 16           |
| `GND`       | `GND`     | 38           |

#### ST7789 Display
| Display Pin | Pico GPIO | Physical Pin |
|:------------|:----------|:-------------|
| `SDA` (MOSI)| `GPIO 17` | 22           |
| `SCL` (SCK) | `GPIO 16` | 21           |
| `CS`        | `GPIO 20` | 26           |
| `DC`        | `GPIO 19` | 25           |
| `RST`       | `GPIO 18` | 24           |
| `VCC`       | `3V3(OUT)`| 36           |
| `GND`       | `GND`     | 3            |

---

## Project Configuration

### 1. Wi-Fi Credentials (Firmware)
Before building, you **must** configure your network settings.
1.  Open `config/private_config.h`.
2.  Edit the constants:
    ```cpp
    constexpr const char* const WIFI_SSID = "Your_WiFi_Name";
    constexpr const char* const WIFI_PASSWORD = "Your_WiFi_Password";
    ```

### 2. Host Settings (Python)
After flashing the Pico and getting its IP address (via Serial Monitor):
1.  Open `scripts/config.py`.
2.  Update the target IP:
    ```python
    PICO_IP = "192.168.1.XXX" # Replace with your Pico's IP
    ```
3.  (Optional) Update `LOCATION_LAT` and `LOCATION_LON` for accurate weather.

---

## Building and Flashing

This project is configured for **VS Code** with the **Raspberry Pi Pico Extension**.

### Step 1: Build
1.  Open the project in VS Code.
2.  Select the target (e.g., `media_app`) in the CMake status bar.
3.  Click **Build** (or press `F7`).

### Step 2: Flash

#### Option A: Direct USB (Standard)
*Use this if connecting the Pico W directly to your computer via USB.*
1.  Hold the **BOOTSEL** button on the Pico W while plugging it in.
2.  Run the VS Code Task: **"Run media_app (picotool)"**.
3.  *Alternative:* Drag and drop `build/media_app.uf2` onto the `RPI-RP2` drive.

#### Option B: Debug Probe / OpenOCD (Advanced)
*Use this if using a Raspberry Pi Debug Probe connected to the SWD pins.*

**⚠️ Critical Driver Setup:**
If OpenOCD fails with `unable to find a matching CMSIS-DAP device` or `could not claim interface`, you must update the driver:
1.  Plug in the Debug Probe.
2.  Download and run **Zadig**.
3.  Select **Options > List All Devices**.
4.  Find **"CMSIS-DAP v2 (Interface 0)"** (or similar). **Do not** select Interface 1 (Serial).
5.  Set the driver to **WinUSB** and click **Reinstall Driver**.

**To Flash:**
1.  Run the VS Code Task: **"Flash media_app (openocd)"**.

---

## Running the System

1.  **Start the Pico:**
    *   Reset the Pico.
    *   Open a Serial Monitor (115200 baud).
    *   Wait for the log: `Connected to Wi-Fi. IP: 192.168.X.X`.
    *   The LCD will display: "Waiting for host...".

2.  **Start the Python Host:**
    *   Navigate to the scripts folder: `cd scripts`
    *   Run the manager: `python display_manager.py`
    *   The script will generate the UI and push it to the Pico.

3.  **Connect Bluetooth:**
    *   Scan for Bluetooth devices on your phone/PC.
    *   Connect to **"Pico MediaCtl"**.
    *   Turn the encoder to change volume!

---

## Architecture Design

### Firmware (C++)
*   **Producer-Consumer Pattern:**
    *   **Producer (ISR):** The TCP receive callback runs in an interrupt context. It validates the CRC of incoming "Tiles" and pushes them into a thread-safe `m_tile_queue`.
    *   **Consumer (Main Loop):** The main loop pulls tiles from the queue and draws them. It only sends an **ACK** to the host when it is ready for more data.
*   **Cooperative Multitasking:**
    *   The system uses a single `while(true)` loop.
    *   Blocking operations (like drawing 76,800 pixels) explicitly yield via `cyw43_arch_poll()` every 64 pixels to keep the Wi-Fi and Bluetooth stacks alive.
*   **Interrupt Priority Management:**
    *   The Rotary Encoder uses a **Raw IRQ Handler** with a higher priority (`0x30`) than the Wi-Fi chip (`0x40`) to ensure volume inputs are never missed, even during heavy network traffic.

### Host (Python)
*   **Diff & Tile Algorithm:**
    *   The script keeps a copy of the previous frame.
    *   It calculates the bounding box of changed pixels (the "Diff").
    *   It slices this area into 8KB "Tiles" (payloads).
    *   It waits for an application-level `ACK` from the Pico before sending the next tile, ensuring 100% reliability.

## Troubleshooting

*   **"Entity not found" (OpenOCD):** This means the wrong driver is applied to the Debug Probe. Use Zadig to force **WinUSB** on **Interface 0**.
*   **Connection Refused (Python):** Check that the `PICO_IP` in `config.py` matches the actual IP printed in the Serial Monitor.
*   **Bluetooth Disconnects:** Ensure your power supply is stable. Wi-Fi transmission spikes can cause brownouts on weak USB ports.