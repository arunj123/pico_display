#include "HCIController.h"
#include "compat/bluetooth.h"
#include "compat/hci.h"
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <vector>
#include <array>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <poll.h>

std::string timestamp(); 

namespace ble {

HCIController::HCIController() {
}

HCIController::~HCIController() {
    if (hci_fd_ >= 0) {
        stop_scan(); // Try to be nice
        close(hci_fd_);
    }
}

void HCIController::add_module(Module* module) {
    modules_.push_back(module);
}

void HCIController::open_device(int dev_id) {
    // 1. Open raw HCI socket
    hci_fd_ = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);
    if (hci_fd_ < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to open HCI socket");
    }

    // 2. Bind to device
    struct sockaddr_hci addr;
    memset(&addr, 0, sizeof(addr));
    addr.hci_family = AF_BLUETOOTH;
    addr.hci_dev = dev_id;
    addr.hci_channel = HCI_CHANNEL_USER; // Try USER channel first which gives exclusive access? 
                                         // Or RAW channel. BlueZ uses RAW for tools usually. 
                                         // Let's use standard binding which defaults to RAW usually if not specific.
                                         // Wait, 'HCI_CHANNEL_USER' is for when you want to take over controller from kernel.
                                         // We probably want shared access if possible, or exclusive. 
                                         // Let's stick to standard bind for now.
    // addr.hci_channel = HCI_CHANNEL_RAW; 
    addr.hci_channel = HCI_CHANNEL_USER; // Exclusive (Requires DOWN)

    // Ensure DOWN for Exclusive User Channel
    if (ioctl(hci_fd_, HCIDEVDOWN, dev_id) < 0) {
        if (errno != EALREADY) {
            // perror("ioctl HCIDEVDOWN ignored/failed");
        }
    } else {
        std::cout << "Device put to DOWN state for Exclusive User Channel." << std::endl;
    }
    sleep(1); 

    if (bind(hci_fd_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
         // EBUSY is less likely on Raw channel unless exclusive lock exists
         throw std::system_error(errno, std::generic_category(), "Failed to bind HCI socket");
    }
    
    // HCI Reset to clean state
    {
         std::cout << "Sending HCI_RESET..." << std::endl;
         uint8_t cmd[] = {0x03, 0x0C, 0x00}; // Opcode 0x0C03 (Reset)
         // Direct Write because send_command adds header? 
         // My send_command adds header. 
         // OGF=3 (0x03), OCF=3 (0x03). 
         // Opcode = (3 << 10) | 3 = 0x0C03.
         send_command(0x0C03, {}); 
         sleep(1); // Wait for reset
    }

    // Skip HCIDEVUP for User Channel (managed manually/reset only)
    
    // Give it a moment to initialize
    sleep(1);

    // Force Stop Scan (in case it was left on)
    {
        le_set_scan_enable_cp scan_enable;
        scan_enable.enable = 0x00;
        scan_enable.filter_dup = 0x00;
        std::array<uint8_t, sizeof(scan_enable)> enable_buf;
        memcpy(enable_buf.data(), &scan_enable, sizeof(scan_enable));
        send_command(cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE), enable_buf);
    }
    
    // Read BD_ADDR
    {
        send_command(0x1009, {});
    }

    // 3. Skip Filter for User Channel (Not supported/Needed)
    if (addr.hci_channel != 1) { 
        set_filter();
    }
    
    std::cout << "HCI Device " << dev_id << " opened successfully." << std::endl;
}

bool HCIController::attach_uart(const std::string& tty_dev) {
    std::cout << "Attaching to UART: " << tty_dev << std::endl;
    
    int fd = open(tty_dev.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        perror("Failed to open UART TTY");
        return false;
    }

    // 1. Set line discipline to N_HCI
    int ldisc = N_HCI;
    if (ioctl(fd, TIOCSETD, &ldisc) < 0) {
        perror("Failed to set N_HCI line discipline");
        close(fd);
        return false;
    }

    // 2. Set HCI UART Protocol (BCM = 7 for Broadcom)
    // We assume bcm43xx usage which is standard for RPi
    int proto = HCI_UART_BCM;
    if (ioctl(fd, HCIUARTSETPROTO, &proto) < 0) {
        perror("Failed to set HCI UART protocol");
        close(fd);
        return false;
    }

    std::cout << "UART attached successfully via N_HCI." << std::endl;
    // We don't close fd! The kernel holds the line discipline as long as fd is open.
    // However, for HCI_CHANNEL_USER, we usually don't need to hold THIS fd if the driver creates hci0.
    // Wait, if we close this FD, the line discipline is lost and hci0 disappears.
    // So we must keep this FD open for the lifetime of the daemon.
    // We'll leak it intentionally or store it.
    // Let's store it in a static or member if we want to clean up, but for now leak/keep alive is fine
    // as long as daemon runs.
    
    // Allow a moment for hci0 to appear
    sleep(1);
    
    return true;
}

void HCIController::set_filter() {
    struct hci_filter nf;
    hci_filter_clear(&nf);
    hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
    hci_filter_set_ptype(HCI_ACLDATA_PKT, &nf); // Enable ACL Data!
    hci_filter_set_event(EVT_LE_META_EVENT, &nf);
    // Allow command status/complete events too if we want to track commands
    // hci_filter_set_event(EVT_CMD_STATUS, &nf); 
    // hci_filter_set_event(EVT_CMD_COMPLETE, &nf);
    // Enable them for debugging!
    hci_filter_set_event(0x0F, &nf); // Status
    hci_filter_set_event(0x0E, &nf); // Complete
    // Also Disconnect Complete (0x05)
    hci_filter_set_event(0x05, &nf);

    if (setsockopt(hci_fd_, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
        throw std::system_error(errno, std::generic_category(), "Failed to set HCI filter");
    }
}

void HCIController::send_command(uint16_t opcode, std::span<const uint8_t> params) {
    std::vector<uint8_t> buf(4 + params.size());
    
    buf[0] = HCI_COMMAND_PKT;
    buf[1] = opcode & 0xFF;
    buf[2] = (opcode >> 8) & 0xFF;
    buf[3] = (uint8_t)params.size();

    if (!params.empty()) {
        memcpy(buf.data() + 1 + sizeof(hci_cmd_hdr), params.data(), params.size());
    }

    if (write(hci_fd_, buf.data(), buf.size()) < 0) {
         perror("Failed to write HCI command");
    }
}

void HCIController::start_scan() {
    std::cout << "Starting LE Scan..." << std::endl;

    // 1. Set Scan Parameters
    le_set_scan_parameters_cp scan_params;
    memset(&scan_params, 0, sizeof(scan_params));
    scan_params.type = 0x01; // Active scanning
    scan_params.interval = htobs(0x0010); 
    scan_params.window = htobs(0x0010);
    scan_params.own_bdaddr_type = 0x00; // Public
    scan_params.filter = 0x00; // All

    std::array<uint8_t, sizeof(scan_params)> param_buf;
    memcpy(param_buf.data(), &scan_params, sizeof(scan_params));
    
    send_command(cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_SCAN_PARAMETERS), param_buf);

    // 2. Enable Scanning
    le_set_scan_enable_cp scan_enable;
    scan_enable.enable = 0x01;
    scan_enable.filter_dup = 0x01; // Filter duplicates
    
    std::array<uint8_t, sizeof(scan_enable)> enable_buf;
    memcpy(enable_buf.data(), &scan_enable, sizeof(scan_enable));

    send_command(cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE), enable_buf);
}

void HCIController::stop_scan() {
    le_set_scan_enable_cp scan_enable;
    scan_enable.enable = 0x00;
    scan_enable.filter_dup = 0x00;
    
    std::array<uint8_t, sizeof(scan_enable)> enable_buf;
    memcpy(enable_buf.data(), &scan_enable, sizeof(scan_enable));

    send_command(cmd_opcode_pack(OGF_LE_CTL, OCF_LE_SET_SCAN_ENABLE), enable_buf);
}

void HCIController::process_events() {
    // Poll for data or timeout
    struct pollfd fds[1];
    fds[0].fd = hci_fd_;
    fds[0].events = POLLIN;
    
    // 100ms timeout
    int ret = poll(fds, 1, 100);
    
    // Dispatch periodic process to modules
    for(auto* mod : modules_) {
        mod->process();
    }
    
    if (ret < 0) {
        if (errno == EINTR) return;
        perror("Poll HCI");
        return;
    }
    
    if (ret == 0) return; // Timeout
    
    if (fds[0].revents & POLLIN) {
        std::array<uint8_t, HCI_MAX_EVENT_SIZE> buf;
        ssize_t len = read(hci_fd_, buf.data(), buf.size());
        
        if (len < 0) {
            if (errno == EAGAIN || errno == EINTR) return;
            perror("Read HCI");
            return;
        }
    
        if (len == 0) return;
    
        uint8_t packet_type = buf[0];
    
    // Dispatch ACL Data
    if (packet_type == HCI_ACLDATA_PKT) {
        // Log it!
        // Log it? No, too verbal
        // std::cout << timestamp() << " [HCI] ACL Data Pkt: " << len << " bytes" << std::endl;
        
        std::span<const uint8_t> data(buf.data() + 1, len - 1);
        for(auto* mod : modules_) {
             mod->on_acl_data(data);
        }
        return;
    }

    if (packet_type != HCI_EVENT_PKT) return;

    uint8_t event_code = buf[1];
    
    // Debug Log for Non-Advertising events
    if (event_code != EVT_LE_META_EVENT || (len > 3 && buf[3] != EVT_LE_ADVERTISING_REPORT)) {
         std::cout << timestamp() << " [HCI] Event: 0x" << std::hex << (int)event_code << std::dec << std::endl;
                         
         if (event_code == 0x05) { // Disconnection Complete
             uint8_t status = buf[3];
             uint16_t handle = buf[4] | (buf[5] << 8);
             uint8_t reason = buf[6];
             std::cout << timestamp() << " [HCI] Disconnect Handle=0x" << std::hex << handle << " Reason=0x" << (int)reason << " Status=0x" << (int)status << std::dec << std::endl;
             
             for(auto* mod : modules_) {
                 mod->on_disconnect(handle, reason);
             }
         }
    }

    if (event_code == EVT_LE_META_EVENT) {
        uint8_t subevent = buf[3];
        
        if (subevent != EVT_LE_ADVERTISING_REPORT) {
             std::cout << timestamp() << " [HCI] LE Subevent: 0x" << std::hex << (int)subevent << std::dec;
             if (subevent == 0x03 && len >= 10) { // Connection Update Complete
                 uint8_t status = buf[4];
                 uint16_t handle = buf[5] | (buf[6] << 8);
                 std::cout << " (ConnUpdate) Status=0x" << (int)status << " Handle=0x" << handle;
             }
             std::cout << std::endl;
        }

        if (len > 4) {
             std::span<const uint8_t> data(buf.data() + 4, len - 4);
             for(auto* mod : modules_) {
                 mod->on_le_meta_event(subevent, data);
             }
        }
    }
    }
}

void HCIController::send_acl(uint16_t handle, std::span<const uint8_t> l2cap_pdu) {
    // HCI Packet Type (1) + Handle/Flags (2) + DataLen (2) + L2CAP PDU (N)
    std::vector<uint8_t> buf(1 + 4 + l2cap_pdu.size());
    
    buf[0] = HCI_ACLDATA_PKT;
    
    // Handle (12 bits) | PB Flag (2 bits) | BC Flag (2 bits)
    // PB=00 for First Fragment (Start) or 02 for Start?
    // Linux/BlueZ uses PB=0 (Start Non-Flush) for LE.
    
    uint16_t h_f = handle & 0x0FFF;
    h_f |= (0 << 12); // PB=0
    h_f |= (0 << 14); // BC=0
    
    buf[1] = h_f & 0xFF;
    buf[2] = (h_f >> 8) & 0xFF;
    
    uint16_t len = l2cap_pdu.size();
    buf[3] = len & 0xFF;
    buf[4] = (len >> 8) & 0xFF;
    
    memcpy(buf.data() + 5, l2cap_pdu.data(), l2cap_pdu.size());
    
    if (write(hci_fd_, buf.data(), buf.size()) < 0) {
        perror("Failed to write ACL data");
    }
}

} // namespace ble

std::string timestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    auto t = system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%T") << "." << std::setfill('0') << std::setw(3) << ms.count();
    return "[" + ss.str() + "]";
}
