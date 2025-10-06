// File: include/net/TcpServer.h

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
    void setReceiveCallback(tcp_frame_dispatch_callback_t callback);
    void poll();
    err_t send_frame(Protocol::FrameType type, const uint8_t* payload, uint16_t len);

private:
    // Static callbacks remain the same
    static err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
    static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    static void tcp_err_callback(void *arg, err_t err);

    // Member function implementations
    err_t _recv_callback(struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    void _close_client_connection();
    void _process_stream_buffer();

    MediaApplication* m_app_context;
    struct tcp_pcb *m_server_pcb = nullptr;
    struct tcp_pcb *m_client_pcb = nullptr;
    tcp_frame_dispatch_callback_t m_dispatch_callback = nullptr;

    // Replace the pbuf queue with a simple circular byte buffer ---
    critical_section_t m_rx_crit_sec;
    static constexpr size_t CIRC_BUFFER_SIZE = 16 * 1024; // 16KB, should be plenty
    std::array<uint8_t, CIRC_BUFFER_SIZE> m_circ_buffer;
    volatile uint32_t m_circ_head = 0;
    volatile uint32_t m_circ_tail = 0;
    
    // This stream buffer is now populated from the circular buffer, not pbufs
    static constexpr size_t RX_BUFFER_SIZE = Protocol::MAX_PAYLOAD_SIZE * 2;
    std::array<uint8_t, RX_BUFFER_SIZE> m_rx_buffer;
    size_t m_rx_head = 0;
    size_t m_rx_tail = 0;
};

#endif // TCPSERVER_H