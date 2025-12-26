#include "HidDevice.h"
#include <cstdio>
#include <cstring>
#include "btstack.h"

HidDevice::HidDevice() : m_connection_handle(HCI_CON_HANDLE_INVALID) {}

bool HidDevice::isConnected() const { return m_connection_handle != HCI_CON_HANDLE_INVALID; }
void HidDevice::requestToSend() { if (isConnected()) hids_device_request_can_send_now_event(m_connection_handle); }
void HidDevice::disconnect() {
    if (isConnected()) {
        gap_disconnect(m_connection_handle);
    }
}
void HidDevice::setup() {
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
}
void HidDevice::powerOn() { hci_power_control(HCI_POWER_ON); }
void HidDevice::sendHidReport(const uint8_t* report, uint16_t size) { if (isConnected()) hids_device_send_input_report(m_connection_handle, report, size); }

void HidDevice::onReadyToSend_impl() {}
void HidDevice::onHidSubscribed_impl() {}
void HidDevice::onTypingTimer_impl(btstack_timer_source_t* ts) { (void)ts; }

void HidDevice::onReadyToSend() { onReadyToSend_impl(); }
void HidDevice::onHidSubscribed() { onHidSubscribed_impl(); }
void HidDevice::typingTimerHandler(btstack_timer_source_t* ts) { onTypingTimer_impl(ts); }

void HidDevice::handlePacket(uint8_t packet_type, uint16_t, uint8_t* packet, uint16_t) {
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
                    if (isConnected()) onHidSubscribed();
                    break;
                case HIDS_SUBEVENT_CAN_SEND_NOW:
                    onReadyToSend();
                    break;
                default: break;
            }
            break;
        default: break;
    }
}