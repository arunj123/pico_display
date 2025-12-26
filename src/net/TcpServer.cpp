#include "TcpServer.h"
#include "MediaApplication.h"
#include "pico/cyw43_arch.h"
#include "pico/time.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

// CRC32 function must be visible here or included
extern uint32_t calculate_crc32(const uint8_t *data, size_t length);

static TcpServer* g_tcp_server_instance = nullptr;

TcpServer::TcpServer(MediaApplication* app) : m_app_context(app) {
    g_tcp_server_instance = this;
    critical_section_init(&m_buffer_crit_sec);
}

// --- Lifecycle Management ---

bool TcpServer::init(uint16_t port) {
    if (m_server_pcb) return true; // Already initialized

    m_server_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!m_server_pcb) return false;
    
    err_t err = tcp_bind(m_server_pcb, IP_ADDR_ANY, port);
    if (err != ERR_OK) {
        tcp_close(m_server_pcb);
        m_server_pcb = nullptr;
        return false;
    }
    
    m_server_pcb = tcp_listen(m_server_pcb);
    tcp_arg(m_server_pcb, this);
    tcp_accept(m_server_pcb, &TcpServer::tcp_accept_callback);
    
    // Reset buffer state
    critical_section_enter_blocking(&m_buffer_crit_sec);
    m_rx_head = 0;
    m_rx_tail = 0;
    critical_section_exit(&m_buffer_crit_sec);
    
    return true;
}

void TcpServer::close() {
    if (m_server_pcb) {
        tcp_close(m_server_pcb);
        m_server_pcb = nullptr;
    }
    _close_client_connection();
}

// --- Connection Handling ---

err_t TcpServer::tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (!g_tcp_server_instance) return ERR_ABRT;
    
    // If we already have a client, reject this one
    if (g_tcp_server_instance->m_client_pcb != nullptr) {
        tcp_abort(newpcb); 
        return ERR_ABRT;
    }

    printf("TCP Client Connected\n");
    g_tcp_server_instance->m_client_pcb = newpcb;
    g_tcp_server_instance->m_last_activity_time_ms = to_ms_since_boot(get_absolute_time());
    
    tcp_arg(newpcb, g_tcp_server_instance);
    tcp_recv(newpcb, &TcpServer::tcp_recv_callback);
    tcp_err(newpcb, &TcpServer::tcp_err_callback);
    
    // Important: Clear buffer on new connection to prevent processing old garbage
    critical_section_enter_blocking(&g_tcp_server_instance->m_buffer_crit_sec);
    g_tcp_server_instance->m_rx_head = 0;
    g_tcp_server_instance->m_rx_tail = 0;
    critical_section_exit(&g_tcp_server_instance->m_buffer_crit_sec);

    return ERR_OK;
}

// --- Data Reception ---

err_t TcpServer::tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    auto* server = static_cast<TcpServer*>(arg);
    if (server) return server->_recv_callback(tpcb, p, err);
    return ERR_VAL;
}

err_t TcpServer::_recv_callback(struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == nullptr) {
        _close_client_connection();
        return ERR_OK;
    }

    m_last_activity_time_ms = to_ms_since_boot(get_absolute_time());
    tcp_recved(tpcb, p->tot_len);

    // 1. Copy data into linear buffer (Protected)
    critical_section_enter_blocking(&m_buffer_crit_sec);
    if (m_rx_head + p->tot_len <= RX_BUFFER_SIZE) {
        pbuf_copy_partial(p, m_rx_buffer.data() + m_rx_head, p->tot_len, 0);
        m_rx_head += p->tot_len;
    } else {
        printf("ERROR: RX Buffer Overflow! Resetting connection.\n");
        critical_section_exit(&m_buffer_crit_sec);
        pbuf_free(p);
        _close_client_connection(); // Nuclear option
        return ERR_MEM;
    }
    critical_section_exit(&m_buffer_crit_sec);
    pbuf_free(p);

    // 2. Process Buffer Loop
    while (true) {
        // Snapshot indices for reading (head is volatile)
        size_t head = m_rx_head;
        size_t tail = m_rx_tail;
        size_t data_len = head - tail;

        if (data_len < sizeof(Protocol::FrameHeader)) break;

        Protocol::FrameHeader header;
        memcpy(&header, m_rx_buffer.data() + tail, sizeof(header));

        // Resync if magic is wrong
        if (header.magic != Protocol::FRAME_MAGIC) {
            printf("Bad Magic: %02x, skipping byte\n", header.magic);
            m_rx_tail++; 
            continue;
        }

        size_t frame_len = sizeof(header) + header.payload_length;
        if (data_len < frame_len) break; // Wait for more data

        // We have a full frame. Validate and Dispatch.
        const uint8_t* payload = m_rx_buffer.data() + tail + sizeof(Protocol::FrameHeader);

        if (header.type == Protocol::FrameType::IMAGE_TILE) {
            Protocol::ImageTileHeader tile_header;
            memcpy(&tile_header, payload, sizeof(Protocol::ImageTileHeader));
            const uint8_t* pixel_data = payload + sizeof(Protocol::ImageTileHeader);
            size_t pixel_len = header.payload_length - sizeof(Protocol::ImageTileHeader);
            
            uint32_t calc_crc = calculate_crc32(pixel_data, pixel_len);
            if (calc_crc == tile_header.crc32) {
                m_app_context->on_valid_tile_received(header, payload);
            } else {
                printf("CRC Mismatch! Exp: %08X, Calc: %08X. Len: %d\n", 
                       tile_header.crc32, calc_crc, pixel_len);
                send_frame(Protocol::FrameType::TILE_NACK, nullptr, 0);
            }
        }
        
        // Move tail past this frame
        m_rx_tail += frame_len;
    }

    // 3. Compact Buffer (Protected)
    // Only compact if tail is significant to save cycles
    if (m_rx_tail > 1024) {
        critical_section_enter_blocking(&m_buffer_crit_sec);
        size_t remaining = m_rx_head - m_rx_tail;
        if (remaining > 0) {
            memmove(m_rx_buffer.data(), m_rx_buffer.data() + m_rx_tail, remaining);
        }
        m_rx_head = remaining;
        m_rx_tail = 0;
        critical_section_exit(&m_buffer_crit_sec);
    }
    
    return ERR_OK;
}

// --- Cleanup & Error Handling ---

void TcpServer::tcp_err_callback(void *arg, err_t err) {
    auto* server = static_cast<TcpServer*>(arg);
    if (server) {
        printf("TCP Error: %d\n", err);
        // Do NOT call tcp_close here, PCB is already freed by LWIP
        server->m_client_pcb = nullptr; 
        
        // Just reset buffer state
        critical_section_enter_blocking(&server->m_buffer_crit_sec);
        server->m_rx_head = 0;
        server->m_rx_tail = 0;
        critical_section_exit(&server->m_buffer_crit_sec);
    }
}

void TcpServer::_close_client_connection() {
    if (m_client_pcb != nullptr) {
        tcp_arg(m_client_pcb, nullptr);
        tcp_sent(m_client_pcb, nullptr);
        tcp_recv(m_client_pcb, nullptr);
        tcp_err(m_client_pcb, nullptr);
        tcp_close(m_client_pcb);
        m_client_pcb = nullptr;
        printf("TCP Client Disconnected\n");

        critical_section_enter_blocking(&m_buffer_crit_sec);
        m_rx_head = 0;
        m_rx_tail = 0;
        critical_section_exit(&m_buffer_crit_sec);
    }
}

// --- Poll (Called from Main Loop) ---

void TcpServer::poll() {
    // Check for timeout
    if (m_client_pcb != nullptr) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        
        // Timeout after 120 seconds of inactivity
        if ((now - m_last_activity_time_ms) > 120000) { 
            printf("TCP Timeout. Resetting...\n");
            _close_client_connection();
        }
    }
}

// --- Send ---

err_t TcpServer::send_frame(Protocol::FrameType type, const uint8_t* payload, uint16_t len) {
    if (!m_client_pcb) return ERR_CONN;
    
    Protocol::FrameHeader header;
    header.magic = Protocol::FRAME_MAGIC;
    header.type = type;
    header.payload_length = len;

    // Note: tcp_write queues data. It doesn't send immediately.
    // If output buffer is full, it returns ERR_MEM.
    if (tcp_sndbuf(m_client_pcb) < (sizeof(header) + len)) {
        return ERR_MEM;
    }

    err_t err = tcp_write(m_client_pcb, &header, sizeof(header), TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK && len > 0) {
        err = tcp_write(m_client_pcb, payload, len, TCP_WRITE_FLAG_COPY);
    }

    if (err == ERR_OK) {
        tcp_output(m_client_pcb);
    }
    
    return err;
}