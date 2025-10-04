// File: include/net/TcpServer.h

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "lwip/tcp.h"
#include "FrameProtocol.h"
#include <array>
#include <cstdint>

// Forward declare to resolve circular dependency
class MediaApplication;

// The callback now provides a raw buffer view
typedef void (*tcp_receive_callback_t)(MediaApplication* app, const uint8_t* data, size_t len);

class TcpServer {
public:
    TcpServer(MediaApplication* app);
    bool init(uint16_t port);
    void setReceiveCallback(tcp_receive_callback_t callback);
    err_t send_frame(Protocol::FrameType type, const uint8_t* payload, uint16_t len);

private:
    static err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
    static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    static void tcp_err_callback(void *arg, err_t err);

    err_t _recv_callback(struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    void _close_client_connection();
    void _parse_buffer();

    MediaApplication* m_app_context;
    struct tcp_pcb *m_server_pcb = nullptr;
    struct tcp_pcb *m_client_pcb = nullptr;
    
    tcp_receive_callback_t m_receive_callback = nullptr;

    // Fixed-size circular buffer for incoming data
    static constexpr size_t RX_BUFFER_SIZE = Protocol::MAX_PAYLOAD_SIZE + (Protocol::MAX_PAYLOAD_SIZE / 2);
    std::array<uint8_t, RX_BUFFER_SIZE> m_rx_buffer;
    size_t m_rx_buffer_len = 0;

    enum class ParserState {
        WAITING_FOR_MAGIC,
        WAITING_FOR_HEADER,
        WAITING_FOR_PAYLOAD
    };
    ParserState m_parser_state = ParserState::WAITING_FOR_MAGIC;
    Protocol::FrameHeader m_current_header;
};

#endif // TCPSERVER_H