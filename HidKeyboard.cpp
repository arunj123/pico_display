#include "HidKeyboard.h"
#include "BleDescriptors.h"
#include "BtStackManager.h"
#include "hog_keyboard_demo.h"
#include <cstdio>
#include <cstring>
#include <cinttypes>

// --- Template Implementation ---

template <typename LayoutType>
HidKeyboard<LayoutType>::HidKeyboard() :
    m_connection_handle(HCI_CON_HANDLE_INVALID),
    m_protocol_mode(ProtocolMode::Report)
#ifdef HAVE_BTSTACK_STDIN
    , m_typing_state(TypingState::Idle)
#endif
{
    // Constructor body is clean
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::setup() {
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    setupServices();
    setupAdvertisements();
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
    hids_device_init(0, BleDescriptors::hid_descriptor_keyboard_boot_mode, sizeof(BleDescriptors::hid_descriptor_keyboard_boot_mode));
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::setupAdvertisements() {
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(ADV_INTERVAL_MIN, ADV_INTERVAL_MAX, 0, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(BleDescriptors::advertising_data), (uint8_t*)BleDescriptors::advertising_data);
    gap_advertisements_enable(1);
}

// --- FIX: sendReport now takes a const reference to the HidReport struct ---
template <typename LayoutType>
void HidKeyboard<LayoutType>::sendReport(const HidReport& report) {
    if (m_connection_handle == HCI_CON_HANDLE_INVALID) return;

    switch (m_protocol_mode) {
        case ProtocolMode::Boot:
            // Use the report's data() method to get a raw pointer for the C API
            hids_device_send_boot_keyboard_input_report(m_connection_handle, report.data(), HidReport::size());
            break;
        case ProtocolMode::Report:
            hids_device_send_input_report(m_connection_handle, report.data(), HidReport::size());
            break;
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
template <typename LayoutType>
void HidKeyboard<LayoutType>::processStdin(char character) {
    uint8_t c = character;
    btstack_ring_buffer_init(&m_ascii_input_buffer, m_ascii_input_storage, sizeof(m_ascii_input_storage));
    btstack_ring_buffer_write(&m_ascii_input_buffer, &c, 1);
    if (m_typing_state == TypingState::Idle && m_connection_handle != HCI_CON_HANDLE_INVALID) {
        m_typing_state = TypingState::WaitingToSendFromBuffer;
        hids_device_request_can_send_now_event(m_connection_handle);
    }
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::canSendNow() {
    if (m_typing_state == TypingState::WaitingToSendFromBuffer) {
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
            // --- FIX: Create a report object and store it ---
            m_report_to_send = HidReport{}; // Start with a clean report
            m_report_to_send.modifier = key.modifier;
            m_report_to_send.setKey(key.keycode);
            
            sendReport(m_report_to_send);
            m_typing_state = TypingState::WaitingToSendKeyUp;
            hids_device_request_can_send_now_event(m_connection_handle);
        }
    } else if (m_typing_state == TypingState::WaitingToSendKeyUp) {
        // --- FIX: Send a static "key up" report ---
        sendReport(HidReport::keyUp());
        
        if (btstack_ring_buffer_bytes_available(&m_ascii_input_buffer)) {
            m_typing_state = TypingState::WaitingToSendFromBuffer;
            hids_device_request_can_send_now_event(m_connection_handle);
        } else {
            m_typing_state = TypingState::Idle;
        }
    }
}
#else
// Automatic demo mode implementation
template <typename LayoutType>
void HidKeyboard<LayoutType>::startTypingDemo() {
    printf("Start typing demo...\n");
    m_typing_timer.process = &BtStackManager::typingTimerForwarder;
    btstack_run_loop_set_timer(&m_typing_timer, TYPING_PERIOD_MS);
    btstack_run_loop_add_timer(&m_typing_timer);
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::typingTimerHandler(btstack_timer_source_t* ts) {
    if (m_connection_handle == HCI_CON_HANDLE_INVALID) return;

    if (m_send_keyup) {
        m_send_keyup = false;
        // --- FIX: Set the report to send to a "key up" state ---
        m_report_to_send = HidReport::keyUp();
        hids_device_request_can_send_now_event(m_connection_handle);
    } else {
        uint8_t character = m_demo_text[m_demo_pos++];
        if (m_demo_text[m_demo_pos] == 0) m_demo_pos = 0;
        
        Keypress key;
        if (m_layout.findKey(character, key)) {
            printf("%c\n", character);
            // --- FIX: Populate the report struct ---
            m_report_to_send = HidReport{};
            m_report_to_send.modifier = key.modifier;
            m_report_to_send.setKey(key.keycode);

            m_send_keyup = true;
            hids_device_request_can_send_now_event(m_connection_handle);
        }
    }
    btstack_run_loop_set_timer(ts, TYPING_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::canSendNow() {
    // --- FIX: Simply send the report we've already prepared ---
    sendReport(m_report_to_send);
}
#endif // HAVE_BTSTACK_STDIN

// Explicitly instantiate the template for our specific keyboard type
template class HidKeyboard<USKeyboardLayout>;