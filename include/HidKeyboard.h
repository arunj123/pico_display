#ifndef HID_KEYBOARD_H
#define HID_KEYBOARD_H

#include "HidDevice.h"
#include "KeyboardLayout.h"
#include "HidReport.h"

class I_InputController;

template <typename LayoutType>
class HidKeyboard : public HidDevice {
public:
    void setup() override;
    void setInputController(I_InputController* controller);
    
    // Public API for controllers
    bool charToKeypress(char character, Keypress& out_key) const;
    void sendReport(const HidReport& report);

protected:
    // Override the implementation hooks from HidDevice
    void onReadyToSend_impl() override;
    void onHidSubscribed_impl() override;
    void onTypingTimer_impl(btstack_timer_source_t* ts) override;

    const uint8_t* getHidDescriptor() const override;
    uint16_t getHidDescriptorSize() const override;
    const uint8_t* getAdvertisingData() const override;
    uint16_t getAdvertisingDataSize() const override;

private:
    LayoutType m_layout;
    I_InputController* m_input_controller = nullptr;
};

extern template class HidKeyboard<USKeyboardLayout>;
#endif // HID_KEYBOARD_H