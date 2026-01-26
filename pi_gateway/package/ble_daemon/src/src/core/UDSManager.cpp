#include "UDSManager.h"
#include <iostream>
#include <algorithm>
#include <cstring>

namespace ble {

UDSManager::UDSManager(const std::string& path) : path_(path) {
    setup_socket();
}

UDSManager::~UDSManager() {
    if (listen_fd_ >= 0) close(listen_fd_);
    for (int fd : clients_) close(fd);
    unlink(path_.c_str());
}

void UDSManager::setup_socket() {
    unlink(path_.c_str());
    listen_fd_ = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listen_fd_ < 0) {
        perror("UDS socket");
        return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path_.c_str(), sizeof(addr.sun_path)-1);

    if (bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("UDS bind");
        close(listen_fd_);
        listen_fd_ = -1;
        return;
    }

    if (listen(listen_fd_, 5) < 0) {
        perror("UDS listen");
        close(listen_fd_);
        listen_fd_ = -1;
        return;
    }
}

void UDSManager::process() {
    if (listen_fd_ < 0) return;

    // Accept new clients
    int client_fd;
    while ((client_fd = accept(listen_fd_, nullptr, nullptr)) >= 0) {
        int flags = fcntl(client_fd, F_GETFL, 0);
        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
        clients_.push_back(client_fd);
        std::cout << "[UDS] New client connected" << std::endl;
        
        if (on_connected_) {
            on_connected_(client_fd);
        }
    }
}

void UDSManager::send_to_client(int client_fd, const std::string& message) {
    std::string packet = message + "\n";
    write(client_fd, packet.c_str(), packet.size());
}

void UDSManager::broadcast(const std::string& message) {
    std::string packet = message + "\n";
    auto it = clients_.begin();
    while (it != clients_.end()) {
        ssize_t sent = write(*it, packet.c_str(), packet.size());
        if (sent < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cout << "[UDS] Client disconnected" << std::endl;
                close(*it);
                it = clients_.erase(it);
                continue;
            }
        }
        ++it;
    }
}

} // namespace ble
