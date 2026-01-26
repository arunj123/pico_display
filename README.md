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
* **Smart Configuration:** Reads settings from `/tmp/config.json`, which is populated by the system init script from the persistent `/mnt/data/config.json` on boot.
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
1.  **BLE Method (Recommended):** Use the "Gateway Setup" Web Interface.
    *   Connect to the device via Bluetooth.
    *   The Map will auto-load your current saved location.
    *   Select a new point and click "Save". The device will persist this to `config.json` and reboot.
2.  **Manual Method:**
    *   Connect the device to PC via USB.
    *   Open the "Data Partition" drive.
    *   Edit `config.json`: `{"name":"Paris", "lat":48.85, "lon":2.35}`.
    *   *Note: If `config.json` is missing, the system will auto-create a default one on boot.*

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
3.  Create or edit `wifi.conf` on the **Data Partition** (which appears as a USB Drive when connected to PC):
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
*   **Location & Settings:** 
    *   **BLE Method:** Use the "Gateway Setup" Web Interface (connected via Bluetooth) to set location. This saves to `config.json`.
    *   **Manual Method:** Edit `config.json` on the USB Data Partition:
    ```json
    {
        "name": "Nuremberg",
        "lat": 49.45,
        "lon": 11.08
    }
    ```

## New Features (v2.0)

### 1. Smart USB Storage & Console
The device now intelligently manages the USB connection:
- **PC Connection**: When connected to a computer, it exposes:
    - **USB Mass Storage**: The Data Partition (`/dev/mmcblk0p3`) appears as a USB Drive. You can edit `wifi.conf` or `config.json` directly.
    - **Serial Console**: A shell is available on `ttyGS0` (accessible via `screen /dev/ttyACM0 115200` on host).
    - **Note**: In this mode, local access to the data partition is disabled to prevent corruption.
- **Power-Only / Wall Adapter**: 
    - Mass Storage is disabled (shows "No Media").
    - **Local Provisioning**: The Data Partition is mounted to `/mnt/data`. The system is ready for BLE provisioning.

### 2. Partition Layout
- **Boot (`/boot`)**: Read-only system boot files. Now also stores `seedrng` entropy for reliable random number generation across reboots.
- **RootFS**: Read-only SquashFS.
- **Data (`/mnt/data`)**: Read-Write vFAT partition. Stores:
    - `wifi.conf`: WPA Supplicant configuration.
    - `config.json`: Location and Settings.

### 3. BLE Provisioning (GATT)

The project supports provisioning for both firmware targets, but their capabilities differ:

#### A. Pi Gateway (Linux / Pi Zero 2W)
- **Device Name:** "Gateway Setup"
- **Service UUID:** `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
- **Characteristics:**
    - `0xbeb5...` (Handle `0x0032`): **Read/Write** (SSID & Password)
    - `0x0000...` (Handle `0x0034`): **Read/Write** (Location/Config JSON)
- **Features:** Full read/write support, partial save, write buffering.

#### B. Pico Controller (Microcontroller / Pico W)
- **Device Name:** "Pico Setup"
- **Service UUID:** `0000FF00-0000-1000-8000-00805F9B34FB`
- **Characteristics:**
    - `0xFF01`: **Write-Only** (Wi-Fi Credentials)
- **Note:** The Pico W firmware uses a lightweight stack and *only* supports Wi-Fi credential provisioning. It does not support location configuration or reading back saved data.
