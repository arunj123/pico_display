# Pico W BLE Media Controller & Keyboard

This project transforms a Raspberry Pi Pico W into a versatile, standalone Bluetooth Low Energy (BLE) Human Interface Device (HID). It provides two distinct, high-quality firmware targets: an advanced Media Controller with a rotary encoder and an LCD display, and a classic BLE Keyboard.

The project is architected in C++ for robustness and clarity, leveraging the Raspberry Pi Pico's unique Programmable I/O (PIO) for high-performance peripheral control and standard GPIO interrupts for efficient user input.

## Features

### 1. Media Controller (`media_app`)

This is the flagship application of the project. It turns the Pico W into a sophisticated wireless media knob.

*   **Rotary Encoder Input:**
    *   **Rotation:** Controls volume up and down on the connected host device (PC, phone, tablet).
    *   **Push-Button:** Toggles mute on/off.
*   **ST7789 Color LCD Display:**
    *   Provides a rich graphical user interface.
    *   Displays status, icons, text, and graphical elements.
    *   High-speed, non-blocking driver powered by a PIO state machine.
*   **Bluetooth LE Connectivity:**
    *   Reports as a standard "Consumer Control" HID device for broad compatibility.
    *   Includes a Battery Service to report power levels to the host.
*   **Robust and Efficient Design:**
    *   The rotary encoder is read using a highly reliable, low-overhead interrupt-driven method.
    *   Button presses are debounced with a state machine in the main application loop to prevent glitches and double-clicks.
    *   The entire application is event-driven and sleeps when idle to conserve power.

### 2. BLE Keyboard (`keyboard_app`)

A more traditional HID implementation that turns the Pico W into a wireless keyboard.

*   **Standard HID Keyboard:** Emulates a standard USB keyboard over BLE.
*   **Multiple Input Modes:**
    *   **Stdin Mode:** (If `HAVE_BTSTACK_STDIN` is defined) Type characters into a serial terminal (like PuTTY or Minicom) and have them sent as keystrokes.
    *   **Demo Mode:** (Default) Automatically types a pre-defined string of text, demonstrating the keyboard functionality without any external input.
*   **US Keyboard Layout:** Includes a full implementation for a standard US keyboard layout, mapping ASCII characters to the correct HID keycodes and modifiers.

## Hardware & Wiring

### Required Components

*   Raspberry Pi Pico W
*   Rotary Encoder (EC11-style with push-button)
*   2.0" 240x320 ST7789 TFT LCD Display (Model: GMT020-02 or similar)
*   Breadboard and jumper wires

### Wiring Schematics

Carefully wire the components to the Pico W according to the tables below.

#### Rotary Encoder Wiring

| Encoder Pin | Pico Physical Pin | Pico GPIO Pin | Description        |
|:------------|:------------------|:--------------|:-------------------|
| `GND`       | `38`              | `GND`         | Ground             |
| `S1` / `A`  | `14`              | `GPIO10`      | Encoder Signal A   |
| `S2` / `B`  | `15`              | `GPIO11`      | Encoder Signal B   |
| `KEY`       | `16`              | `GPIO12`      | Push-Button Switch |
| `+`         | -                 | *Unconnected* | VCC (Not needed with internal pull-ups) |

#### ST7789 Display Wiring (GMT020-02)

| Display Pin | Function      | Pico Physical Pin | Pico GPIO Pin |
|:------------|:--------------|:------------------|:--------------|
| `VCC`       | Power (3.3V)  | `36`              | `3V3(OUT)`    |
| `GND`       | Ground        | `3`               | `GND`         |
| **SCL**     | Serial Clock  | `21`              | `GPIO16`      |
| **SDA**     | Serial Data   | `22`              | `GPIO17`      |
| **RST**     | Reset         | `24`              | `GPIO18`      |
| **DC** / **RS** | Data/Command  | `25`              | `GPIO19`      |
| **CS**      | Chip Select   | `26`              | `GPIO20`      |
| `BL` / `LED`| Backlight     | `36`              | `3V3(OUT)`    |

## Project Architecture and Design

The project is written in C++ and heavily leverages object-oriented principles to create modular, reusable, and easy-to-understand components.

### Core Classes

*   **`MediaApplication`**: The central orchestrator for the `media_app`. It initializes all hardware and software components, contains the main polling loop, and implements a robust state machine for debouncing the encoder's push-button.

*   **`RotaryEncoder`**: A self-contained class that manages the rotary encoder. It uses GPIO interrupts on the encoder's pins to detect events reliably without consuming CPU cycles in the main loop. The ISRs are extremely lightweight, simply setting flags that are then processed safely by the `MediaApplication`.

*   **`St7789Display`**: A low-level driver for the ST7789 display. It uses a **PIO state machine** (`st7789_lcd.pio`) to handle the high-speed SPI data transmission, offloading the CPU and preventing the application from blocking during screen updates.

*   **`Drawing`**: A high-level graphics class that sits on top of the `Display` driver. It provides a simple API for drawing primitives (pixels, lines, rectangles), text (using custom bitmap fonts), and images.

*   **`HidDevice`**: A generic base class that encapsulates the common logic for creating a BLE HID device using the BTstack library. It manages connection state and packet handling.

*   **`MediaControllerDevice`**: Inherits from `HidDevice` and specializes it for a consumer media controller. It provides the specific HID descriptor and methods like `increaseVolume()`, `mute()`, etc., that send the correct HID reports.

*   **`BtStackManager`**: A singleton class that provides a clean C++ wrapper around the C-based BTstack library, forwarding events to the appropriate handler and simplifying integration.

### Asset Pipeline

The project includes Python scripts to automatically convert assets into C++ header files, streamlining the development workflow.

*   **`image_converter.py`**: Converts standard image files (PNG, JPG, etc.) into a C header file containing the image data as a 16-bit RGB565 array. It automatically resizes the image to fit the display while maintaining its aspect ratio.
*   **`create_font.py`**: Converts any TrueType Font (`.ttf`) file into a custom bitmap font, also stored in a C header, which can then be used by the `Drawing` class.

## Building and Flashing

This project is configured to be built with the standard Raspberry Pi Pico C/C++ SDK toolchain using CMake and Ninja. The `.vscode` directory is pre-configured for a seamless development experience in Visual Studio Code.

### Prerequisites

1.  **VS Code with Extensions:**
    *   Visual Studio Code
    *   The official **Raspberry Pi Pico** extension pack. This will install all necessary tools (compilers, CMake, Ninja, OpenOCD, etc.) and configure your environment automatically.

2.  **Pico SDK:** The first time you open the project in VS Code, the extension will prompt you to install the SDK. Follow the instructions to let it download and configure everything for you.

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

*   The PIO program (`st7789_lcd.pio`) for the display driver is based on the official examples from the Raspberry Pi Pico SDK.
*   The original C-based keyboard example that inspired the `keyboard_app` is from the [pico-examples](https://github.com/raspberrypi/pico-examples) repository.
