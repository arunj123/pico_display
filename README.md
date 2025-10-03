# Pico W BLE Media Controller & Dynamic TCP Weather Display

This project transforms a Raspberry Pi Pico W into a versatile, multi-function device. It provides two distinct, high-quality firmware targets and a sophisticated Python host application.

1.  **Advanced Media Controller (`media_app`):** The flagship application. It's a BLE Human Interface Device (HID) with a rotary encoder for media control, which also connects to your local Wi-Fi to receive data from a host application. It then renders a beautiful, real-time weather and clock display on a color LCD.
2.  **Classic BLE Keyboard (`keyboard_app`):** A more traditional firmware that turns the Pico W into a simple wireless keyboard.

The project is architected in C++ for performance and clarity, heavily leveraging the Pico's unique Programmable I/O (PIO) and demonstrating a robust cooperative multitasking approach to handle Bluetooth, Wi-Fi, and hardware interrupts simultaneously.

## Features

### 1. Media Controller & Weather Display (`media_app`)

This is a sophisticated, network-connected information display and media controller.

*   **Dual-Functionality:** Operates simultaneously as a BLE media controller and a Wi-Fi-connected display client.
*   **Rotary Encoder Input:**
    *   **Rotation:** Controls volume up/down on the connected host (PC, phone).
    *   **Push-Button:** Toggles mute on/off.
*   **Dynamic Weather & Clock Display:**
    *   Receives image data over a custom TCP protocol from a host Python application.
    *   Renders a beautiful UI with a dynamic gradient background that changes based on the time of day.
    *   Displays the current time, date, weather icon, temperature, weather description, wind speed, humidity, sunrise, and sunset times.
*   **ST7789 Color LCD:**
    *   High-speed, non-blocking driver powered by a PIO state machine to offload the CPU.
*   **Bluetooth LE HID:**
    *   Reports as a standard "Consumer Control" device for broad compatibility.
*   **Robust, Non-Blocking Design:**
    *   The entire application runs in a cooperative, single-threaded main loop that services Wi-Fi, Bluetooth, and application logic without panics or race conditions.
    *   Encoder inputs are captured via high-priority GPIO interrupts that coexist safely with the wireless driver.

### 2. Host Weather Display Application (`display_manager.py`)

A Python application designed to run on a networked computer (like a Raspberry Pi or PC) that serves data to the Pico W.

*   **Live Data Fetching:** Periodically pulls real-time weather data from the Open-Meteo API.
*   **Advanced UI Generation:** Creates a polished, aesthetically pleasing UI image using the Pillow library, complete with colorful, programmatically drawn weather icons.
*   **Efficient "Diff & Tile" Algorithm:**
    *   To minimize network traffic and prevent screen flicker, the script compares the newly generated UI with the last frame sent.
    *   It calculates the smallest rectangular "bounding box" that contains all the changed pixels (a "diff").
    *   If this diff is larger than the 8KB TCP payload limit, it is automatically broken down into smaller "tiles."
    *   Only the necessary tiles are sent to the Pico W. This is extremely efficient for updates like the clock changing, where only a small part of the screen needs to be redrawn.
*   **Reliable TCP Communication:** Uses a custom, framed TCP protocol to send image tiles to the Pico W.

### 3. BLE Keyboard (`keyboard_app`)

A more traditional HID implementation that turns the Pico W into a wireless keyboard.

*   **Standard HID Keyboard:** Emulates a standard keyboard over BLE.
*   **Multiple Input Modes:** Can be controlled via a serial terminal or run in an automatic demo mode.

## Hardware & Wiring

### Required Components

*   Raspberry Pi Pico W
*   Rotary Encoder (EC11-style with push-button)
*   2.0" 240x320 ST7789 TFT LCD Display (e.g., Model: GMT020-02 or similar)
*   Breadboard and jumper wires

### Wiring Schematics

#### Rotary Encoder

| Encoder Pin | Pico Physical Pin | Pico GPIO Pin |
|:------------|:------------------|:--------------|
| `GND`       | `38`              | `GND`         |
| `S1` / `A`  | `14`              | `GPIO10`      |
| `S2` / `B`  | `15`              | `GPIO11`      |
| `KEY`       | `16`              | `GPIO12`      |

#### ST7789 Display

| Display Pin | Pico Physical Pin | Pico GPIO Pin |
|:------------|:------------------|:--------------|
| `VCC`       | `36`              | `3V3(OUT)`    |
| `GND`       | `3`               | `GND`         |
| **SCL**     | `21`              | `GPIO16`      |
| **SDA**     | `22`              | `GPIO17`      |
| **RST**     | `24`              | `GPIO18`      |
| **DC**      | `25`              | `GPIO19`      |
| **CS**      | `26`              | `GPIO20`      |
| `BL`        | `36`              | `3V3(OUT)`    |

## Project Architecture and Design Decisions

### Firmware (C++)

*   **Cooperative Multitasking:** The `media_app` must handle three asynchronous tasks: Bluetooth events, Wi-Fi/TCP events, and user input. The core design decision was to use a single, non-blocking main `while(true)` loop in `MediaApplication::run()`. This loop is paced by the `cyw43_arch_wait_for_work_until()` function, which efficiently yields CPU time, allowing both the BTstack and lwIP networking stacks to process their background tasks without conflict. This avoids the panics and instability that arise from attempting to use blocking calls within different interrupt contexts.

*   **Interrupt Handling:** The Pico W wireless driver takes ownership of the primary GPIO interrupt handler. To allow the `RotaryEncoder` to also respond to high-frequency events, it was designed to register its own **raw interrupt handler** using `gpio_add_raw_irq_handler_with_order_priority_masked`. This allows it to coexist with the wireless driver, with a high enough priority to ensure encoder clicks are not missed during Wi-Fi activity.

*   **Non-Blocking TCP Server:** To prevent panics caused by calling blocking functions from an interrupt context, the `TcpServer` is designed with a **producer-consumer** pattern. The lwIP receive callback (the producer, running in an interrupt context) does the absolute minimum: it verifies the data and pushes it onto a thread-safe queue (`m_tile_queue`). The main application loop (the consumer) then safely pops from this queue and performs the slow, blocking `drawImage` operation.

*   **PIO for Display:** The `St7789Display` driver offloads the high-speed SPI data transmission to a PIO state machine. This is a key performance optimization, as it allows the CPU to perform other tasks while the DMA and PIO work in the background to send pixel data to the screen.

### Host Application (Python)

*   **Modular Design:** The Python code is split into logical modules:
    *   `config.py`: Centralized configuration for IP addresses, pins, fonts, and colors.
    *   `weather.py`: Dedicated to fetching and parsing data from the weather API.
    *   `ui_generator.py`: Handles all graphics, icon drawing, and layout logic.
    *   `display_manager.py`: The main application orchestrator that handles device connection, the diff/tile algorithm, and the main update loop.
*   **Efficient Screen Updates:** The decision to implement a "diff and tile" algorithm is central to the project's performance. Instead of sending a full 150KB framebuffer every second, it sends only a few kilobytes when the time changes, making the application extremely network-efficient.
*   **Robust Communication:** The initial design used a simple echo. This was replaced with a more robust framed protocol. When stability issues arose, an ACK/NACK flow control system was added, and finally simplified by relying on TCP's inherent reliability. The final protocol sends self-contained `IMAGE_TILE` frames, which is both simple and effective.

## Building and Flashing (Firmware)

This project uses the standard Raspberry Pi Pico C/C++ SDK. The recommended environment is VS Code with the official Pico extension.

### Prerequisites

1.  Visual Studio Code.
2.  The official **Raspberry Pi Pico (C/C++ SDK)** extension. This will install the toolchain (compilers, CMake, etc.) for you.

### Build & Flash with VS Code

1.  **Open the project folder** in VS Code.
2.  Select the **CMake** extension from the activity bar.
3.  Choose the desired build target: `media_app` or `keyboard_app`.
4.  Press **`F7`** to build the project.
5.  Put your Pico W into **BOOTSEL mode** (hold the BOOTSEL button while plugging it in).
6.  In VS Code, open the command palette (`Ctrl+Shift+P`) and run the task **"Tasks: Run Task" -> "Flash media_app (openocd)"** (or the keyboard equivalent).

## Running the Host Weather Display

The Python script sends the UI to your Pico W over Wi-Fi.

### Prerequisites

1.  **Python 3.x** installed on your computer.
2.  Install the required libraries:
    ```bash
    pip install requests Pillow
    ```
3.  Download the required fonts (`FreeSansBold.ttf`, `Ubuntu-L.ttf`) and place them in a `fonts` sub-directory.

### Configuration

1.  Open `config.py`.
2.  Set the `PICO_IP` variable to the IP address assigned to your Pico W (you can see this in the serial monitor output when it connects to Wi-Fi).
3.  Set your `LOCATION_LAT` and `LOCATION_LON` for accurate weather.

### Execution

Run the main display manager script from your terminal:
```bash
python display_manager.py
```
The script will connect to your Pico W, fetch the weather, and begin sending screen updates.

## Key Problems Solved

This project overcame several complex integration challenges common in embedded systems:

1.  **Wi-Fi and BLE Coexistence:** Solved initial panics by using the correct lwIP-aware CYW43 driver library (`pico_cyw43_arch_lwip_threadsafe_background`) and a cooperative main loop (`cyw43_arch_wait_for_work_until`).
2.  **GPIO Interrupt Conflicts:** Resolved `Hard assert` panics by converting the `RotaryEncoder` from a simple GPIO callback to a high-priority Raw IRQ Handler, allowing it to coexist with the wireless driver's interrupt requirements.
3.  **Concurrency Panics (`sleep in handler`):** Eliminated panics by architecting a producer-consumer pattern. The TCP receive callback (interrupt context) only pushes data to a queue, while the main application loop is the only place where slow, blocking functions (`drawImage`) are called.
4.  **Memory Leaks (`pbuf_free`):** Fixed a subtle memory leak in the TCP server where empty packets were not being correctly freed, eventually exhausting the lwIP memory pool and causing a crash after long run times.
5.  **Inefficient Display Updates:** Implemented an intelligent "diff and tile" algorithm in the Python host to drastically reduce network traffic, only sending the portions of the screen that have actually changed.

### Building and Flashing with VS Code (Recommended)

The included `.vscode` configuration files (`settings.json`, `tasks.json`, `launch.json`) make building and flashing trivial.

1.  **Select the Target:** Use the **CMake** extension's panel in the activity bar to select the build target. Choose either `media_app` or `keyboard_app`.
2.  **Build:**
    *   Press **`F7`**, or open the command palette (`Ctrl+Shift+P`) and run **"Tasks: Run Build Task"**. This will compile the selected target.
3.  **Flash:**
    *   Put your Pico W into BOOTSEL mode (hold the BOOTSEL button while plugging it in).
    *   Open the command palette (`Ctrl+Shift+P`) and run the appropriate task:
        *   **"Tasks: Run Task" -> "Flash media_app (openocd)"**
        *   **"Tasks: Run Task" -> "Flash keyboard_app (openocd)"**

### Building from the Command Line

1.  **Clone the Repository:**
    ```bash
    git clone https://github.com/Arun-Prakash-S/pico_w_ble_media_controller.git
    cd pico_w_ble_media_controller
    ```

2.  **Configure with CMake:**
    ```bash
    # Set the path to your Pico SDK
    export PICO_SDK_PATH=/path/to/your/pico-sdk
    
    # Create and navigate to the build directory
    mkdir build
    cd build

    # Configure the project
    cmake ..
    ```

3.  **Compile a Specific Target:**
    ```bash
    # To build the Media Controller
    cmake --build . --target media_app

    # To build the Keyboard
    cmake --build . --target keyboard_app
    ```
    This will generate a `.uf2` file inside the `build` directory (e.g., `media_app.uf2`).

4.  **Flash the `.uf2` file:**
    1.  Hold down the **BOOTSEL** button on your Pico W and connect it to your computer.
    2.  It will appear as a USB Mass Storage device named `RPI-RP2`.
    3.  Drag and drop the generated `.uf2` file onto the device. The Pico W will automatically reboot and run the new firmware.

## Acknowledgements

*   The PIO program for the display driver is based on the official examples from the Raspberry Pi Pico SDK.
*   The original C-based keyboard example that inspired the `keyboard_app` is from the [pico-examples](https://github.com/raspberrypi/pico-examples) repository.