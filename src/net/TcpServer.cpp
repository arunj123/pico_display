// File: src/net/TcpServer.cpp

#include "TcpServer.h"
#include "MediaApplication.h"
#include "pico/cyw43_arch.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

static TcpServer* g_tcp_server_instance = nullptr;

TcpServer::TcpServer(MediaApplication* app) : m_app_context(app) {
    g_tcp_server_instance = this;
    critical_section_init(&m_pbuf_queue_crit_sec);
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

// THE ISR: This is the ONLY code that runs in the interrupt context.
err_t TcpServer::_recv_callback(struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == nullptr) {
        _close_client_connection();
        return ERR_OK;
    }

    critical_section_enter_blocking(&m_pbuf_queue_crit_sec);
    if (m_pbuf_queue_count < PBUF_QUEUE_SIZE) {
        m_pbuf_queue[m_pbuf_queue_head] = p;
        m_pbuf_queue_head = (m_pbuf_queue_head + 1) % PBUF_QUEUE_SIZE;
        m_pbuf_queue_count++;
    } else {
        printf("ERROR: pbuf queue overflow! Discarding packet.\n");
        pbuf_free(p);
    }
    critical_section_exit(&m_pbuf_queue_crit_sec);
    
    return ERR_OK;
}

// THE MAIN LOOP PUMP: Called repeatedly from the application's main loop.
void TcpServer::poll() {
    struct pbuf* p = nullptr;
    
    critical_section_enter_blocking(&m_pbuf_queue_crit_sec);
    if (m_pbuf_queue_count > 0) {
        p = m_pbuf_queue[m_pbuf_queue_tail];
        m_pbuf_queue_tail = (m_pbuf_queue_tail + 1) % PBUF_QUEUE_SIZE;
        m_pbuf_queue_count--;
    }
    critical_section_exit(&m_pbuf_queue_crit_sec);

    if (p != nullptr) {
        cyw43_arch_lwip_check();
        
        if (p->tot_len > 0) {
            size_t free_space = RX_BUFFER_SIZE - m_rx_head;
            if (p->tot_len > free_space) {
                printf("ERROR: TCP stream buffer would overflow. Discarding data.\n");
            } else {
                pbuf_copy_partial(p, m_rx_buffer.data() + m_rx_head, p->tot_len, 0);
                m_rx_head += p->tot_len;
            }
        }
        
        tcp_recved(m_client_pcb, p->tot_len);
        pbuf_free(p);
        
        _process_stream_buffer();
    }
}

// The stream parser, now safely running in the main loop context via poll().
void TcpServer::_process_stream_buffer() {
    bool processed_frame;
    do {
        processed_frame = false;
        size_t data_len = m_rx_head - m_rx_tail;

        if (data_len < sizeof(Protocol::FrameHeader)) break;

        Protocol::FrameHeader header;
        memcpy(&header, m_rx_buffer.data() + m_rx_tail, sizeof(header));

        if (header.magic != Protocol::FRAME_MAGIC) {
            printf("ERROR: Bad magic byte. Searching...\n");
            m_rx_tail++;
            processed_frame = true;
            continue;
        }

        size_t frame_len = sizeof(header) + header.payload_length;
        if (data_len < frame_len) break;

        if (m_dispatch_callback) {
            m_dispatch_callback(m_app_context, m_rx_buffer.data() + m_rx_tail, frame_len);
        }

        m_rx_tail += frame_len;
        processed_frame = true;

    } while (processed_frame);

    if (m_rx_head > 0 && m_rx_head == m_rx_tail) {
        m_rx_head = 0;
        m_rx_tail = 0;
    }
}

void TcpServer::_close_client_connection() {
    if (m_client_pcb != nullptr) {
        tcp_arg(m_client_pcb, nullptr);
        tcp_recv(m_client_pcb, nullptr);
        tcp_err(m_client_pcb, nullptr);
        tcp_close(m_client_pcb);
        m_client_pcb = nullptr;
        m_rx_head = 0;
        m_rx_tail = 0;
        critical_section_enter_blocking(&m_pbuf_queue_crit_sec);
        while(m_pbuf_queue_count > 0) {
            struct pbuf* p = m_pbuf_queue[m_pbuf_queue_tail];
            m_pbuf_queue_tail = (m_pbuf_queue_tail + 1) % PBUF_QUEUE_SIZE;
            m_pbuf_queue_count--;
            pbuf_free(p);
        }
        critical_section_exit(&m_pbuf_queue_crit_sec);
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
void TcpServer::setReceiveCallback(tcp_frame_dispatch_callback_t callback) { m_dispatch_callback = callback; }
// err_t TcpServer::send_frame(Protocol::FrameType type, const uint8_t* payload, uint16_t len) { return ERR_OK; }