#ifndef MODULES_PROVISIONING_MODULE_H
#define MODULES_PROVISIONING_MODULE_H

#include "../core/Module.h"
#include "../core/HCIController.h"
// #include "../core/L2CAPServer.h"
#include "../proto/ATT.h"
#include <map>
#include <chrono>
#include <span>

namespace ble {

struct Attribute {
    uint16_t handle;
    UUID type;
    std::vector<uint8_t> value;
    uint8_t permissions; // 0=Read, 1=Write, 2=RW
};

class ProvisioningModule : public Module {
public:
    explicit ProvisioningModule(HCIController& hci);
    ~ProvisioningModule() override;
    
    void on_le_meta_event(uint8_t subevent_code, std::span<const uint8_t> data) override;
    void on_acl_data(std::span<const uint8_t> data) override;
    void on_disconnect(uint16_t handle, uint8_t reason) override;
    void process() override;
    
    void start_advertising();
    void stop_advertising();
    void save_and_reboot();

private:
    HCIController& hci_;
    uint16_t conn_handle_ = 0;
    std::chrono::steady_clock::time_point last_activity_;
    std::string stored_ssid_;
    std::string stored_pass_;
    std::string stored_loc_;
    std::string write_buffer_; // For reassembling fragmented writes
    // L2CAPServer l2cap_; // Removed
    std::map<uint16_t, Attribute> db_;
    
    void setup_gatt_db();
    
    void process_att_packet(int client_fd, std::vector<uint8_t>& packet);
    
    // Commands
    void handle_read_by_group(int client_fd, const uint8_t* data, size_t len);
    void handle_read_by_type(int client_fd, const uint8_t* data, size_t len);
    void handle_write_req(int client_fd, const uint8_t* data, size_t len);
    
    void send_error(int client_fd, uint8_t opcode, uint16_t handle, uint8_t ecode);
    void send_response(int client_fd, const std::vector<uint8_t>& resp);
};

} // namespace ble

#endif // MODULES_PROVISIONING_MODULE_H
