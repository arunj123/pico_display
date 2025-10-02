#include "TcpServer.h"
#include "pico/cyw43_arch.h"
#include <cstdio>
#include <cstring>

static TcpServer* g_tcp_server_instance = nullptr;

// --- lwIP Callback Implementations (mostly unchanged) ---
err_t TcpServer::tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (newpcb == nullptr) return ERR_VAL;
    printf("TCP Client Connected\n");
    if (g_tcp_server_instance->m_client_pcb != nullptr) {
        tcp_close(newpcb);
        return ERR_ABRT;
    }
    g_tcp_server_instance->m_client_pcb = newpcb;
    tcp_arg(newpcb, g_tcp_server_instance);
    tcp_recv(newpcb, &TcpServer::tcp_recv_callback);
    tcp_err(newpcb, &TcpServer::tcp_err_callback);
    return ERR_OK;
}

err_t TcpServer::tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    auto* server = static_cast<TcpServer*>(arg);
    if (server) return server->_recv_callback(tpcb, p, err);
    return ERR_VAL;
}

void TcpServer::tcp_err_callback(void *arg, err_t err) {
    auto* server = static_cast<TcpServer*>(arg);
    if (server) {
        printf("TCP Error: %d\n", err);
        server->_close_client_connection();
    }
}

// --- NEW Frame Parsing Logic ---
void TcpServer::_parse_buffer() {
    bool processed_frame;
    do {
        processed_frame = false;
        switch (m_parser_state) {
            case ParserState::WAITING_FOR_MAGIC:
                if (!m_rx_buffer.empty()) {
                    if (m_rx_buffer[0] == FRAME_MAGIC) {
                        m_parser_state = ParserState::WAITING_FOR_HEADER;
                    } else {
                        // Not a magic byte, discard it and keep searching
                        m_rx_buffer.erase(m_rx_buffer.begin());
                    }
                }
                break;

            case ParserState::WAITING_FOR_HEADER:
                if (m_rx_buffer.size() >= sizeof(FrameHeader)) {
                    memcpy(&m_current_header, m_rx_buffer.data(), sizeof(FrameHeader));
                    m_parser_state = ParserState::WAITING_FOR_PAYLOAD;
                }
                break;

            case ParserState::WAITING_FOR_PAYLOAD:
                if (m_rx_buffer.size() >= sizeof(FrameHeader) + m_current_header.payload_length) {
                    Frame frame;
                    frame.header = m_current_header;
                    frame.payload.assign(m_rx_buffer.begin() + sizeof(FrameHeader), 
                                         m_rx_buffer.begin() + sizeof(FrameHeader) + frame.header.payload_length);

                    if (m_receive_callback) {
                        m_receive_callback(frame);
                    }
                    
                    // Remove the processed frame from the buffer
                    m_rx_buffer.erase(m_rx_buffer.begin(), 
                                      m_rx_buffer.begin() + sizeof(FrameHeader) + frame.header.payload_length);
                    
                    m_parser_state = ParserState::WAITING_FOR_MAGIC;
                    processed_frame = true; // Indicate we might have another full frame in the buffer
                }
                break;
        }
    } while (processed_frame && !m_rx_buffer.empty());
}

err_t TcpServer::_recv_callback(struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == nullptr) {
        printf("TCP Client disconnected.\n");
        _close_client_connection();
        return ERR_OK;
    }

    if (p->tot_len > 0) {
        for (struct pbuf *q = p; q != nullptr; q = q->next) {
            m_rx_buffer.insert(m_rx_buffer.end(), (uint8_t*)q->payload, (uint8_t*)q->payload + q->len);
        }
        tcp_recved(tpcb, p->tot_len);
        _parse_buffer(); // Attempt to parse frames from the new data
    }
    pbuf_free(p);
    return ERR_OK;
}

// ... _close_client_connection is mostly unchanged ...
void TcpServer::_close_client_connection() {
    if (m_client_pcb != nullptr) {
        tcp_arg(m_client_pcb, nullptr);
        tcp_recv(m_client_pcb, nullptr);
        tcp_err(m_client_pcb, nullptr);
        tcp_close(m_client_pcb);
        m_client_pcb = nullptr;
        m_rx_buffer.clear();
        m_parser_state = ParserState::WAITING_FOR_MAGIC; // Reset parser state
    }
}

// --- Class Methods ---
TcpServer::TcpServer() {
    g_tcp_server_instance = this;
}

bool TcpServer::init(uint16_t port) {
    m_server_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!m_server_pcb) return false;
    err_t err = tcp_bind(m_server_pcb, IP_ADDR_ANY, port);
    if (err != ERR_OK) return false;
    m_server_pcb = tcp_listen(m_server_pcb);
    if (!m_server_pcb) return false;
    tcp_arg(m_server_pcb, this);
    tcp_accept(m_server_pcb, &TcpServer::tcp_accept_callback);
    printf("TCP Server initialized and listening on port %d\n", port);
    return true;
}

void TcpServer::setReceiveCallback(tcp_frame_receive_callback_t callback) {
    m_receive_callback = callback;
}

// New method to send a framed response
err_t TcpServer::send_frame(FrameType type, const std::vector<uint8_t>& payload) {
    if (m_client_pcb == nullptr) return ERR_CONN;

    std::vector<uint8_t> frame_data;
    FrameHeader header = {FRAME_MAGIC, type, (uint16_t)payload.size()};
    
    frame_data.resize(sizeof(FrameHeader) + payload.size());
    memcpy(frame_data.data(), &header, sizeof(FrameHeader));
    memcpy(frame_data.data() + sizeof(FrameHeader), payload.data(), payload.size());
    
    err_t err = tcp_write(m_client_pcb, frame_data.data(), frame_data.size(), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("Failed to write to TCP stream (err %d)\n", err);
        return err;
    }

    err = tcp_output(m_client_pcb);
    if (err != ERR_OK) {
        printf("Failed to output TCP data (err %d)\n", err);
    }
    return err;
}