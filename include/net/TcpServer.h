#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "FrameProtocol.h"
#include "pico/sync.h"
#include <array>
#include <cstdint>

class MediaApplication;

typedef void (*tcp_frame_dispatch_callback_t)(MediaApplication* app, const uint8_t* data, size_t len);

class TcpServer {
public:
    TcpServer(MediaApplication* app);
    bool init(uint16_t port);
    void close(); // <--- NEW: Explicitly close server
    void poll();
    err_t send_frame(Protocol::FrameType type, const uint8_t* payload, uint16_t len);

private:
    static err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
    static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    static void tcp_err_callback(void *arg, err_t err);

    err_t _recv_callback(struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    void _close_client_connection();

    MediaApplication* m_app_context;
    struct tcp_pcb *m_server_pcb = nullptr;
    struct tcp_pcb *m_client_pcb = nullptr;
    
    // Safety
    critical_section_t m_buffer_crit_sec;
    uint32_t m_last_activity_time_ms = 0;

    // Buffers
    static constexpr size_t RX_BUFFER_SIZE = Protocol::MAX_PAYLOAD_SIZE * 2;
    std::array<uint8_t, RX_BUFFER_SIZE> m_rx_buffer;
    volatile size_t m_rx_head = 0;
    volatile size_t m_rx_tail = 0;
};

#endif // TCPSERVER_H