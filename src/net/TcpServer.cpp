// File: src/net/TcpServer.cpp

#include "TcpServer.h"
#include "pico/cyw43_arch.h"
#include <cstdio>
#include <cstring>
#include <algorithm> // For std::min

// Global instance pointer for static lwIP callbacks
static TcpServer* g_tcp_server_instance = nullptr;

// --- lwIP Callback Implementations ---
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

// --- Frame Parsing Logic ---
void TcpServer::_parse_buffer() {
    bool processed_frame;
    do {
        processed_frame = false;
        const uint8_t* buf_ptr = m_rx_buffer.data();
        size_t buf_len = m_rx_buffer_len;

        switch (m_parser_state) {
            case ParserState::WAITING_FOR_MAGIC:
                if (buf_len > 0) {
                    if (buf_ptr[0] == Protocol::FRAME_MAGIC) {
                        m_parser_state = ParserState::WAITING_FOR_HEADER;
                    } else {
                        // Not a magic byte, discard it
                        memmove(m_rx_buffer.data(), m_rx_buffer.data() + 1, buf_len - 1);
                        m_rx_buffer_len--;
                    }
                }
                break;

            case ParserState::WAITING_FOR_HEADER:
                if (buf_len >= sizeof(Protocol::FrameHeader)) {
                    memcpy(&m_current_header, buf_ptr, sizeof(Protocol::FrameHeader));
                    m_parser_state = ParserState::WAITING_FOR_PAYLOAD;
                }
                break;

            case ParserState::WAITING_FOR_PAYLOAD:
                size_t frame_len = sizeof(Protocol::FrameHeader) + m_current_header.payload_length;
                if (buf_len >= frame_len) {
                    if (m_receive_callback) {
                        m_receive_callback(m_app_context, buf_ptr, frame_len);
                    }
                    
                    // Remove the processed frame from the buffer
                    memmove(m_rx_buffer.data(), m_rx_buffer.data() + frame_len, buf_len - frame_len);
                    m_rx_buffer_len -= frame_len;
                    
                    m_parser_state = ParserState::WAITING_FOR_MAGIC;
                    processed_frame = true;
                }
                break;
        }
    } while (processed_frame && m_rx_buffer_len > 0);
}

err_t TcpServer::_recv_callback(struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    // If p is NULL, the connection has been closed.
    if (p == nullptr) {
        printf("TCP Client disconnected.\n");
        _close_client_connection();
        return ERR_OK;
    }

    // This method is called from an interrupt, so lock the scheduler
    cyw43_arch_lwip_check();

    if (p->tot_len > 0) {
        size_t free_space = RX_BUFFER_SIZE - m_rx_buffer_len;
        size_t copy_len = std::min((size_t)p->tot_len, free_space);

        if (copy_len < p->tot_len) {
            printf("TCP RX Buffer Overflow! Discarding data.\n");
        }
        
        if (copy_len > 0) {
            pbuf_copy_partial(p, m_rx_buffer.data() + m_rx_buffer_len, copy_len, 0);
            m_rx_buffer_len += copy_len;
        }

        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);
    _parse_buffer();
    return ERR_OK;
}

void TcpServer::_close_client_connection() {
    if (m_client_pcb != nullptr) {
        tcp_arg(m_client_pcb, nullptr);
        tcp_recv(m_client_pcb, nullptr);
        tcp_err(m_client_pcb, nullptr);
        tcp_close(m_client_pcb);
        m_client_pcb = nullptr;
        m_rx_buffer_len = 0; // Reset buffer length
        m_parser_state = ParserState::WAITING_FOR_MAGIC; // Reset parser state
    }
}

// --- Class Methods ---
TcpServer::TcpServer(MediaApplication* app) : m_app_context(app) {
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

void TcpServer::setReceiveCallback(tcp_receive_callback_t callback) {
    m_receive_callback = callback;
}

err_t TcpServer::send_frame(Protocol::FrameType type, const uint8_t* payload, uint16_t len) {
    if (m_client_pcb == nullptr) return ERR_CONN;

    Protocol::FrameHeader header = {Protocol::FRAME_MAGIC, type, len};
    
    // First, try to write the header
    err_t err = tcp_write(m_client_pcb, &header, sizeof(header), TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);
    if (err != ERR_OK) {
        printf("Failed to write TCP header (err %d)\n", err);
        return err;
    }

    // Then, write the payload
    if (len > 0 && payload != nullptr) {
        err = tcp_write(m_client_pcb, payload, len, TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            printf("Failed to write TCP payload (err %d)\n", err);
            return err;
        }
    }
    
    err = tcp_output(m_client_pcb);
    if (err != ERR_OK) {
        printf("Failed to output TCP data (err %d)\n", err);
    }
    return err;
}