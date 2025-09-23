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

### Final Wiring Guide for GMT020-02

Wire board as follows before flashing:

| GMT020-02 Pin | Function | Your GPIO Pin | Pico Pin Name |
| :--- | :--- | :--- | :--- |
| **CS** | Chip Select | **20** | GP20 |
| **SCL** | Serial Clock | **16** | GP16 |
| **SDA** | Serial Data | **17** | GP17 |
| **DC** / **RS** | Data/Command | **19** | GP19 |
| **RST** | Reset | **18** | GP18 |
| **VCC** | Power (3.3V) | - | 3V3(OUT) |
| **GND** | Ground | - | GND |
| **LED+** / **LED-** | Backlight Power | - | (e.g., to 3V3 and GND) |

## Acknowledgements

The PIO program (`pio_rotary_encoder.pio`) and the C++ class-based approach for reading the encoder are adapted from the excellent work by Jerome T. (`GitJer`).

-   **Source:** [https://github.com/GitJer/Rotary_encoder](https://github.com/GitJer/Rotary_encoder)

Ai studio help: https://aistudio.google.com/prompts/1D4fNG2rjF4TBSy8I3s4RhmVrPLea1HU7