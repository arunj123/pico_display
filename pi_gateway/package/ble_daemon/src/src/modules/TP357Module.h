#ifndef MODULES_TP357_MODULE_H
#define MODULES_TP357_MODULE_H

#include "../core/Module.h"
#include <string>
#include <vector>
#include <array>
#include <map>

namespace ble {

class UDSManager; // Forward declaration

struct TP357Device {
    std::string name;
    std::array<uint8_t, 6> address;
};

class TP357Module : public Module {
public:
    struct SensorState {
        float temp;
        int hum;
        int rssi;
        uint32_t timestamp; // Seconds since boot (approx) or absolute
    };

    explicit TP357Module(UDSManager* uds = nullptr);
    
    void on_le_meta_event(uint8_t subevent_code, std::span<const uint8_t> data) override;

    void dump_state_to_client(int client_fd);

private:
    void handle_advertising_report(std::span<const uint8_t> data);
    std::string to_json(const std::string& name, const SensorState& state);
    
    UDSManager* uds_;
    std::map<std::array<uint8_t, 6>, TP357Device> devices_;
    std::map<std::string, SensorState> states_;
};

} // namespace ble

#endif // MODULES_TP357_MODULE_H
