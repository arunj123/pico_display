#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "lwip/tcp.h"
#include <vector>
#include <cstdint>

// Callback function pointer type for received data
typedef void (*tcp_receive_callback_t)(const std::vector<uint8_t>& data);

class TcpServer {
public:
    TcpServer();
    bool init(uint16_t port);
    void setReceiveCallback(tcp_receive_callback_t callback);
    err_t send_data(const std::vector<uint8_t>& data);

private:
    // --- lwIP Callbacks (must be static) ---
    static err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
    static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    static void tcp_err_callback(void *arg, err_t err);

    // --- Member function implementations for the callbacks ---
    err_t _recv_callback(struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    void _close_client_connection();

    // --- Member variables for state ---
    struct tcp_pcb *m_server_pcb = nullptr;
    struct tcp_pcb *m_client_pcb = nullptr;
    
    std::vector<uint8_t> m_rx_buffer;
    tcp_receive_callback_t m_receive_callback = nullptr;
};

#endif // TCPSERVER_H
