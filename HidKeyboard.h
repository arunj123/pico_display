#ifndef HID_KEYBOARD_H
#define HID_KEYBOARD_H

#include "btstack.h"
#include "hog_keyboard_demo.h"
#include "KeyboardLayout.h"
#include "BleDescriptors.h" // <-- Include the new header for BLE data
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cinttypes>

// Forward declaration
template <typename LayoutType>
class HidKeyboard;

template <typename LayoutType>
HidKeyboard<LayoutType>* g_keyboard_instance = nullptr;

// Template Class Definition
template <typename LayoutType>
class HidKeyboard {
public:
    HidKeyboard() :
        m_connection_handle(HCI_CON_HANDLE_INVALID),
        m_protocol_mode(ProtocolMode::Report)
    #ifdef HAVE_BTSTACK_STDIN
        , m_typing_state(TypingState::Idle)
    #endif
    {
        g_keyboard_instance<LayoutType> = this;
        memset(&m_hci_event_callback_reg, 0, sizeof(m_hci_event_callback_reg));
        memset(&m_sm_event_callback_reg, 0, sizeof(m_sm_event_callback_reg));
    }

    void setup();
    void powerOn();
    void processStdin(char character);

private:
    static void s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
    static void s_can_send_now();
#ifndef HAVE_BTSTACK_STDIN
    static void s_typing_timer_handler(btstack_timer_source_t* ts);
#endif

    void handlePacket(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
    void canSendNow();

    enum class ProtocolMode : uint8_t { Boot = 0, Report = 1 };
    enum class TypingState { Idle, WaitingToSendFromBuffer, WaitingToSendKeyUp };

    static constexpr uint8_t BATTERY_LEVEL = 100;
    static constexpr uint16_t ADV_INTERVAL_MIN = 0x0030;
    static constexpr uint16_t ADV_INTERVAL_MAX = 0x0030;

    void setupServices();
    void setupAdvertisements();
    void sendReport(uint8_t modifier, uint8_t keycode);
    
    btstack_packet_callback_registration_t m_hci_event_callback_reg;
    btstack_packet_callback_registration_t m_sm_event_callback_reg;
    hci_con_handle_t m_connection_handle;
    ProtocolMode m_protocol_mode;
    
    LayoutType m_layout;

#ifdef HAVE_BTSTACK_STDIN
    TypingState m_typing_state;
    btstack_ring_buffer_t m_ascii_input_buffer;
    uint8_t m_ascii_input_storage[20];
#else
    void startTypingDemo();
    void typingTimerHandler(btstack_timer_source_t* ts);

    static constexpr int TYPING_PERIOD_MS = 50;
    const char* m_demo_text = "\n\nHello World!\n\nThis is a C++ BTstack HID Keyboard.\n\n";
    int m_demo_pos = 0;
    btstack_timer_source_t m_typing_timer;
    Keypress m_key_to_send = {0, 0};
    bool m_send_keyup = false;
#endif
};

// --- Template Implementation ---

// --- FIX: HID descriptor array is REMOVED from here ---

template <typename LayoutType>
void HidKeyboard<LayoutType>::setup() {
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    setupServices();
    setupAdvertisements();

    m_hci_event_callback_reg.callback = &HidKeyboard::s_packet_handler;
    hci_add_event_handler(&m_hci_event_callback_reg);

    m_sm_event_callback_reg.callback = &HidKeyboard::s_packet_handler;
    sm_add_event_handler(&m_sm_event_callback_reg);

    hids_device_register_packet_handler(&HidKeyboard::s_packet_handler);

#ifdef HAVE_BTSTACK_STDIN
    btstack_ring_buffer_init(&m_ascii_input_buffer, m_ascii_input_storage, sizeof(m_ascii_input_storage));
#endif
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::powerOn() {
    hci_power_control(HCI_POWER_ON);
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::setupServices() {
    att_server_init(profile_data, nullptr, nullptr);
    battery_service_server_init(BATTERY_LEVEL);
    device_information_service_server_init();
    // --- FIX: Use descriptor from the BleDescriptors namespace ---
    hids_device_init(0, BleDescriptors::hid_descriptor_keyboard_boot_mode, sizeof(BleDescriptors::hid_descriptor_keyboard_boot_mode));
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::setupAdvertisements() {
    // --- FIX: Advertising data array is now defined in BleDescriptors.h ---
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(ADV_INTERVAL_MIN, ADV_INTERVAL_MAX, 0, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(BleDescriptors::advertising_data), (uint8_t*)BleDescriptors::advertising_data);
    gap_advertisements_enable(1);
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::sendReport(uint8_t modifier, uint8_t keycode) {
    uint8_t report[] = { modifier, 0, keycode, 0, 0, 0, 0, 0 };
    if (m_connection_handle == HCI_CON_HANDLE_INVALID) return;

    switch (m_protocol_mode) {
        case ProtocolMode::Boot:
            hids_device_send_boot_keyboard_input_report(m_connection_handle, report, sizeof(report));
            break;
        case ProtocolMode::Report:
            hids_device_send_input_report(m_connection_handle, report, sizeof(report));
            break;
    }
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (g_keyboard_instance<LayoutType>) {
        g_keyboard_instance<LayoutType>->handlePacket(packet_type, channel, packet, size);
    }
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::handlePacket(uint8_t packet_type, uint16_t, uint8_t *packet, uint16_t) {
    if (packet_type != HCI_EVENT_PACKET) return;
    switch (hci_event_packet_get_type(packet)) {
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            m_connection_handle = HCI_CON_HANDLE_INVALID;
            printf("Disconnected\n");
            break;
        case SM_EVENT_JUST_WORKS_REQUEST:
            printf("Just Works requested\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;
        case SM_EVENT_NUMERIC_COMPARISON_REQUEST:
            printf("Confirming numeric comparison: %" PRIu32 "\n", sm_event_numeric_comparison_request_get_passkey(packet));
            sm_numeric_comparison_confirm(sm_event_passkey_display_number_get_handle(packet));
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            printf("Display Passkey: %" PRIu32 "\n", sm_event_passkey_display_number_get_passkey(packet));
            break;
        case HCI_EVENT_HIDS_META:
            switch (hci_event_hids_meta_get_subevent_code(packet)) {
                case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
                    m_connection_handle = hids_subevent_input_report_enable_get_con_handle(packet);
                    printf("Report Characteristic Subscribed %u\n", hids_subevent_input_report_enable_get_enable(packet));
                #ifndef HAVE_BTSTACK_STDIN
                    if (hids_subevent_input_report_enable_get_enable(packet)){
                        startTypingDemo();
                    }
                #endif
                    break;
                case HIDS_SUBEVENT_PROTOCOL_MODE:
                    m_protocol_mode = static_cast<ProtocolMode>(hids_subevent_protocol_mode_get_protocol_mode(packet));
                    printf("Protocol Mode: %s mode\n", (m_protocol_mode == ProtocolMode::Report) ? "Report" : "Boot");
                    break;
                case HIDS_SUBEVENT_CAN_SEND_NOW:
                    s_can_send_now();
                    break;
                default: break;
            }
            break;
        default: break;
    }
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::s_can_send_now() {
    if (g_keyboard_instance<LayoutType>) {
        g_keyboard_instance<LayoutType>->canSendNow();
    }
}

#ifdef HAVE_BTSTACK_STDIN
template <typename LayoutType>
void HidKeyboard<LayoutType>::processStdin(char character) {
    uint8_t c = character;
    btstack_ring_buffer_write(&m_ascii_input_buffer, &c, 1);
    if (m_typing_state == TypingState::Idle && m_connection_handle != HCI_CON_HANDLE_INVALID) {
        m_typing_state = TypingState::WaitingToSendFromBuffer;
        hids_device_request_can_send_now_event(m_connection_handle);
    }
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::canSendNow() {
    switch (m_typing_state) {
        case TypingState::WaitingToSendFromBuffer: {
            uint8_t c;
            uint32_t num_bytes_read;
            btstack_ring_buffer_read(&m_ascii_input_buffer, &c, 1, &num_bytes_read);
            if (num_bytes_read == 0) {
                m_typing_state = TypingState::Idle;
                return;
            }
            Keypress key;
            if (m_layout.findKey(c, key)) {
                printf("sending: %c\n", c);
                sendReport(key.modifier, key.keycode);
                m_typing_state = TypingState::WaitingToSendKeyUp;
                hids_device_request_can_send_now_event(m_connection_handle);
            }
            break;
        }
        case TypingState::WaitingToSendKeyUp:
            sendReport(0, 0);
            if (btstack_ring_buffer_bytes_available(&m_ascii_input_buffer)) {
                m_typing_state = TypingState::WaitingToSendFromBuffer;
                hids_device_request_can_send_now_event(m_connection_handle);
            } else {
                m_typing_state = TypingState::Idle;
            }
            break;
        default: break;
    }
}
#else
// Automatic demo mode implementation
template <typename LayoutType>
void HidKeyboard<LayoutType>::startTypingDemo() {
    printf("Start typing demo...\n");
    m_typing_timer.process = &HidKeyboard::s_typing_timer_handler;
    btstack_run_loop_set_timer(&m_typing_timer, TYPING_PERIOD_MS);
    btstack_run_loop_add_timer(&m_typing_timer);
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::s_typing_timer_handler(btstack_timer_source_t* ts) {
    if (g_keyboard_instance<LayoutType>) {
        g_keyboard_instance<LayoutType>->typingTimerHandler(ts);
    }
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::typingTimerHandler(btstack_timer_source_t* ts) {
    if (m_connection_handle == HCI_CON_HANDLE_INVALID) return;

    if (m_send_keyup) {
        m_send_keyup = false;
        m_key_to_send = {0, 0};
        hids_device_request_can_send_now_event(m_connection_handle);
    } else {
        uint8_t character = m_demo_text[m_demo_pos++];
        if (m_demo_text[m_demo_pos] == 0) m_demo_pos = 0;
        
        if (m_layout.findKey(character, m_key_to_send)) {
            printf("%c\n", character);
            m_send_keyup = true;
            hids_device_request_can_send_now_event(m_connection_handle);
        }
    }
    btstack_run_loop_set_timer(ts, TYPING_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::canSendNow() {
    sendReport(m_key_to_send.modifier, m_key_to_send.keycode);
}
#endif // HAVE_BTSTACK_STDIN

#endif // HID_KEYBOARD_H