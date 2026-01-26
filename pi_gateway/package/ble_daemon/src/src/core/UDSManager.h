#ifndef CORE_UDS_MANAGER_H
#ifndef CORE_UDS_MANAGER_H
#define CORE_UDS_MANAGER_H

#include <string>
#include <vector>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include <functional>

namespace ble {

class UDSManager {
public:
    using OnClientConnected = std::function<void(int client_fd)>;

    UDSManager(const std::string& path);
    ~UDSManager();

    void set_on_client_connected(OnClientConnected cb) { on_connected_ = cb; }
    void broadcast(const std::string& message);
    void send_to_client(int client_fd, const std::string& message);
    void process();

private:
    std::string path_;
    int listen_fd_ = -1;
    std::vector<int> clients_;
    OnClientConnected on_connected_;

    void setup_socket();
};

} // namespace ble

#endif // CORE_UDS_MANAGER_H
#endif
