#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "lwip/tcp.h"
#include "FrameProtocol.h" // Include our new protocol header
#include <vector>
#include <cstdint>

// New callback type that provides a fully parsed frame
typedef void (*tcp_frame_receive_callback_t)(const Frame& frame);

class TcpServer {
public:
    TcpServer();
    bool init(uint16_t port);
    void setReceiveCallback(tcp_frame_receive_callback_t callback);
    err_t send_frame(FrameType type, const std::vector<uint8_t>& payload);

private:
    static err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err);
    static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    static void tcp_err_callback(void *arg, err_t err);

    err_t _recv_callback(struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    void _close_client_connection();
    void _parse_buffer(); // New method for frame parsing

    struct tcp_pcb *m_server_pcb = nullptr;
    struct tcp_pcb *m_client_pcb = nullptr;
    
    std::vector<uint8_t> m_rx_buffer;
    tcp_frame_receive_callback_t m_receive_callback = nullptr;

    // State for the frame parser
    enum class ParserState {
        WAITING_FOR_MAGIC,
        WAITING_FOR_HEADER,
        WAITING_FOR_PAYLOAD
    };
    ParserState m_parser_state = ParserState::WAITING_FOR_MAGIC;
    FrameHeader m_current_header;
};

#endif // TCPSERVER_H