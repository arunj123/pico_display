#include "HidKeyboard.h"
#include "hog_keyboard_demo.h"
#include "btstack_stdin.h"
#include <cstdio>
#include <cstring>
#include <cinttypes>

// --- HID Report Descriptor (unchanged) ---
static constexpr uint8_t hid_descriptor_keyboard_boot_mode[] =  {

    0x05, 0x01,                    // Usage Page (Generic Desktop)
    0x09, 0x06,                    // Usage (Keyboard)
    0xa1, 0x01,                    // Collection (Application)

    0x85,  0x01,                   // Report ID 1

    // Modifier byte

    0x75, 0x01,                    //   Report Size (1)
    0x95, 0x08,                    //   Report Count (8)
    0x05, 0x07,                    //   Usage Page (Key codes)
    0x19, 0xe0,                    //   Usage Minimum (Keyboard LeftControl)
    0x29, 0xe7,                    //   Usage Maxium (Keyboard Right GUI)
    0x15, 0x00,                    //   Logical Minimum (0)
    0x25, 0x01,                    //   Logical Maximum (1)
    0x81, 0x02,                    //   Input (Data, Variable, Absolute)

    // Reserved byte

    0x75, 0x01,                    //   Report Size (1)
    0x95, 0x08,                    //   Report Count (8)
    0x81, 0x03,                    //   Input (Constant, Variable, Absolute)

    // LED report + padding

    0x95, 0x05,                    //   Report Count (5)
    0x75, 0x01,                    //   Report Size (1)
    0x05, 0x08,                    //   Usage Page (LEDs)
    0x19, 0x01,                    //   Usage Minimum (Num Lock)
    0x29, 0x05,                    //   Usage Maxium (Kana)
    0x91, 0x02,                    //   Output (Data, Variable, Absolute)

    0x95, 0x01,                    //   Report Count (1)
    0x75, 0x03,                    //   Report Size (3)
    0x91, 0x03,                    //   Output (Constant, Variable, Absolute)

    // Keycodes

    0x95, 0x06,                    //   Report Count (6)
    0x75, 0x08,                    //   Report Size (8)
    0x15, 0x00,                    //   Logical Minimum (0)
    0x25, 0xff,                    //   Logical Maximum (1)
    0x05, 0x07,                    //   Usage Page (Key codes)
    0x19, 0x00,                    //   Usage Minimum (Reserved (no event indicated))
    0x29, 0xff,                    //   Usage Maxium (Reserved)
    0x81, 0x00,                    //   Input (Data, Array)

    0xc0,                          // End collection
};

// --- Callback Forwarders ---
extern HidKeyboard g_keyboard; 
static void packet_handler_forwarder(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    g_keyboard.handlePacket(packet_type, channel, packet, size);
}

// --- Class Implementation ---

HidKeyboard::HidKeyboard() :
    m_connection_handle(HCI_CON_HANDLE_INVALID),
    m_protocol_mode(ProtocolMode::Report)
    // --- FIX: Remove invalid initialization. m_layout is default-constructed automatically. ---
#ifdef HAVE_BTSTACK_STDIN
    , m_typing_state(TypingState::Idle)
#endif
{
    memset(&m_hci_event_callback_reg, 0, sizeof(m_hci_event_callback_reg));
    memset(&m_sm_event_callback_reg, 0, sizeof(m_sm_event_callback_reg));
}

// --- FIX: Remove unnecessary destructor definition ---

void HidKeyboard::setup() {
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    setupServices();
    setupAdvertisements();

    m_hci_event_callback_reg.callback = &packet_handler_forwarder;
    hci_add_event_handler(&m_hci_event_callback_reg);

    m_sm_event_callback_reg.callback = &packet_handler_forwarder;
    sm_add_event_handler(&m_sm_event_callback_reg);

    hids_device_register_packet_handler(packet_handler_forwarder);

#ifdef HAVE_BTSTACK_STDIN
    btstack_ring_buffer_init(&m_ascii_input_buffer, m_ascii_input_storage, sizeof(m_ascii_input_storage));
#endif
}

void HidKeyboard::powerOn() {
    hci_power_control(HCI_POWER_ON);
}

void HidKeyboard::setupServices() {
    att_server_init(profile_data, nullptr, nullptr);
    battery_service_server_init(BATTERY_LEVEL);
    device_information_service_server_init();
    hids_device_init(0, hid_descriptor_keyboard_boot_mode, sizeof(hid_descriptor_keyboard_boot_mode));
}

void HidKeyboard::setupAdvertisements() {
    static const uint8_t adv_data[] = {
        0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
        0x0d, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'H', 'I', 'D', ' ', 'K', 'e', 'y', 'b', 'o', 'a', 'r', 'd',
        0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8,
        0x03, BLUETOOTH_DATA_TYPE_APPEARANCE, 0xC1, 0x03,
    };
    
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(ADV_INTERVAL_MIN, ADV_INTERVAL_MAX, 0, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(adv_data), (uint8_t*)adv_data);
    gap_advertisements_enable(1);
}

void HidKeyboard::sendReport(uint8_t modifier, uint8_t keycode) {
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

void HidKeyboard::handlePacket(uint8_t packet_type, uint16_t, uint8_t *packet, uint16_t) {
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
                    canSendNow();
                    break;
                default: break;
            }
            break;
        default: break;
    }
}

#ifdef HAVE_BTSTACK_STDIN
void HidKeyboard::processStdin(char character) {
    uint8_t c = character;
    btstack_ring_buffer_write(&m_ascii_input_buffer, &c, 1);
    if (m_typing_state == TypingState::Idle && m_connection_handle != HCI_CON_HANDLE_INVALID) {
        m_typing_state = TypingState::WaitingToSendFromBuffer;
        hids_device_request_can_send_now_event(m_connection_handle);
    }
}

void HidKeyboard::canSendNow() {
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
            // --- FIX: Use . instead of -> to access member of direct object ---
            if (m_layout.findKey(c, key)) {
                printf("sending: %c\n", c);
                sendReport(key.modifier, key.keycode);
                m_typing_state = TypingState::WaitingToSendKeyUp;
                hids_device_request_can_send_now_event(m_connection_handle);
            }
            break;
        }
        case TypingState::WaitingToSendKeyUp:
            sendReport(0, 0); // Key up
            if (btstack_ring_buffer_bytes_available(&m_ascii_input_buffer)) {
                m_typing_state = TypingState::WaitingToSendFromBuffer;
                hids_device_request_can_send_now_event(m_connection_handle);
            } else {
                m_typing_state = TypingState;
            }
            break;
        default: break;
    }
}
#else
// Automatic demo mode implementation
void HidKeyboard::startTypingDemo() {
    printf("Start typing demo...\n");
    m_demo_pos = 0;
    m_send_keyup = false;
    m_typing_timer.process = &HidKeyboard::s_typingTimerHandler;
    btstack_run_loop_set_timer(&m_typing_timer, TYPING_PERIOD_MS);
    btstack_run_loop_add_timer(&m_typing_timer);
}

void HidKeyboard::s_typingTimerHandler(btstack_timer_source_t* ts) {
    g_keyboard.typingTimerHandler(ts);
}

void HidKeyboard::typingTimerHandler(btstack_timer_source_t* ts) {
    if (m_connection_handle == HCI_CON_HANDLE_INVALID) return;

    if (m_send_keyup) {
        m_send_keyup = false;
        m_key_to_send = {0, 0}; // Send key up
        hids_device_request_can_send_now_event(m_connection_handle);
    } else {
        uint8_t character = m_demo_text[m_demo_pos++];
        if (m_demo_text[m_demo_pos] == 0) {
            m_demo_pos = 0;
        }
        
        // --- FIX: Use . instead of -> to access member of direct object ---
        if (m_layout.findKey(character, m_key_to_send)) {
            printf("%c\n", character);
            m_send_keyup = true;
            hids_device_request_can_send_now_event(m_connection_handle);
        }
    }

    btstack_run_loop_set_timer(ts, TYPING_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
}

void HidKeyboard::canSendNow() {
    sendReport(m_key_to_send.modifier, m_key_to_send.keycode);
}
#endif
