#ifndef CORE_HCI_CONTROLLER_H
#define CORE_HCI_CONTROLLER_H

#include "compat/hci.h"
#include "Module.h"
#include <string>
#include <vector>
#include <functional>
#include <span>

namespace ble {

class HCIController {
public:
    HCIController();
    ~HCIController();

    // Prevent copying
    HCIController(const HCIController&) = delete;
    HCIController& operator=(const HCIController&) = delete;

    void open_device(int dev_id = 0);
    bool attach_uart(const std::string& tty_dev);
    void start_scan();
    void stop_scan();

    int get_fd() const { return hci_fd_; }
    
    // Reads from socket and dispatches events. Non-blocking if socket is set so.
    void process_events();

    void add_module(Module* module);
    
    void send_command(uint16_t opcode, std::span<const uint8_t> params);
    void send_acl(uint16_t handle, std::span<const uint8_t> l2cap_pdu);

private:
    int hci_fd_ = -1;
    std::vector<Module*> modules_;

    void set_filter();
};

} // namespace ble

#endif // CORE_HCI_CONTROLLER_H
