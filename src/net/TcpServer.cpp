// File: src/net/TcpServer.cpp

#include "TcpServer.h"
#include "MediaApplication.h"
#include "pico/cyw43_arch.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

uint32_t calculate_crc32(const uint8_t *data, size_t length); // Forward-declare

static TcpServer* g_tcp_server_instance = nullptr;

TcpServer::TcpServer(MediaApplication* app) : m_app_context(app) {
    g_tcp_server_instance = this;
    critical_section_init(&m_rx_crit_sec);
}

// --- Static Callbacks ---
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

// The TCP receive interrupt handler
err_t TcpServer::_recv_callback(struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == nullptr) {
        _close_client_connection();
        return ERR_OK;
    }
    tcp_recved(tpcb, p->tot_len);

    if (m_rx_head + p->tot_len <= RX_BUFFER_SIZE) {
        pbuf_copy_partial(p, m_rx_buffer.data() + m_rx_head, p->tot_len, 0);
        m_rx_head += p->tot_len;
    } else {
        printf("ERROR: Linear RX buffer overflow!\n");
        pbuf_free(p);
        return ERR_OK;
    }
    pbuf_free(p);

    while (true) {
        size_t data_len = m_rx_head - m_rx_tail;
        if (data_len < sizeof(Protocol::FrameHeader)) break;

        Protocol::FrameHeader header;
        memcpy(&header, m_rx_buffer.data() + m_rx_tail, sizeof(header));

        if (header.magic != Protocol::FRAME_MAGIC) {
            m_rx_tail++;
            continue;
        }

        size_t frame_len = sizeof(header) + header.payload_length;
        if (data_len < frame_len) break;
        
        const uint8_t* payload = m_rx_buffer.data() + m_rx_tail + sizeof(Protocol::FrameHeader);

        if (header.type == Protocol::FrameType::IMAGE_TILE) {
            Protocol::ImageTileHeader tile_header;
            memcpy(&tile_header, payload, sizeof(Protocol::ImageTileHeader));
            const uint8_t* pixel_data = payload + sizeof(Protocol::ImageTileHeader);
            size_t pixel_len = header.payload_length - sizeof(Protocol::ImageTileHeader);
            
            if (calculate_crc32(pixel_data, pixel_len) == tile_header.crc32) {
                // CRC is good. Pass the validated frame to the application to be queued.
                // The application is now responsible for sending the ACK.
                m_app_context->on_valid_tile_received(header, payload);
            } else {
                // CRC is bad. We can send NACK immediately because we are dropping the data.
                send_frame(Protocol::FrameType::TILE_NACK, nullptr, 0);
                printf("!!! CHECKSUM MISMATCH in ISR !!!\n");
            }
        }
        
        m_rx_tail += frame_len;
    }

    if (m_rx_tail > 0) {
        size_t remaining = m_rx_head - m_rx_tail;
        if (remaining > 0) {
            memmove(m_rx_buffer.data(), m_rx_buffer.data() + m_rx_tail, remaining);
        }
        m_rx_head = remaining;
        m_rx_tail = 0;
    }
    
    return ERR_OK;
}

// The poll function is no longer needed, as logic is in the ISR. We keep a stub.
void TcpServer::poll() {
    cyw43_arch_poll();
}

err_t TcpServer::send_frame(Protocol::FrameType type, const uint8_t* payload, uint16_t len) {
    if (!m_client_pcb || len > Protocol::MAX_PAYLOAD_SIZE) {
        return ERR_VAL;
    }
    if (tcp_sndbuf(m_client_pcb) < (sizeof(Protocol::FrameHeader) + len)) {
        return ERR_MEM;
    }

    Protocol::FrameHeader header;
    header.magic = Protocol::FRAME_MAGIC;
    header.type = type;
    header.payload_length = len;

    err_t err = tcp_write(m_client_pcb, &header, sizeof(header), TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK && payload && len > 0) {
        err = tcp_write(m_client_pcb, payload, len, TCP_WRITE_FLAG_COPY);
    }

    if (err == ERR_OK) {
        err = tcp_output(m_client_pcb);
    }
    
    // --- Give the network stack a chance to process the outgoing data NOW ---
    if (err == ERR_OK) {
        cyw43_arch_poll();
    }
    
    return err;
}

// The stream parser, safely running in the main loop context via poll().
void TcpServer::_process_stream_buffer() {
    // No longer needed, logic moved to _recv_callback
}

// --- This function now cleans up the new circular buffer ---
void TcpServer::_close_client_connection() {
    if (m_client_pcb != nullptr) {
        tcp_arg(m_client_pcb, nullptr);
        tcp_recv(m_client_pcb, nullptr);
        tcp_err(m_client_pcb, nullptr);
        tcp_close(m_client_pcb);
        m_client_pcb = nullptr;
        printf("TCP Client Disconnected\n");

        // Reset the linear processing buffer
        m_rx_head = 0;
        m_rx_tail = 0;
        
        // Reset the circular buffer by setting head and tail to the same value
        critical_section_enter_blocking(&m_rx_crit_sec);
        m_circ_head = 0;
        m_circ_tail = 0;
        critical_section_exit(&m_rx_crit_sec);
    }
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

// This function is no longer used, we call the app method directly
void TcpServer::setReceiveCallback(tcp_frame_dispatch_callback_t callback) { }