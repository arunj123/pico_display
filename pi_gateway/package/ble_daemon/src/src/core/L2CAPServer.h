#ifndef CORE_L2CAP_SERVER_H
#define CORE_L2CAP_SERVER_H

#include <vector>
#include <functional>
#include <thread>
#include <atomic>

namespace ble {

class L2CAPServer {
public:
    using ConnectionCallback = std::function<void(int client_fd)>;

    L2CAPServer();
    ~L2CAPServer();

    void start(ConnectionCallback on_connect);
    void stop();

    // Polling loop (could be integrated into main loop, but using thread for simplicity in this demo)
    void poll_loop();

private:
    int server_fd_ = -1;
    std::atomic<bool> running_{false};
    std::thread poller_;
    ConnectionCallback on_connect_;
};

} // namespace ble

#endif // CORE_L2CAP_SERVER_H
