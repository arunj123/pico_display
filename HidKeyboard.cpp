#include "HidKeyboard.h"
#include "I_InputController.h"
#include "BleDescriptors.h"
#include "hog_keyboard_demo.h"
#include <cstdio>
#include <cstring>
#include <cinttypes>

// --- Template Implementation ---

template <typename LayoutType>
HidKeyboard<LayoutType>::HidKeyboard() :
    m_connection_handle(HCI_CON_HANDLE_INVALID),
    m_protocol_mode(ProtocolMode::Report)
{}

template <typename LayoutType>
void HidKeyboard<LayoutType>::setInputController(I_InputController* controller) {
    m_input_controller = controller;
}

template <typename LayoutType>
bool HidKeyboard<LayoutType>::isConnected() const {
    return m_connection_handle != HCI_CON_HANDLE_INVALID;
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::requestToSend() {
    if (isConnected()) {
        hids_device_request_can_send_now_event(m_connection_handle);
    }
}

template <typename LayoutType>
bool HidKeyboard<LayoutType>::sendChar(char character) {
    Keypress key;
    if (m_layout.findKey(character, key)) {
        HidReport report{};
        report.modifier = key.modifier;
        report.setKey(key.keycode);
        sendReport(report);
        return true;
    }
    return false;
}

template <typename LayoutType>
bool HidKeyboard<LayoutType>::charToKeypress(char character, Keypress& out_key) const {
    return m_layout.findKey(character, out_key);
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
    battery_service_server_init(100);
    device_information_service_server_init();
    hids_device_init(0, BleDescriptors::hid_descriptor_keyboard_boot_mode, sizeof(BleDescriptors::hid_descriptor_keyboard_boot_mode));
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::setupAdvertisements() {
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(0x0030, 0x0030, 0, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(BleDescriptors::advertising_data), (uint8_t*)BleDescriptors::advertising_data);
    gap_advertisements_enable(1);
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::sendReport(const HidReport& report) {
    if (isConnected()) {
        hids_device_send_input_report(m_connection_handle, report.data(), HidReport::size());
    }
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::handlePacket(uint8_t packet_type, uint16_t, uint8_t* packet, uint16_t) {
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
                    
                    // --- FIX: Notify the input controller that we are ready to send ---
                    if (m_input_controller && isConnected()) {
                        m_input_controller->onHidSubscribed();
                    }
                    break;
                case HIDS_SUBEVENT_CAN_SEND_NOW:
                    if (m_input_controller) m_input_controller->onReadyToSend();
                    break;
                default: break;
            }
            break;
        default: break;
    }
}

template <typename LayoutType>
void HidKeyboard<LayoutType>::canSendNow() {
    if (m_input_controller) {
        m_input_controller->onReadyToSend();
    }
}

#ifndef HAVE_BTSTACK_STDIN
template <typename LayoutType>
void HidKeyboard<LayoutType>::typingTimerHandler(btstack_timer_source_t* ts) {
    // --- FIX: No longer needs a cast. Forwards the call polymorphically. ---
    if (m_input_controller) {
        m_input_controller->typingTimerHandler(ts);
    }
}
#endif

template class HidKeyboard<USKeyboardLayout>;