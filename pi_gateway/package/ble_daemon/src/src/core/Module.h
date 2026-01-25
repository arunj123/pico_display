#ifndef CORE_MODULE_H
#define CORE_MODULE_H

#include <vector>
#include <cstdint>
#include <span>

namespace ble {

class Module {
public:
    virtual ~Module() = default;
    
    // Called when an LE Meta Event (Advertising Report, etc) is received
    virtual void on_le_meta_event(uint8_t subevent_code, std::span<const uint8_t> data) = 0;
    
    // Called when ACL Data is received
    virtual void on_acl_data(std::span<const uint8_t> data) {} 
    
    // Called on Disconnection Complete
    virtual void on_disconnect(uint16_t handle, uint8_t reason) {}
    
    // Periodic processing (called every ~100ms)
    virtual void process() {}
};

} // namespace ble

#endif // CORE_MODULE_H
