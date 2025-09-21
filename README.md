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

## Acknowledgements

The PIO program (`pio_rotary_encoder.pio`) and the C++ class-based approach for reading the encoder are adapted from the excellent work by Jerome T. (`GitJer`).

-   **Source:** [https://github.com/GitJer/Rotary_encoder](https://github.com/GitJer/Rotary_encoder)