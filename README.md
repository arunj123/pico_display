# Pico W Standalone Bluetooth Keyboard

This project provides implementations for a standalone Bluetooth Low Energy (BLE) keyboard and a media controller using a Raspberry Pi Pico W.

## Binaries

This project can be built to produce two different binaries:

1.  **Keyboard Example:** A BLE keyboard example, initially taken from the pico-sdk and then modified to build as C++.
2.  **Media Controller:** A media controller that uses a rotary encoder for input.

## Wiring

Connect the rotary encoder to the Raspberry Pi Pico W as follows. Note that the encoder's `+` pin should be left unconnected.

| Encoder Pin | Pico Pin         | Description        |
|-------------|------------------|--------------------|
| `GND`       | `GND` (Pin 38)   | Ground             |
| `S1`        | `GPIO 10` (Pin 14) | Encoder Signal A   |
| `S2`        | `GPIO 11` (Pin 15) | Encoder Signal B   |
| `KEY`       | `GPIO 12` (Pin 16) | Push-Button Switch |

| GMT020-02 Pin Name (Datasheet pg. 6) | Function | `config.h` Constant | Pico Pin Name (Recommended) |
| :--- | :--- | :--- | :--- |
| **SDA** (Pin 21) | Serial Data | `DISPLAY_PIN_DIN` | GP0 |
| **SCL** (Pin 17, marked RS) | Serial Clock | `DISPLAY_PIN_CLK` | GP1 |
| **CS** (Pin 18) | Chip Select | `DISPLAY_PIN_CS` | GP2 |
| **RS** (Pin 17) | Data/Command | `DISPLAY_PIN_DC` | GP3 |
| **RESET** (Pin 20) | Reset | `DISPLAY_PIN_RESET` | GP4 |
| **LED+** (Pin 1) / **LED-** (Pin 2) | Backlight Power | `DISPLAY_PIN_BL` | GP5 |

## Acknowledgements

The PIO program (`pio_rotary_encoder.pio`) and the C++ class-based approach for reading the encoder are adapted from the excellent work by Jerome T. (`GitJer`).

-   **Source:** [https://github.com/GitJer/Rotary_encoder](https://github.com/GitJer/Rotary_encoder)

Ai studio help: https://aistudio.google.com/prompts/1D4fNG2rjF4TBSy8I3s4RhmVrPLea1HU7