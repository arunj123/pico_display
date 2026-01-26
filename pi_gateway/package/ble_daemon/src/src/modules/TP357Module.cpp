#include "TP357Module.h"
#include "../core/compat/hci.h"
#include "../core/UDSManager.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace ble {

TP357Module::TP357Module(UDSManager* uds) : uds_(uds) {
    // Initialize known devices
    devices_[{0xf0, 0xe4, 0x4b, 0xf5, 0x76, 0xe2}] = {"Kindr", {0xf0, 0xe4, 0x4b, 0xf5, 0x76, 0xe2}};
    devices_[{0xf7, 0x73, 0x3c, 0x40, 0x2c, 0xce}] = {"Wohn ", {0xf7, 0x73, 0x3c, 0x40, 0x2c, 0xce}};
    devices_[{0x7c, 0xb8, 0xc9, 0xd2, 0xd5, 0xe9}] = {"Flur ", {0x7c, 0xb8, 0xc9, 0xd2, 0xd5, 0xe9}};
    devices_[{0x89, 0x84, 0x21, 0x8b, 0x50, 0xdf}] = {"Bad  ", {0x89, 0x84, 0x21, 0x8b, 0x50, 0xdf}};
    devices_[{0xbc, 0xc0, 0xfd, 0x85, 0x05, 0xd6}] = {"Kuche", {0xbc, 0xc0, 0xfd, 0x85, 0x05, 0xd6}};
    devices_[{0xf5, 0xe5, 0x62, 0x2b, 0x5f, 0xf8}] = {"Schlf", {0xf5, 0xe5, 0x62, 0x2b, 0x5f, 0xf8}};
}

void TP357Module::on_le_meta_event(uint8_t subevent_code, std::span<const uint8_t> data) {
    if (subevent_code == EVT_LE_ADVERTISING_REPORT) {
        handle_advertising_report(data);
    }
}

void TP357Module::handle_advertising_report(std::span<const uint8_t> data) {
    if (data.empty()) return;
    
    uint8_t num_reports = data[0];
    size_t offset = 1;
    
    for (int i = 0; i < num_reports; ++i) {
        if (offset + 10 > data.size()) break; // Header + Addr
        
        uint8_t evt_type = data[offset++];
        uint8_t addr_type = data[offset++];
        
        std::array<uint8_t, 6> addr;
        for (int k = 0; k < 6; ++k) addr[k] = data[offset++];
        
        uint8_t data_len = data[offset++];
        if (offset + data_len + 1 > data.size()) break; // Data + RSSI
        
        std::span<const uint8_t> adv_data = data.subspan(offset, data_len);
        offset += data_len;
        
        int8_t rssi = (int8_t)data[offset++];
        
        auto it = devices_.find(addr);
        if (it != devices_.end()) {
            const auto& dev = it->second;
            
            // TRY TO PARSE
            // TP357 Advertisement uses Manufacturer Specific Data (0xFF)
            // Format observed: 07 ff c2 [TempL] [TempH] [Hum] [??] [??]
            size_t p = 0;
            while (p + 2 <= adv_data.size()) {
                uint8_t len = adv_data[p];
                uint8_t type = adv_data[p+1];
                if (p + 1 + len > adv_data.size()) break;
                
                if (type == 0xFF && len >= 5) { // Manufacturer Data
                    std::span<const uint8_t> mdata = adv_data.subspan(p + 2, len - 1);
                    if (!mdata.empty() && mdata[0] == 0xC2) {
                        // Found TP357 Signature
                        if (mdata.size() >= 4) {
                            int16_t temperature = (int16_t)((uint8_t)mdata[1] | ((uint8_t)mdata[2] << 8));
                            uint8_t humidity = mdata[3];
                            
                            std::cout << "[TP357] " << dev.name 
                                      << " Temp: " << std::fixed << std::setprecision(1) << (temperature / 10.0) << "C"
                                      << " Hum: " << (int)humidity << "%"
                                      << " RSSI: " << (int)rssi << "dBm" << std::endl;

                            if (uds_) {
                                SensorState st = { (float)(temperature / 10.0), (int)humidity, (int)rssi, (uint32_t)time(nullptr) };
                                states_[dev.name] = st;
                                uds_->broadcast(to_json(dev.name, st));
                            }
                        }
                    }
                }
                p += len + 1;
            }
        }
    }
}

std::string TP357Module::to_json(const std::string& name, const SensorState& state) {
    std::stringstream json;
    json << "{\"type\":\"sensor\", \"name\":\"" << name 
         << "\", \"temp\":" << std::fixed << std::setprecision(1) << state.temp
         << ", \"hum\":" << state.hum 
         << ", \"rssi\":" << state.rssi 
         << ", \"ts\":" << state.timestamp << "}";
    return json.str();
}

void TP357Module::dump_state_to_client(int client_fd) {
    if (!uds_) return;
    for (const auto& [name, state] : states_) {
        uds_->send_to_client(client_fd, to_json(name, state));
    }
}

} // namespace ble
