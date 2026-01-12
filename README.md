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

### 3. Host Weather Application (`scripts/display_manager.py`)
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
After the Pico connects to Wi-Fi, it will display its IP address.
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

make -C buildroot O=$(pwd)/output menuconfig
make -C buildroot O=$(pwd)/output savedefconfig

./scripts/build_pi_image.sh

make -C buildroot O=$(pwd)/output busybox-menuconfig
cp ./output/build/busybox-*/.config ./pi_gateway/board/busybox.config

To check content of data partition
./output/host/bin/mdir -i output/images/data.vfat ::/

To check content of root partition
unsquashfs -ll output/images/rootfs.squashfs | grep "seedrng"
unsquashfs output/images/rootfs.squashfs etc/inittab


make -C buildroot O=$(pwd)/output linux-menuconfig
make -C buildroot O=$(pwd)/output linux-savedefconfig
mv output/build/linux-custom/defconfig pi_gateway/board/linux_kconfig
make -C buildroot O=$(pwd)/output linux-dirclean
make -C buildroot O=$(pwd)/output linux-reinstall
rpi-wifi-firmware-rebuild
linux-firmware-rebuild
make -C buildroot O=$(pwd)/output rpi-firmware-rebuild

To configure Wifi, connect Raspberry Pi Zero 2W device to PC, a device will appear. Create/updat wifi.conf.
'''
country=DE
update_config=1

network={
    ssid="DEFAULT_WIFI_SSID"
    psk="DEFAULT_WIFI_PASSWORD"
    key_mgmt=WPA-PSK
}
'''
