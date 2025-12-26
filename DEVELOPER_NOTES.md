# Developer Notes & Project Architecture

This document serves as a comprehensive log of the architectural decisions, design patterns, and critical problems solved during the development of the Pico W BLE Media Controller project. Its purpose is to provide deep context for future development and for AI-based code analysis.

## 1. Core Firmware Architecture (C++)

The `media_app` firmware is built on a **cooperative multitasking** model, which is essential for managing three asynchronous tasks on a single core without a preemptive RTOS:
1.  Bluetooth events (BTstack)
2.  Wi-Fi/TCP events (lwIP)
3.  User input (Rotary Encoder interrupts) and display updates.

### 1.1. Cooperative Scheduler & Main Loop
The heart of the system is the single, non-blocking `while(true)` loop in `media_main.cpp`, which calls `btstack_run_loop_execute()`. This loop, paced by `cyw43_arch_wait_for_work_until()`, acts as the cooperative scheduler. This function is critical because it:
-   Yields CPU time to the background tasks for the BTstack (Bluetooth) and lwIP (Networking) drivers.
-   Prevents race conditions by ensuring all application logic (e.g., drawing, handling encoder state) runs in a single, non-interrupt context.
-   Avoids panics common in naive implementations, such as calling blocking functions like `sleep_ms()` from an interrupt handler.

### 1.2. Strict Initialization Sequence
The Pico W SDK requires a specific initialization order to prevent driver conflicts, especially between the wireless chip (CYW43) and other hardware peripherals. The `main()` and `MediaApplication::setup()` methods enforce this sequence:
1.  **Core Hardware:** `stdio`, `RotaryEncoder`, `Display`.
2.  **Wireless Driver Stack:** `cyw43_arch_init()`. This must happen before any software services that depend on it.
3.  **Bluetooth Stack:** BTstack and other application services are initialized.
4.  **Wi-Fi Connection:** The Wi-Fi connection and TCP server are started.
5.  **Enter Main Loop:** The `btstack_run_loop_execute()` scheduler is entered.

### 1.3. High-Priority Interrupt Handling for Rotary Encoder
A major challenge was the conflict between the `RotaryEncoder`'s GPIO interrupts and the CYW43 wireless driver's internal interrupt handler. Both compete for the same hardware IRQ (`IO_IRQ_BANK0`).
-   **Problem:** The CYW43 driver registers its handler with a default priority of `0x40`. If the encoder registers its handler naively, the CYW43 driver can preempt it, causing missed inputs or system deadlocks.
-   **Solution:** The `RotaryEncoder::init()` function was modified to register a **raw IRQ handler** with an explicit, **higher priority** (a numerically lower value, `0x30`). This ensures encoder events are always captured immediately, without conflicting with wireless activity.

### 1.4. Robust TCP Server: The Producer-Consumer Pattern
The "fast producer, slow consumer" problem is a classic embedded systems challenge. A fast host PC can send TCP data far quicker than the Pico can draw it to the slow SPI LCD, leading to buffer overflows and network instability.
-   **Producer (Interrupt Context):** The low-level `_recv_callback` from lwIP is the producer. It runs in an interrupt context and performs minimal work: it validates an incoming data frame's CRC and, if valid, places the entire frame onto a multi-slot, thread-safe ring buffer (`m_tile_queue`).
-   **Consumer (Main Loop Context):** The `poll_handler()` function, called from the main loop, is the consumer. It is the only part of the code that removes items from the queue. It will only process a new tile if the display is not busy with a previous drawing operation.
-   **Application-Level Flow Control:** The crucial `ACK` packet is sent by the consumer (`poll_handler`) *after* it has successfully dequeued a tile and is ready to start drawing it. This provides perfect, reliable flow control, ensuring the host never sends a new tile until the Pico is truly ready for it.

### 1.5. Cooperative Display Driver
A full-screen draw operation involves sending thousands of pixels over SPI, which can take tens of milliseconds. A naive implementation would block the main loop, starving the wireless stack.
-   **Problem:** A long-running `drawBuffer` loop would prevent `cyw43_arch_poll()` from being called, leading to missed TCP packets, lost ACKs, and Bluetooth disconnects.
-   **Solution:** A call to `cyw43_arch_poll()` is injected inside the tight pixel-sending loop in `St7789Display::drawBuffer`. This call is made every 64 pixels, periodically yielding CPU time and allowing the wireless stack to process its background tasks seamlessly, even during a slow screen redraw.

---

## 2. Host Application Architecture (Python)

The Python host application is designed for clarity, efficiency, and robustness.
-   **Modular Design:** The code is split into logical modules:
    -   `config.py`: Centralized configuration.
    -   `weather.py`: Handles weather API interaction.
    -   `ui_generator.py`: All graphics and layout logic.
    -   `display_manager.py`: The main orchestrator.
-   **Efficient "Diff & Tile" Algorithm:** This is central to the project's performance. Instead of sending a full 150KB framebuffer every second, it sends only a few kilobytes when the time changes by:
    1.  Comparing the new UI with the last frame sent.
    2.  Calculating the smallest rectangular "bounding box" of changed pixels.
    3.  Breaking this "diff" into smaller "tiles" to fit within the protocol's payload size.
-   **Reliable TCP Communication:** The protocol uses `ACK`-based flow control. The host sends one tile and blocks, waiting for an `ACK` from the Pico before proceeding. This completely solves the buffer overflow problem by slaving the transfer speed to the Pico's drawing speed.

---

## 3. Key Problems Solved & Debugging Journey

This project overcame several complex integration challenges. The solutions are now embedded in the architecture.

1.  **Startup Deadlocks & GPIO Interrupt Conflicts**
    *   **Problem:** The application would hang on startup. This was caused by `cyw43_arch_init()` and `RotaryEncoder::init()` competing to register a raw GPIO interrupt handler for the same hardware IRQ.
    *   **Solution:** The `RotaryEncoder` now registers its raw IRQ handler with an explicit, higher priority (`0x30`) than the CYW43 driver's default (`0x40`), allowing them to coexist.

2.  **Kernel Panic: `Attempted to sleep inside of an exception handler`**
    *   **Problem:** An early architecture tried to run application logic inside a `btstack` timer callback. The `Display` driver uses `sleep_us()`, a blocking call forbidden in an interrupt context (where timers execute).
    *   **Solution:** The architecture was reverted to the canonical Pico SDK model: a single `while(true)` loop that drives a cooperative scheduler. All application logic now runs in a safe, non-interrupt context.

3.  **Network Instability & Buffer Overflows**
    *   **Problem:** The Python host sent TCP data faster than the Pico could process it, causing lwIP's internal buffers to overflow, leading to corrupted data and dropped connections.
    *   **Solution:** The multi-stage, robust flow control system described in sections 1.4 and 2 was implemented (Producer-Consumer queue + ACK-based protocol).

4.  **Bluetooth Crashes Under Heavy Use**
    *   **Problem:** Rapidly turning the rotary encoder caused a BTstack assertion failure (`btstack_run_loop_base_add_timer`) because the code was trying to add a timer that was already scheduled.
    *   **Solution:** The event handling logic in `handle_encoder()` now explicitly removes the timer (`btstack_run_loop_remove_timer`) before setting its new timeout and adding it back. This is the correct pattern for re-triggering an existing BTstack timer.

5.  **Memory Leaks (`pbuf_free`)**
    *   **Problem:** A subtle memory leak was discovered in the TCP server where empty or malformed packets were not being correctly freed, eventually exhausting the lwIP memory pool and causing a crash after long run times.
    *   **Solution:** The `_recv_callback` logic was refactored to ensure `pbuf_free(p)` is called on all code paths, and the stream processing logic robustly handles buffer boundaries.

---

## 4. Limitations and Design Trade-offs

*   **Cooperative, Not Preemptive:** The firmware runs in a cooperative, single-threaded environment. Any task that blocks for a long time without yielding (e.g., a long calculation or a driver without `cyw43_arch_poll()`) will starve all other tasks, including Bluetooth and Wi-Fi.
*   **Throughput is Limited by Drawing Speed:** The ACK-based flow control makes the network transfer extremely reliable, but the overall data throughput is bottlenecked by the slowest part of the consumer chain: drawing pixels to the LCD.
*   **Memory Usage:** The tile queue (`m_tile_queue`) and the asynchronous drawing buffer in the `Drawing` class consume RAM. Handling larger images would require careful memory management.
*   **Interrupt Priority:** The manual management of IRQ priorities is powerful but fragile. Adding other low-level hardware drivers would require careful consideration of the interrupt priority chain to avoid future conflicts.
