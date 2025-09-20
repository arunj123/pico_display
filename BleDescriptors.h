#ifndef BLE_DESCRIPTORS_H
#define BLE_DESCRIPTORS_H

#include <cstdint>
#include "btstack.h" // Required for the BLUETOOTH_DATA_TYPE macros

namespace BleDescriptors {

    // The HID Report Descriptor defines the device as a standard keyboard.
    // from USB HID Specification 1.1, Appendix B.1
    constexpr uint8_t hid_descriptor_keyboard_boot_mode[] = {
        0x05, 0x01, // Usage Page (Generic Desktop)
        0x09, 0x06, // Usage (Keyboard)
        0xa1, 0x01, // Collection (Application)
        0x85, 0x01, //   Report ID 1
        // Modifier byte
        0x75, 0x01, //   Report Size (1)
        0x95, 0x08, //   Report Count (8)
        0x05, 0x07, //   Usage Page (Key codes)
        0x19, 0xe0, //   Usage Minimum (Keyboard LeftControl)
        0x29, 0xe7, //   Usage Maximum (Keyboard Right GUI)
        0x15, 0x00, //   Logical Minimum (0)
        0x25, 0x01, //   Logical Maximum (1)
        0x81, 0x02, //   Input (Data, Variable, Absolute)
        // Reserved byte
        0x75, 0x01, //   Report Size (1)
        0x95, 0x08, //   Report Count (8)
        0x81, 0x03, //   Input (Constant, Variable, Absolute)
        // LED report
        0x95, 0x05, //   Report Count (5)
        0x75, 0x01, //   Report Size (1)
        0x05, 0x08, //   Usage Page (LEDs)
        0x19, 0x01, //   Usage Minimum (Num Lock)
        0x29, 0x05, //   Usage Maximum (Kana)
        0x91, 0x02, //   Output (Data, Variable, Absolute)
        // LED report padding
        0x95, 0x01, //   Report Count (1)
        0x75, 0x03, //   Report Size (3)
        0x91, 0x03, //   Output (Constant, Variable, Absolute)
        // Keycodes
        0x95, 0x06, //   Report Count (6)
        0x75, 0x08, //   Report Size (8)
        0x15, 0x00, //   Logical Minimum (0)
        0x25, 0xff, //   Logical Maximum (1)
        0x05, 0x07, //   Usage Page (Key codes)
        0x19, 0x00, //   Usage Minimum (Reserved (no event indicated))
        0x29, 0xff, //   Usage Maximum (Reserved)
        0x81, 0x00, //   Input (Data, Array)
        0xc0,       // End collection
    };

    // The advertising data packet tells other devices what this device is.
    constexpr uint8_t advertising_data[] = {
        // Flags general discoverable, BR/EDR not supported
        0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
        // Name
        0x0d, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'H', 'I', 'D', ' ', 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd',
        // 16-bit Service UUIDs, Human Interface Device
        0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8,
        // Appearance HID - Keyboard (Category 15, Sub-Category 1)
        0x03, BLUETOOTH_DATA_TYPE_APPEARANCE, 0xC1, 0x03,
    };

} // namespace BleDescriptors

#endif // BLE_DESCRIPTORS_H

