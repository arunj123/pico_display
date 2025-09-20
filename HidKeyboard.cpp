#include "HidKeyboard.h"
#include "I_InputController.h"
#include "BleDescriptors.h"
#include "hog_keyboard_demo.h"
#include <cstring>

template<typename LayoutType>
void HidKeyboard<LayoutType>::setup() {
    HidDevice::setup();
    att_server_init(profile_data, nullptr, nullptr);
    hids_device_init(0, getHidDescriptor(), getHidDescriptorSize());
    battery_service_server_init(100);
    device_information_service_server_init();
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(0x0030, 0x0030, 0, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(getAdvertisingDataSize(), (uint8_t*)getAdvertisingData());
    gap_advertisements_enable(1);
}

template<typename LayoutType>
void HidKeyboard<LayoutType>::setInputController(I_InputController* controller) {
    m_input_controller = controller;
}

template<typename LayoutType>
bool HidKeyboard<LayoutType>::charToKeypress(char character, Keypress& out_key) const {
    return m_layout.findKey(character, out_key);
}

template<typename LayoutType>
void HidKeyboard<LayoutType>::sendReport(const HidReport& report) {
    sendHidReport(report.data(), report.size());
}

template<typename LayoutType>
void HidKeyboard<LayoutType>::onReadyToSend_impl() {
    if (m_input_controller) m_input_controller->onReadyToSend();
}

template<typename LayoutType>
void HidKeyboard<LayoutType>::onHidSubscribed_impl() {
    if (m_input_controller) m_input_controller->onHidSubscribed();
}

template<typename LayoutType>
void HidKeyboard<LayoutType>::onTypingTimer_impl(btstack_timer_source_t* ts) {
    if (m_input_controller) m_input_controller->typingTimerHandler(ts);
}

template<typename LayoutType>
const uint8_t* HidKeyboard<LayoutType>::getHidDescriptor() const {
    return BleDescriptors::Keyboard::hid_descriptor;
}

template<typename LayoutType>
uint16_t HidKeyboard<LayoutType>::getHidDescriptorSize() const {
    return sizeof(BleDescriptors::Keyboard::hid_descriptor);
}

template<typename LayoutType>
const uint8_t* HidKeyboard<LayoutType>::getAdvertisingData() const {
    return BleDescriptors::Keyboard::advertising_data;
}

template<typename LayoutType>
uint16_t HidKeyboard<LayoutType>::getAdvertisingDataSize() const {
    return sizeof(BleDescriptors::Keyboard::advertising_data);
}

template class HidKeyboard<USKeyboardLayout>;