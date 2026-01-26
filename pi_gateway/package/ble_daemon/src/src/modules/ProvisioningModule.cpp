#include "ProvisioningModule.h"
#include "../core/compat/hci.h"
#include "../core/compat/l2cap.h"
#include "../util/BleUtil.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <cstdlib>
#include <iomanip>

namespace ble {

// 128-bit UUIDs
static const auto SERVICE_UUID_BYTES = util::uuid::from_string("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static const auto WIFI_CHAR_UUID_BYTES = util::uuid::from_string("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static const auto LOC_CHAR_UUID_BYTES = util::uuid::from_string("0000ff02-0000-1000-8000-00805f9b34fb");

// Track alignment offset for ACL packets (0 or 1)
static int g_acl_alignment_offset = 0;
// Track TX handle separately for RPi Zero 2W Split-Handle Quirk


ProvisioningModule::ProvisioningModule(HCIController& hci) : hci_(hci) {
    // Load persisted state on startup
    // 1. Wi-Fi Config (Read from /tmp, populated by Init Script)
    std::ifstream wifi("/tmp/wifi.conf");
    if (wifi.is_open()) {
        std::string line;
        while (std::getline(wifi, line)) {
            // Simple parsing for ssid="value" and psk="value"
            size_t ssid_pos = line.find("ssid=\"");
            if (ssid_pos != std::string::npos) {
                size_t end = line.find("\"", ssid_pos + 6);
                if (end != std::string::npos) {
                    stored_ssid_ = line.substr(ssid_pos + 6, end - (ssid_pos + 6));
                    // std::cout << "[PROV] Loaded SSID: " << stored_ssid_ << std::endl;
                }
            }
            size_t psk_pos = line.find("psk=\"");
            if (psk_pos != std::string::npos) {
                size_t end = line.find("\"", psk_pos + 5);
                if (end != std::string::npos) {
                    stored_pass_ = line.substr(psk_pos + 5, end - (psk_pos + 5));
                    // std::cout << "[PROV] Loaded PSK: " << (stored_pass_.empty() ? "Empty" : "***") << std::endl;
                }
            }
        }
    }
    
    // 2. Config/Location from config.json (Read from /tmp)
    std::ifstream loc("/tmp/config.json");
    if (loc.is_open()) {
        std::string content((std::istreambuf_iterator<char>(loc)), std::istreambuf_iterator<char>());
        stored_loc_ = content;
        // std::cout << "[PROV] Loaded Config: " << stored_loc_ << std::endl;
    }
    


    setup_gatt_db();
}

ProvisioningModule::~ProvisioningModule() {
}



void ProvisioningModule::on_le_meta_event(uint8_t subevent_code, std::span<const uint8_t> data) {
    if (subevent_code == 0x01 || subevent_code == 0x0A) { // Connection Complete
        if (data.size() < 3) return;
        uint8_t status = data[0];
        if (status == 0) {
             uint16_t handle = data[1] | (data[2] << 8);
             conn_handle_ = handle;
             conn_handle_ = handle;
             g_acl_alignment_offset = 0; // Reset ACL offset
             last_activity_ = std::chrono::steady_clock::now();
             std::cout << "[PROV] Connected! Handle: 0x" << std::hex << handle << std::dec << std::endl;
             stop_advertising(); 
        }
    }
    else if (subevent_code == 0x03) { // LE Connection Update Complete
         if (data.size() >= 3) {
             uint8_t status = data[0];
             uint16_t handle = data[1] | (data[2] << 8);
             
             // std::cout << "[PROV] Event 0x03 (" << data.size() << ") Raw: ";
             // for(auto b : data) std::cout << std::hex << (int)b << " ";
             // std::cout << std::dec << "| Curr H=0x" << std::hex << conn_handle_ << " New H=0x" << handle << std::dec << std::endl;
             
             // Detect Shift in Event 0x03 (if present)
             bool shifted = false;
             if (data.size() >= 4 && data[0] == 0x00 && data[1] == 0x00) {
                 uint16_t shifted_handle = data[2] | (data[3] << 8);
                 // Only switch if unshifted looks wrong (high byte) and shifted looks reasonable
                 if ((handle & 0xFF00) && !(shifted_handle & 0xFF00)) {
                     handle = shifted_handle;
                     status = data[1];
                     shifted = true;
                 }
             }

             if (status == 0) {
                 if (conn_handle_ != 0 && handle != conn_handle_) {
                     std::cout << "[PROV] Controller ConnUpdate reported H=0x" << std::hex 
                               << handle << " (Old: 0x" << conn_handle_ << "). UPDATING HANDLE." << std::dec << std::endl;
                     conn_handle_ = handle;
                 }
             }
         }
    }
    else if (subevent_code == 0x04) { // LE Remote Connection Parameter Request
         if (data.size() < 2) return;
         
         // std::cout << "[PROV] Event 0x04Raw: ";
         // for(auto b : data) std::cout << std::hex << (int)b << " ";
         // std::cout << std::dec << "| Curr H=0x" << std::hex << conn_handle_ << std::dec << std::endl;
         
         int offset = 0;
         // Heuristic: If size is >= 11 (expected 10), it's shifted.
         if (data.size() >= 11) {
             if (data[1] == (conn_handle_ & 0xFF)) {
                 std::cout << "[PROV] Event 0x04 Shift Detected." << std::endl;
                 offset = 1;
             }
         }
         
         uint16_t handle = data[0+offset] | (data[1+offset] << 8);
         
         // Check if Controller has shifted the handle (e.g. 0x40 -> 0x64)
         if (conn_handle_ != 0 && handle != conn_handle_) {
             std::cout << "[PROV] Handle Shift in Param Req: 0x" << std::hex << conn_handle_ << " -> 0x" << handle << std::dec << ". Updating." << std::endl;
             conn_handle_ = handle;
         }
         
         if (data.size() >= 10 + offset) {
             uint16_t min_int = data[2+offset] | (data[3+offset] << 8);
             uint16_t max_int = data[4+offset] | (data[5+offset] << 8);
             uint16_t latency = data[6+offset] | (data[7+offset] << 8);
             uint16_t timeout = data[8+offset] | (data[9+offset] << 8);
             
             // Force Stable Connection Parameters (Windows/Chromium friendly: 25-50ms)
             min_int = 20; // 25ms
             max_int = 40; // 50ms
             latency = 0;
             timeout = 200; // 2000ms

             std::cout << "[PROV] Accepting Conn Params (Stable): Min=" << min_int << " Max=" << max_int << " Lat=" << latency << " TO=" << timeout << std::endl;
             
             // 0x2020 = HCI_LE_Remote_Connection_Parameter_Request_Reply
             std::vector<uint8_t> cp(14);
             cp[0] = handle & 0xFF; cp[1] = (handle >> 8) & 0xFF;
             cp[2] = min_int & 0xFF; cp[3] = (min_int >> 8) & 0xFF;
             cp[4] = max_int & 0xFF; cp[5] = (max_int >> 8) & 0xFF;
             cp[6] = latency & 0xFF; cp[7] = (latency >> 8) & 0xFF;
             cp[8] = timeout & 0xFF; cp[9] = (timeout >> 8) & 0xFF;
             cp[10] = 0x00; cp[11] = 0x00; // CE Min
             cp[12] = 0x00; cp[13] = 0x00; // CE Max
             
             hci_.send_command(cmd_opcode_pack(0x08, 0x0020), cp); 
         }
    }
}


void ProvisioningModule::process() {
    auto now = std::chrono::steady_clock::now();
    
    // Heartbeat to prove life (every 2s)
    static auto last_print = now;
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_print).count() >= 2) {
        last_print = now;
        if (conn_handle_ != 0) std::cout << "." << std::flush; 
    }

    if (conn_handle_ != 0) {
        auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - last_activity_).count();
        if (dur > 300) { 
            std::cout << "\n[PROV] Watchdog Timeout! Disconnecting..." << std::endl;
            conn_handle_ = 0;
            start_advertising();
        }
    }
}

void ProvisioningModule::on_disconnect(uint16_t handle, uint8_t reason) {
    if (handle == conn_handle_) {
        std::cout << "\n[PROV] Disconnected. Reason: 0x" << std::hex << (int)reason << std::dec << std::endl;
        conn_handle_ = 0x0000;
        start_advertising();
    }
}

void ProvisioningModule::on_acl_data(std::span<const uint8_t> data) {
    if (data.size() < 4) return;

    size_t offset = g_acl_alignment_offset;
    
    // Auto-detect shift if not locked yet
    if (offset == 0 && conn_handle_ != 0) {
        uint16_t handle = (data[0] | (data[1] << 8)) & 0x0FFF;
        if (handle != conn_handle_) {
            if (data.size() >= 5 && data[0] == 0x00) {
                 uint16_t potential = (data[1] | (data[2] << 8)) & 0x0FFF;
                 if (potential == conn_handle_) {
                      std::cout << "\n[PROV] ACL Shift Detected." << std::endl;
                      g_acl_alignment_offset = 1;
                      offset = 1;
                 }
            }
        }
    }

    last_activity_ = std::chrono::steady_clock::now();
    
    if (data.size() < 4 + offset) return;
    
    uint16_t dlen = data[2 + offset] | (data[3 + offset] << 8);
    
    if (data.size() < 4 + offset + dlen) {
        // Log mismatch to debug
        // std::cout << "[PROV] ACL Len Mismatch. Need " << (4+offset+dlen) << " Got " << data.size() << std::endl;
        return;
    }
    
    std::span<const uint8_t> l2cap_pkt = data.subspan(4 + offset, dlen);
    if (l2cap_pkt.size() < 4) return;
    
    uint16_t sig = l2cap_pkt[2] | (l2cap_pkt[3] << 8);
    uint16_t cid = sig; // For clarity

    // Log all incoming L2CAP/ATT traffic for debugging
    if (cid == L2CAP_CID_ATT) {
        // std::cout << "[PROV] RX ATT Packet. Op: " << std::hex << (int)l2cap_pkt[4] << std::dec << std::endl;
    } else {
        // std::cout << "[PROV] RX L2CAP CID: 0x" << std::hex << cid << " Len: " << l2cap_pkt.size() << std::dec << std::endl;
    }
    
    if (cid == L2CAP_CID_ATT) { // 4
        std::vector<uint8_t> att_payload(l2cap_pkt.begin() + 4, l2cap_pkt.end());
        process_att_packet(0, att_payload);
    }
    else if (cid == 0x0005) { // L2CAP Signaling
        if (l2cap_pkt.size() >= 5) {
            uint8_t sig_op = l2cap_pkt[4];
            uint8_t sig_id = l2cap_pkt[5];
            
            if (sig_op == 0x12) { // Connection Parameter Update Request
                std::vector<uint8_t> resp = { 0x06, 0x00, 0x05, 0x00, 0x13, sig_id, 0x02, 0x00, 0x00, 0x00 };
                hci_.send_acl(conn_handle_, resp);
            } else {
                std::vector<uint8_t> resp = { 0x06, 0x00, 0x05, 0x00, 0x01, sig_id, 0x02, 0x00, 0x00, 0x00 };
                hci_.send_acl(conn_handle_, resp);
            }
        }
    }
}

// ---------------------- GATT DATABASE ----------------------

std::vector<uint8_t> get_uuid_for_handle(uint16_t handle) {
    if (handle == 0x0001 || handle == 0x0010 || handle == 0x0020 || handle == 0x0028 || handle == 0x0030) return {0x00, 0x28}; // Primary Service
    if (handle == 0x0002 || handle == 0x0004 || handle == 0x0011 || handle == 0x0013 || handle == 0x0021 || handle == 0x0029 || handle == 0x0031 || handle == 0x0033) return {0x03, 0x28}; // Char Decl
    if (handle == 0x0003) return {0x00, 0x2A}; // Device Name
    if (handle == 0x0005) return {0x01, 0x2A}; // Appearance
    if (handle == 0x0012) return {0x00, 0x2A}; 
    if (handle == 0x0014) return {0x01, 0x2A}; 
    if (handle == 0x0022) return {0x24, 0x2A}; 
    if (handle == 0x002A) return {0x19, 0x2A}; 
    if (handle == 0x0032) return std::vector<uint8_t>(WIFI_CHAR_UUID_BYTES.begin(), WIFI_CHAR_UUID_BYTES.end());
    if (handle == 0x0034) return std::vector<uint8_t>(LOC_CHAR_UUID_BYTES.begin(), LOC_CHAR_UUID_BYTES.end());
    return {};
}

void ProvisioningModule::process_att_packet(int fd, std::vector<uint8_t>& p) {
    if (p.empty()) return;
    uint8_t opcode = p[0];
    
    // std::cout << "[PROV] RX Op: 0x" << std::hex << (int)opcode << " Len: " << p.size() << " Data: ";
    // for(auto b : p) std::cout << std::hex << (int)b << " ";
    // std::cout << std::dec << std::endl;
    
    if (opcode == ATT_OP_EXCHANGE_MTU_REQ) {
        uint16_t server_mtu = 128; 
        uint8_t resp[] = {ATT_OP_EXCHANGE_MTU_RESP, (uint8_t)(server_mtu & 0xFF), (uint8_t)(server_mtu >> 8)};
        send_response(fd, {resp[0], resp[1], resp[2]});
    }
    else if (opcode == 0x06) { // FIND_BY_TYPE_VALUE_REQ
        uint16_t start_h = p[1] | (p[2]<<8);
        uint16_t type = p[5] | (p[6]<<8); 
        std::vector<uint8_t> resp; resp.push_back(0x07);
        bool match = false;

        if (type == 0x2800) { 
            if (p.size() == 9) { // 16-bit
                 uint16_t uuid16 = p[7] | (p[8]<<8);
                 if (uuid16 == 0x1801 && start_h <= 0x0001) { resp.push_back(0x01); resp.push_back(0x00); resp.push_back(0x07); resp.push_back(0x00); match=true; } 
                 else if (uuid16 == 0x1800 && start_h <= 0x0010) { resp.push_back(0x10); resp.push_back(0x00); resp.push_back(0x17); resp.push_back(0x00); match=true; }
            } else if (p.size() == 23) { // 128-bit
                std::vector<uint8_t> req(p.begin()+7, p.end());
                std::vector<uint8_t> my(SERVICE_UUID_BYTES.begin(), SERVICE_UUID_BYTES.end());
                if (req == my && start_h <= 0x0030) { resp.push_back(0x30); resp.push_back(0x00); resp.push_back(0x3F); resp.push_back(0x00); match=true; }
            }
        }
        if (match) send_response(fd, resp);
        else send_error(fd, opcode, start_h, 0x0A);
    }
    else if (opcode == ATT_OP_READ_GROUP_REQ) { // Discover Services
        uint16_t start_h = p[1] | (p[2]<<8);
        std::vector<uint8_t> resp; resp.push_back(ATT_OP_READ_GROUP_RESP);

        if (start_h <= 0x0001) { resp.push_back(6); resp.push_back(0x01); resp.push_back(0x00); resp.push_back(0x05); resp.push_back(0x00); resp.push_back(0x00); resp.push_back(0x18); send_response(fd, resp); return; }
        if (start_h <= 0x0010) { resp.push_back(6); resp.push_back(0x10); resp.push_back(0x00); resp.push_back(0x14); resp.push_back(0x00); resp.push_back(0x00); resp.push_back(0x18); send_response(fd, resp); return; }
        if (start_h <= 0x0020) { resp.push_back(6); resp.push_back(0x20); resp.push_back(0x00); resp.push_back(0x22); resp.push_back(0x00); resp.push_back(0x0A); resp.push_back(0x18); send_response(fd, resp); return; }
        if (start_h <= 0x0028) { resp.push_back(6); resp.push_back(0x28); resp.push_back(0x00); resp.push_back(0x2A); resp.push_back(0x00); resp.push_back(0x0F); resp.push_back(0x18); send_response(fd, resp); return; }
        if (start_h <= 0x0030) { resp.push_back(20); resp.push_back(0x30); resp.push_back(0x00); resp.push_back(0x34); resp.push_back(0x00); resp.insert(resp.end(), SERVICE_UUID_BYTES.begin(), SERVICE_UUID_BYTES.end()); send_response(fd, resp); return; }
        send_error(fd, opcode, start_h, 0x0A); 
    }
    else if (opcode == ATT_OP_READ_BY_TYPE_REQ) { // Discover Characteristics
        uint16_t start_h = p[1] | (p[2]<<8);
        uint16_t end_h = p[3] | (p[4]<<8);
        if (p.size() >= 7) { uint16_t type = p[5] | (p[6]<<8); if (type != 0x2803) { send_error(fd, opcode, start_h, 0x0A); return; } }

        std::vector<uint8_t> resp; resp.push_back(ATT_OP_READ_BY_TYPE_RESP);

        if (start_h <= 0x0002 && end_h >= 0x0002) { resp.push_back(7); resp.push_back(0x02); resp.push_back(0x00); resp.push_back(0x02); resp.push_back(0x03); resp.push_back(0x00); resp.push_back(0x00); resp.push_back(0x2a); send_response(fd, resp); return; }
        if (start_h <= 0x0004 && end_h >= 0x0004) { resp.push_back(7); resp.push_back(0x04); resp.push_back(0x00); resp.push_back(0x02); resp.push_back(0x05); resp.push_back(0x00); resp.push_back(0x01); resp.push_back(0x2a); send_response(fd, resp); return; }

        if (start_h <= 0x0011 && end_h >= 0x0011) { resp.push_back(7); resp.push_back(0x11); resp.push_back(0x00); resp.push_back(0x02); resp.push_back(0x12); resp.push_back(0x00); resp.push_back(0x00); resp.push_back(0x2a); send_response(fd, resp); return; }
        if (start_h <= 0x0013 && end_h >= 0x0013) { resp.push_back(7); resp.push_back(0x13); resp.push_back(0x00); resp.push_back(0x02); resp.push_back(0x14); resp.push_back(0x00); resp.push_back(0x01); resp.push_back(0x2a); send_response(fd, resp); return; }
        if (start_h <= 0x0021 && end_h >= 0x0021) { resp.push_back(7); resp.push_back(0x21); resp.push_back(0x00); resp.push_back(0x02); resp.push_back(0x22); resp.push_back(0x00); resp.push_back(0x24); resp.push_back(0x2a); send_response(fd, resp); return; }
        if (start_h <= 0x0029 && end_h >= 0x0029) { resp.push_back(7); resp.push_back(0x29); resp.push_back(0x00); resp.push_back(0x02); resp.push_back(0x2A); resp.push_back(0x00); resp.push_back(0x19); resp.push_back(0x2a); send_response(fd, resp); return; }
        if (start_h <= 0x0031 && end_h >= 0x0031) { resp.push_back(21); resp.push_back(0x31); resp.push_back(0x00); resp.push_back(0x0E); resp.push_back(0x32); resp.push_back(0x00); resp.insert(resp.end(), WIFI_CHAR_UUID_BYTES.begin(), WIFI_CHAR_UUID_BYTES.end()); send_response(fd, resp); return; }
        if (start_h <= 0x0033 && end_h >= 0x0033) { resp.push_back(21); resp.push_back(0x33); resp.push_back(0x00); resp.push_back(0x02); resp.push_back(0x34); resp.push_back(0x00); resp.insert(resp.end(), LOC_CHAR_UUID_BYTES.begin(), LOC_CHAR_UUID_BYTES.end()); send_response(fd, resp); return; }
        send_error(fd, opcode, start_h, 0x0A);
    }
    else if (opcode == ATT_OP_FIND_INFO_REQ) { 
         uint16_t start_h = p[1] | (p[2]<<8);
         uint16_t end_h = p[3] | (p[4]<<8);
         std::vector<uint8_t> resp;
         uint16_t found_handle = 0; std::vector<uint8_t> found_uuid;

         for (uint16_t h = start_h; h <= end_h && h <= 0x0040; h++) {
             std::vector<uint8_t> u = get_uuid_for_handle(h);
             if (!u.empty()) { found_handle = h; found_uuid = u; break; }
         }

         if (found_handle != 0) {
             resp.push_back(0x05); 
             if (found_uuid.size() == 2) { resp.push_back(0x01); resp.push_back(found_handle & 0xFF); resp.push_back(found_handle >> 8); resp.push_back(found_uuid[0]); resp.push_back(found_uuid[1]); } 
             else { resp.push_back(0x02); resp.push_back(found_handle & 0xFF); resp.push_back(found_handle >> 8); resp.insert(resp.end(), found_uuid.begin(), found_uuid.end()); }
             send_response(fd, resp);
         } else { send_error(fd, opcode, start_h, 0x0A); }
    }
    else if (opcode == ATT_OP_READ_REQ) { 
        uint16_t handle = p[1] | (p[2]<<8);
        if (handle == 0x0003) { std::string val = "Pi Gateway"; std::vector<uint8_t> resp(1 + val.size()); resp[0] = ATT_OP_READ_RESP; memcpy(&resp[1], val.c_str(), val.size()); send_response(fd, resp); }
        else if (handle == 0x0005) { uint8_t val[] = {ATT_OP_READ_RESP, 0x00, 0x00}; send_response(fd, {val[0], val[1], val[2]}); }
        else if (handle == 0x0012) { std::string val = "Gateway-Setup"; std::vector<uint8_t> resp(1 + val.size()); resp[0] = ATT_OP_READ_RESP; memcpy(&resp[1], val.c_str(), val.size()); send_response(fd, resp); }
        else if (handle == 0x0014) { uint8_t val[] = {ATT_OP_READ_RESP, 0x00, 0x00}; send_response(fd, {val[0], val[1], val[2]}); }
        else if (handle == 0x0022) { std::string val = "PiGateway"; std::vector<uint8_t> resp(1 + val.size()); resp[0] = ATT_OP_READ_RESP; memcpy(&resp[1], val.c_str(), val.size()); send_response(fd, resp); }
        else if (handle == 0x002A) { uint8_t val[] = {ATT_OP_READ_RESP, 100}; send_response(fd, {val[0], val[1]}); }
        else if (handle == 0x0032) { // SSID
             std::string val = stored_ssid_;
             if (val.empty()) val = "";
             std::vector<uint8_t> resp; resp.push_back(ATT_OP_READ_RESP);
             resp.insert(resp.end(), val.begin(), val.end());
             send_response(fd, resp);
        }
        else if (handle == 0x0034) { // Location/Config
             std::string val = stored_loc_;
             if (val.empty()) val = "{}";
             // Chunking support is not implemented for READ in this simple daemon
             // If > 22 bytes (MTU), client will only get first chunk.
             if (val.length() > 500) val = val.substr(0, 500); // Guard
             
             std::vector<uint8_t> resp; resp.push_back(ATT_OP_READ_RESP);
             resp.insert(resp.end(), val.begin(), val.end());
             send_response(fd, resp);
        }
        else { send_error(fd, opcode, handle, 0x0A); }
    }
    else if (opcode == ATT_OP_WRITE_REQ || opcode == ATT_OP_WRITE_CMD) {
        uint16_t handle = p[1] | (p[2]<<8);
        if (handle == 0x0032 || handle == 0x0034) { handle_write_req(fd, p.data(), p.size()); } 
        else if (handle == 0x0004) { if (opcode == ATT_OP_WRITE_REQ) { uint8_t resp[] = {ATT_OP_WRITE_RESP}; send_response(fd, {resp[0]}); } }
        else { if (opcode == ATT_OP_WRITE_REQ) send_error(fd, opcode, handle, 0x0A); }
    }
    else { send_error(fd, opcode, 0, 0x06); }
}

void ProvisioningModule::setup_gatt_db() {}

void ProvisioningModule::handle_write_req(int client_fd, const uint8_t* data, size_t len) {
    if (len > 3) {
        std::string payload((const char*)data + 3, len - 3);
        std::cout << "[PROV] RX: " << payload << std::endl;
        
        // Append to buffer for potential fragmentation
        write_buffer_ += payload;

        // Check for Simple Commands first (not JSON)
        if (payload.rfind("S:", 0) == 0) { 
             stored_ssid_ = payload.substr(2); 
             write_buffer_.clear(); 
        }
        else if (payload.rfind("P:", 0) == 0) { 
             stored_pass_ = payload.substr(2); 
             write_buffer_.clear(); 
        }
        else if (payload == "SAVE") { 
             save_and_reboot(); 
             write_buffer_.clear(); 
        }
        else if (payload == "EXIT") { 
             hci_.stop_scan(); 
             write_buffer_.clear(); 
        }
        // Check for JSON start
        else if (write_buffer_.rfind("{", 0) == 0) {
             // Heuristic: If it starts with { and ends with }, assume complete.
             // This supports simple non-nested JSON objects like {"name":"X","lat":1,"lon":2}
             // For more complex stream parsing, a json parser is needed, but this suffices for known client.
             
             // Trim potential garbage/whitespace at end?
             // No, just check if last char is '}'
             if (write_buffer_.size() > 2 && write_buffer_.back() == '}') {
                 std::cout << "[PROV] JSON Complete: " << write_buffer_ << std::endl;
                 stored_loc_ = write_buffer_;
                 write_buffer_.clear();
             } else {
                 std::cout << "[PROV] JSON Fragment Buffered (" << write_buffer_.size() << " bytes)..." << std::endl;
             }
        }
        else {
             // Unknown/Garbage? Clear buffer to prevent stale state
             // Or maybe it's a middle fragment we missed the start of?
             // For now, if it doesn't match known prefixes and buffer is empty, it's trash.
             if (write_buffer_.length() == payload.length()) { // Buffer was just this payload
                  std::cout << "[PROV] Unknown Command received." << std::endl;
                  write_buffer_.clear();
             }
        }
    }
    if (data[0] == ATT_OP_WRITE_REQ) { send_response(client_fd, {ATT_OP_WRITE_RESP}); }
}

void ProvisioningModule::save_and_reboot() {
    std::cout << "[PROV] Config State Check:" << std::endl;
    std::cout << " SSID: '" << stored_ssid_ << "'" << std::endl;
    std::cout << " LOC:  '" << stored_loc_ << "'" << std::endl;

    if (stored_ssid_.empty() && stored_loc_.empty()) {
        std::cout << "[PROV] Nothing to save!" << std::endl;
        return;
    }
    
    std::cout << "[PROV] Saving Configuration..." << std::endl;
    
    // RELEASE USB LOCK: Unbind mass storage so checking mount works
    // This is critical if the user is powered via PC/USB
    std::cout << "[PROV] Releasing USB Mass Storage Lock..." << std::endl;
    // We try to unbind the backing file from the gadget
    system("echo \"\" > /sys/kernel/config/usb_gadget/mygadget/functions/mass_storage.usb0/lun.0/file");
    sleep(1);

    // Hardening: Ensure we can write to /mnt/data
    system("mount -o remount,rw /"); // Make root RW if needed
    system("mkdir -p /mnt/data"); 
    system("mount /dev/mmcblk0p3 /mnt/data"); // Try mount
    system("mount -o remount,rw /mnt/data"); // Ensure RW if already mounted
    
    bool saved = false;

    if (!stored_ssid_.empty()) {
        std::cout << "Saving Wi-Fi Config for SSID: " << stored_ssid_ << std::endl;
        std::ofstream wifi_file("/mnt/data/wifi.conf");
        if (wifi_file.is_open()) { wifi_file << "country=DE\nupdate_config=1\n\nnetwork={\n\tssid=\"" << stored_ssid_ << "\"\n\tpsk=\"" << stored_pass_ << "\"\n\tkey_mgmt=WPA-PSK\n}\n"; wifi_file.close(); saved=true; }
        else { std::cerr << "[PROV] Failed to open wifi.conf for writing!" << std::endl; }
    }

    if (!stored_loc_.empty()) {
        std::cout << "Saving Location/Config Data..." << std::endl;
        std::ofstream loc_file("/mnt/data/config.json");
        if (loc_file.is_open()) { loc_file << stored_loc_; loc_file.close(); saved=true; }
        else { std::cerr << "[PROV] Failed to open config.json for writing!" << std::endl; }
    }

    system("sync"); 
    // Do not unmount, just reboot to be safe.
    
    if (saved) {
        std::cout << "Cleaning seed.credit and Rebooting..." << std::endl;
        system("rm -f /var/lib/seedrng/seed.credit");
        system("reboot");
    } else {
        std::cerr << "[PROV] Save failed or nothing to save." << std::endl;
        // Force reboot anyway if we attempted? No, only if saved.
    }
}

void ProvisioningModule::start_advertising() {
    std::cout << "[PROV] Starting Advertising..." << std::endl;
    uint8_t params[] = { 0xA0, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00 };
    hci_.send_command(cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_ADVERTISING_PARAMETERS), params);
    uint8_t payload[] = { 0x02, 0x01, 0x06, 0x0E, 0x09, 'G', 'a', 't', 'e', 'w', 'a', 'y', '-', 'S', 'e', 't', 'u', 'p' };
    uint8_t data[32] = {0}; data[0] = sizeof(payload); memcpy(&data[1], payload, sizeof(payload));
    hci_.send_command(cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_ADVERTISING_DATA), data);
    uint8_t enable[] = {0x01}; hci_.send_command(cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_ADVERTISE_ENABLE), enable);
}

void ProvisioningModule::stop_advertising() {
    std::cout << "[PROV] Stopping Advertising..." << std::endl;
    uint8_t disable[] = {0x00}; hci_.send_command(cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_ADVERTISE_ENABLE), disable);
}

void ProvisioningModule::send_response(int fd, const std::vector<uint8_t>& resp) {
    if (conn_handle_ == 0) return;
    uint16_t cid = L2CAP_CID_ATT;
    std::vector<uint8_t> packet;
    uint16_t len = resp.size();
    packet.push_back(len & 0xFF); packet.push_back((len >> 8) & 0xFF);
    packet.push_back(cid & 0xFF); packet.push_back((cid >> 8) & 0xFF);
    packet.insert(packet.end(), resp.begin(), resp.end());
    // std::cout << "[PROV] TX to H=0x" << std::hex << conn_handle_ << " Len=" << std::dec << resp.size() << " Data: ";
    // for(auto b : resp) std::cout << std::hex << (int)b << " ";
    // std::cout << std::dec << std::endl;
    hci_.send_acl(conn_handle_, packet);
}

void ProvisioningModule::send_error(int fd, uint8_t opcode, uint16_t handle, uint8_t ecode) {
    std::vector<uint8_t> err = {ATT_OP_ERROR_RESP, opcode, (uint8_t)(handle & 0xFF), (uint8_t)(handle >> 8), ecode};
    send_response(fd, err);
}

} // namespace ble