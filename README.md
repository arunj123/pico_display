# Pico W BLE Media Controller & Dynamic TCP Weather Display

This project transforms a Raspberry Pi Pico W into a versatile, multi-function device. It provides two distinct, high-quality firmware targets and a sophisticated Python host application.

1.  **Advanced Media Controller (`media_app`):** The flagship application. It's a BLE Human Interface Device (HID) with a rotary encoder for media control, which also connects to your local Wi-Fi to receive data from a host application. It then renders a beautiful, real-time weather and clock display on a color LCD.
2.  **Classic BLE Keyboard (`keyboard_app`):** A more traditional firmware that turns the Pico W into a simple wireless keyboard.

The project is architected in C++ for performance and clarity, heavily leveraging the Pico's unique Programmable I/O (PIO) and demonstrating a robust cooperative multitasking approach to handle Bluetooth, Wi-Fi, and real-time hardware interrupts simultaneously.

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
    *   To minimize network traffic, the script compares the newly generated UI with the last frame sent and calculates the smallest rectangular "bounding box" of changed pixels.
    *   This "diff" is broken down into smaller "tiles" to fit within a reasonable payload size.
*   **Reliable TCP Communication with ACK-based Flow Control:**
    *   Uses a custom, framed TCP protocol to send image tiles to the Pico W.
    *   To ensure stability, the script sends only one tile at a time and waits for an "ACK" (acknowledgment) packet from the Pico before sending the next. This prevents the host from overwhelming the Pico's limited buffers.

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

*   **Cooperative Multitasking:** The `media_app` must handle three asynchronous tasks: Bluetooth events, Wi-Fi/TCP events, and user input. The final, stable architecture is built around a single, non-blocking main `while(true)` loop in `MediaApplication::run()`. This loop is the heart of the cooperative scheduler, paced by `cyw43_arch_wait_for_work_until()`. This function correctly yields CPU time to allow the BTstack (Bluetooth) and lwIP (Networking) background tasks to run without conflict, while also allowing the main thread to handle application logic safely.

*   **Correct Initialization Sequence:** A critical design element is the strict initialization order required by the Pico W SDK to prevent driver conflicts. The `MediaApplication::run()` method now enforces this order:
    1.  Core hardware (`stdio`, `RotaryEncoder`, `Display`).
    2.  `cyw43_arch_init()` to initialize the wireless driver stack.
    3.  BTstack and other application software services.
    4.  Wi-Fi connection and TCP server initialization.
    5.  Enter the main application loop.

*   **High-Priority Interrupt Handling:** The Pico W's wireless driver (`cyw43`) registers a high-priority raw interrupt handler for GPIO events. To prevent this from blocking the `RotaryEncoder`, the encoder's `init()` function now explicitly registers its own raw IRQ handler with a **higher priority** (a numerically lower value, `0x30`) than the CYW43 driver's default (`0x40`). This ensures encoder events are always captured without conflicting with wireless activity.

*   **Robust TCP Server with a Producer-Consumer Queue:** To solve the "fast producer, slow consumer" problem, where a fast host overwhelms the Pico, a robust producer-consumer pattern was implemented:
    *   **The Producer (Interrupt Context):** The low-level `_recv_callback` from lwIP is the producer. It runs in an interrupt context and does the absolute minimum: it validates an incoming data frame's CRC and, if valid, places the entire frame onto a multi-slot, thread-safe ring buffer (`m_tile_queue`).
    *   **The Consumer (Main Loop):** The `poll_handler()` function, called from the main `while(true)` loop, is the consumer. It is the only part of the code that removes items from the queue. It will only do so if the display is not busy drawing (`m_drawing.processDrawing()` returns `IDLE`).
    *   **Flow Control:** The crucial `ACK` packet is sent by the consumer (`poll_handler`) *after* it has successfully dequeued a tile and is about to start drawing it. This provides perfect, application-level flow control, ensuring the host never sends a new tile until the Pico is truly ready for it.

*   **Cooperative Display Driver:** The `St7789Display::drawBuffer` function contains a tight loop that sends thousands of pixels. To prevent this from starving the background network and Bluetooth tasks, a call to `cyw43_arch_poll()` is injected inside this loop. This periodically yields CPU time, allowing the wireless stack to process incoming data and send outgoing ACKs even during a slow screen redraw.

## Limitations and Design Trade-offs

*   **Cooperative, Not Preemptive:** The entire firmware runs in a cooperative, single-threaded environment. Any task that blocks for a long time without yielding (e.g., a long calculation or a driver without `cyw43_arch_poll()`) will starve all other tasks, including Bluetooth and Wi-Fi, leading to instability.
*   **Throughput is Limited by Drawing Speed:** The ACK-based flow control makes the network transfer extremely reliable, but it also means the overall data throughput is limited by the slowest part of the consumer chain: drawing pixels to the LCD.
*   **Memory Usage:** The tile buffer (`m_tile_queue`) and the asynchronous drawing buffer in the `Drawing` class consume RAM. While currently small, handling larger images or more buffered tiles would require careful memory management.
*   **Interrupt Priority:** The manual management of IRQ priorities is powerful but fragile. Adding other new, low-level hardware drivers would require careful consideration of the interrupt priority chain to avoid future conflicts.

### Host Application (Python)

*   **Modular Design:** The Python code is split into logical modules:
    *   `config.py`: Centralized configuration for IP addresses, pins, fonts, and colors.
    *   `weather.py`: Dedicated to fetching and parsing data from the weather API.
    *   `ui_generator.py`: Handles all graphics, icon drawing, and layout logic.
    *   `display_manager.py`: The main application orchestrator that handles device connection, the diff/tile algorithm, and the main update loop.
*   **Efficient Screen Updates:** The decision to implement a "diff and tile" algorithm is central to the project's performance. Instead of sending a full 150KB framebuffer every second, it sends only a few kilobytes when the time changes, making the application extremely network-efficient.
*   **Robust Communication:** The initial design used a simple echo. This was replaced with a more robust framed protocol. When stability issues arose, an ACK/NACK flow control system was added, and finally simplified by relying on TCP's inherent reliability. The final protocol sends self-contained `IMAGE_TILE` frames, which is both simple and effective.

## Key Problems Solved

This project overcame several complex integration challenges common in embedded systems. The debugging journey revealed critical insights into the Pico W's architecture:

1.  **Startup Deadlocks & GPIO Interrupt Conflicts:**
    *   **Problem:** The application would hang on startup, with logs stopping after "Initializing Display...". This was caused by `cyw43_arch_init()` and `RotaryEncoder::init()` competing to register a raw GPIO interrupt handler for the same hardware IRQ (IO_IRQ_BANK0). The driver, having a higher priority, would block the encoder, causing an assertion failure or deadlock.
    *   **Solution:** The `RotaryEncoder` was modified to register its raw IRQ handler with an explicit, higher priority (`0x30`) than the CYW43 driver's default (`0x40`). This allows the two handlers to coexist, with encoder events being processed first.

2.  **`PANIC - Attempted to sleep inside of an exception handler`:**
    *   **Problem:** An experimental architecture that moved the main application loop into a `btstack` timer caused a kernel panic. The `Display` driver uses `sleep_us()`, which is a blocking call forbidden in an interrupt context (where timers execute).
    *   **Solution:** The architecture was reverted to the canonical Pico SDK model: a single `while(true)` loop in the main thread that drives the scheduler via `cyw43_arch_wait_for_work_until()`. This ensures all application logic, including calls to the display driver, runs in a safe, non-interrupt context.

3.  **Network Instability & Buffer Overflows (The "Fast Producer, Slow Consumer" Problem):**
    *   **Problem:** The Python host, running on a fast PC, could send TCP data far faster than the Pico could process it and draw it to the screen. This caused internal lwIP buffers, and later the application's own buffers, to overflow, leading to data corruption, checksum mismatches, and dropped connections.
    *   **Solution:** A multi-stage, robust flow control system was implemented.
        1.  **ACK-Based Protocol:** The Python host was modified to wait for an ACK from the Pico after sending each tile.
        2.  **Decoupled ACK:** The `ACK` is now sent by the main loop (the consumer) only when it is ready to start drawing a new tile. This provides perfect application-level flow control.
        3.  **Multi-Slot Queue:** A thread-safe, multi-slot ring buffer was added to hold incoming tiles, allowing the fast network ISR to queue several tiles while the main loop is busy with a slow drawing operation.

4.  **Bluetooth Crashes Under Heavy Use:**
    *   **Problem:** Rapidly turning the rotary encoder would cause a BTstack assertion failure: `btstack_run_loop_base_add_timer`. This was caused by trying to add the `m_release_timer` to the scheduler when it was already scheduled.
    *   **Solution:** The event handling logic in `handle_encoder()` was made more robust. It now explicitly removes the timer (`btstack_run_loop_remove_timer`) before setting its new timeout and adding it back. This is the correct pattern for re-triggering an existing event timer.

Old ones:
5.  **Wi-Fi and BLE Coexistence:** Solved initial panics by using the correct lwIP-aware CYW43 driver library (`pico_cyw43_arch_lwip_threadsafe_background`) and a cooperative main loop (`cyw43_arch_wait_for_work_until`).
6.  **GPIO Interrupt Conflicts:** Resolved `Hard assert` panics by converting the `RotaryEncoder` from a simple GPIO callback to a high-priority Raw IRQ Handler, allowing it to coexist with the wireless driver's interrupt requirements.
7.  **Concurrency Panics (`sleep in handler`):** Eliminated panics by architecting a producer-consumer pattern. The TCP receive callback (interrupt context) only pushes data to a queue, while the main application loop is the only place where slow, blocking functions (`drawImage`) are called.
8.  **Memory Leaks (`pbuf_free`):** Fixed a subtle memory leak in the TCP server where empty packets were not being correctly freed, eventually exhausting the lwIP memory pool and causing a crash after long run times.
9.  **Inefficient Display Updates:** Implemented an intelligent "diff and tile" algorithm in the Python host to drastically reduce network traffic, only sending the portions of the screen that have actually changed.

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
    cmake -B build -G "Ninja"
    ```

3.  **Compile a Specific Target:**
    ```bash
    # To build the Media Controller
    cmake --build build --target media_app

    # To build the Keyboard
    ccmake --build build --target keyboard_app
    ```
    This will generate a `.uf2` file inside the `build` directory (e.g., `media_app.uf2`).

4.  **Flash the `.uf2` file:**
    1.  Hold down the **BOOTSEL** button on your Pico W and connect it to your computer.
    2.  It will appear as a USB Mass Storage device named `RPI-RP2`.
    3.  Drag and drop the generated `.uf2` file onto the device. The Pico W will automatically reboot and run the new firmware.

## Acknowledgements

*   The PIO program for the display driver is based on the official examples from the Raspberry Pi Pico SDK.
*   The original C-based keyboard example that inspired the `keyboard_app` is from the [pico-examples](https://github.com/raspberrypi/pico-examples) repository.