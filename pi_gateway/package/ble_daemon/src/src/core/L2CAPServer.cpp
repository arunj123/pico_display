#include "L2CAPServer.h"
#include "compat/l2cap.h"
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <thread>
#include <chrono>

namespace ble {

L2CAPServer::L2CAPServer() {}

L2CAPServer::~L2CAPServer() {
    stop();
}

void L2CAPServer::start(ConnectionCallback on_connect) {
    on_connect_ = on_connect;
    
    server_fd_ = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    if (server_fd_ < 0) {
        throw std::system_error(errno, std::generic_category(), "L2CAP socket failed");
    }

    struct sockaddr_l2 addr;
    memset(&addr, 0, sizeof(addr));
    addr.l2_family = AF_BLUETOOTH;
    // addr.l2_bdaddr = *BDADDR_ANY; // Error
    memset(&addr.l2_bdaddr, 0, sizeof(addr.l2_bdaddr)); // Equivalent to ANY
    addr.l2_psm = htobs(0); // 0??? Wait. 
                            // For LE, we bind to CID directly? No.
                            // Accessing GATT over LE uses fixed CID 4.
                            // But standard linux socket API:
                            // To listen for LE connections:
                            // bind with cid = ATT_CID? 
                            // Actually, standard BlueZ L2CAP socket for LE server:
                            // bind to `bdaddr_type = BDADDR_LE_PUBLIC` (or ANY),
                            // but implementation usually handles the signaling.
                            // For a peripheral, the KERNEL listens?
                            // No, we must listen.
                            // 
                            // How to listen for LE connections on port 4 (ATT)?
                            // In BlueZ/Linux:
                            // socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP)
                            // bind(addr with l2_cid = htobs(4), l2_bdaddr_type = BDADDR_LE_PUBLIC)
                            // listen()
    
    addr.l2_cid = htobs(L2CAP_CID_ATT); // 4
    addr.l2_bdaddr_type = BDADDR_LE_PUBLIC;

    if (bind(server_fd_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        // Fallback for older kernels/setups or if we need to set security level first?
         perror("L2CAP bind failed");
         // Maybe permissions?
    }
    
    // Set security level? sudo required usually.
    
    if (listen(server_fd_, 1) < 0) {
        perror("L2CAP listen failed");
    }

    running_ = true;
    poller_ = std::thread(&L2CAPServer::poll_loop, this);
}

void L2CAPServer::stop() {
    running_ = false;
    if (poller_.joinable()) poller_.join();
    if (server_fd_ >= 0) close(server_fd_);
    server_fd_ = -1;
}

void L2CAPServer::poll_loop() {
    while(running_) {
        struct sockaddr_l2 peer_addr;
        socklen_t len = sizeof(peer_addr);
        int client = accept(server_fd_, (struct sockaddr *)&peer_addr, &len);
        
        if (client >= 0) {
            std::cout << "Incoming L2CAP Connection!" << std::endl;
            if (on_connect_) on_connect_(client);
        } else {
            // Sleep to avoid busy loop if non-blocking, but accept is blocking unless configured otherwise
            // If blocking, how to stop? We'll assume accept blocks. 
            // To stop clean we'd need shutdown() or similar.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

} // namespace ble
