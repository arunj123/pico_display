# Pico W BLE Media Controller & Dynamic Weather Station

This project transforms a Raspberry Pi Pico W into a versatile, multi-function desktop device. It features a robust cooperative multitasking architecture that handles Bluetooth Low Energy (BLE), Wi-Fi networking, and real-time hardware interrupts simultaneously.

## Features

### 1. Media Controller & Weather Display (`media_app`)
The flagship firmware target. It operates as a hybrid IoT device:
* **BLE Media HID:**
    * **Rotary Encoder:** Controls host volume (Clockwise/Counter-Clockwise).
    * **Push Button:** Toggles Mute.
    * **Long Press (3s):** Reboots device into **Setup Mode**.
    * **Battery Service:** Simulates battery level for BLE testing.
* **Smart TCP Display:**
    * Connects to Wi-Fi and listens on a TCP port.
    * Receives optimized "image tiles" from a host Python script.
    * Renders a high-speed, tear-free UI on an ST7789 IPS LCD.
    * **Zero-Blocking:** Uses a custom "Diff & Tile" protocol with ACK flow control to prevent network buffer overflows.

### 2. Web-Based Provisioning (Setup Mode)
No need to recompile to change Wi-Fi networks!
* **QR Code Pairing:** Generates a QR code on the LCD pointing to a hosted Web App.
* **Web Bluetooth:** Uses the browser's Bluetooth API to connect securely to the Pico.
* **Flash Persistence:** Saves SSID and Password to the Pico's onboard Flash memory.

### 3. Host Weather Application (`pi_gateway/board/overlay/opt/pico_display/display_manager.py`)
A Python application running on your PC/Mac/Raspberry Pi:
* **Live Data:** Fetches real-time weather from the Open-Meteo API.
* **Dynamic UI:** Generates a beautiful gradient background that changes based on the time of day (Morning/Day/Sunset/Night).
* **Efficient Transport:** Only sends changed pixels (diffs) to the Pico W to minimize latency.

---

## Hardware Wiring

### Components
* Raspberry Pi Pico W
* ST7789 Display (240x320 SPI)
* EC11 Rotary Encoder (with Push Button)
* (Optional) Raspberry Pi Debug Probe for SWD debugging

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

## Setup & Configuration

### 1. Wi-Fi Configuration (Provisioning)
You do **not** need to edit header files to set up Wi-Fi.
1.  Flash the `media_app` firmware.
2.  On boot, if no credentials are found, or if you **Long Press (3s)** the encoder button, the device enters **Setup Mode**.
3.  The LCD will display a QR Code.
4.  Scan the QR Code (or visit the URL) on a device with Bluetooth (Phone/Laptop).
5.  Follow the web prompts to connect to "Pico Setup" and send your Wi-Fi credentials.
6.  The device will automatically save and reboot into Normal Mode.

### 2. Host Settings (Python)
The host application uses **UDP Auto-Discovery** to find the Pico W.
1.  Navigate to `pi_gateway/board/overlay/opt/pico_display/`.
2.  Edit `config.py` to set your Location (Latitude/Longitude).
3.  (Optional) You can manually set `PICO_IP` if discovery fails.

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

## Architecture Design

### Firmware (C++)
* **Dual-Profile Bluetooth:**
    * **Normal Mode:** Exposes HID Service (Keyboard/Consumer Control).
    * **Setup Mode:** Exposes a custom GATT Service for Wi-Fi provisioning.
    * Switching relies on a "Magic Byte" in the Watchdog Scratch register to persist state across soft reboots.
* **Producer-Consumer Pattern:**
    * **Producer (ISR):** The TCP receive callback runs in an interrupt context. It validates the CRC of incoming "Tiles" and pushes them into a thread-safe `m_tile_queue`.
    * **Consumer (Main Loop):** The main loop pulls tiles from the queue and draws them. It only sends an **ACK** to the host when it is ready for more data.
* **Cooperative Multitasking:**
    * The system uses a single `while(true)` loop.
    * Blocking operations (like drawing 76,800 pixels) explicitly yield via `cyw43_arch_poll()` every 64 pixels to keep the Wi-Fi and Bluetooth stacks alive.

### Host (Python)
* **Diff & Tile Algorithm:**
    * The script keeps a copy of the previous frame.
    * It calculates the bounding box of changed pixels (the "Diff").
    * It slices this area into 8KB "Tiles" (payloads).
    * It waits for an application-level `ACK` from the Pico before sending the next tile, ensuring 100% reliability.

## Hosting python script using Buildroot cooked IOT image

### 1. Build the Image
The project includes a helper script to automate the Buildroot setup and compilation:

```bash
./scripts/build_pi_image.sh
```

This script will:
1.  Clone the required Buildroot branch (if missing).
2.  Configure it using the custom `raspberrypizero2w_64` defconfig.
3.  Build the system image.

**Output:** `output/images/sdcard.img`

### 2. Flash & Wi-Fi Setup
1.  Flash `output/images/sdcard.img` to an SD card (using Raspberry Pi Imager, Etcher, or `dd`).
2.  To configure Wi-Fi, mount the SD card (or connect the Pi Zero 2W via USB if configured as a gadget).
3.  Create or edit `wifi.conf` on the boot partition:
    ```text
    country=DE
    update_config=1

    network={
        ssid="your_ssid"
        psk="your_password"
        key_mgmt=WPA-PSK
    }
    ```

### 3. Application Configuration (Autodiscovery)
The Python host application (`display_manager.py`) is installed to `/opt/pico_display/`.

*   **Autodiscovery:** The application automatically finds the Pico W via UDP broadcast (`PICO_DISCOVER`). You do **not** need to set the IP manually in `config.py` unless discovery fails.
*   **Location & Settings:** To change weather location or manual IP, edit `/opt/pico_display/config.py` on the device (or `pi_gateway/board/overlay/opt/pico_display/config.py` before building).

    ```python
    # pi_gateway/board/overlay/opt/pico_display/config.py
    
    # Network
    PICO_IP = None  # None = Auto-discover
    
    # Location
    LOCATION_NAME = "Nuremberg"
    LOCATION_LAT = 49.45
    LOCATION_LON = 11.08
    ```
