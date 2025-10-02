#include "TcpServer.h"
#include "pico/cyw43_arch.h"
#include <cstdio>
#include <cstring>

// Global instance for static C callbacks to find the C++ object
static TcpServer* g_tcp_server_instance = nullptr;

// --- lwIP Callback Implementations ---

err_t TcpServer::tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (newpcb == nullptr) {
        printf("TCP Accept: Failed to accept new connection\n");
        return ERR_VAL;
    }
    printf("TCP Client Connected\n");
    
    // Only allow one client at a time
    if (g_tcp_server_instance->m_client_pcb != nullptr) {
        printf("TCP Accept: Already have a client, refusing new one.\n");
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
    if (server) {
        return server->_recv_callback(tpcb, p, err);
    }
    return ERR_VAL;
}

void TcpServer::tcp_err_callback(void *arg, err_t err) {
    auto* server = static_cast<TcpServer*>(arg);
    if (server) {
        printf("TCP Error: %d\n", err);
        server->_close_client_connection();
    }
}

err_t TcpServer::_recv_callback(struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == nullptr) {
        printf("TCP Client disconnected.\n");
        _close_client_connection();
        return ERR_OK;
    }

    if (p->tot_len > 0) {
        printf("TCP receiving %d bytes\n", p->tot_len);
        // Copy data from pbuf chain to our vector
        for (struct pbuf *q = p; q != nullptr; q = q->next) {
            m_rx_buffer.insert(m_rx_buffer.end(), (uint8_t*)q->payload, (uint8_t*)q->payload + q->len);
        }
        tcp_recved(tpcb, p->tot_len);

        // For this example, we assume 8KB is the full message
        if (m_rx_buffer.size() >= 8192) {
            printf("TCP full 8KB message received.\n");
            if (m_receive_callback) {
                m_receive_callback(m_rx_buffer);
            }
            m_rx_buffer.clear(); // Clear buffer for next message
        }
    }
    pbuf_free(p);
    return ERR_OK;
}

void TcpServer::_close_client_connection() {
    if (m_client_pcb != nullptr) {
        tcp_arg(m_client_pcb, nullptr);
        tcp_recv(m_client_pcb, nullptr);
        tcp_err(m_client_pcb, nullptr);
        tcp_close(m_client_pcb);
        m_client_pcb = nullptr;
        m_rx_buffer.clear();
    }
}

// --- TcpServer Class Methods ---

TcpServer::TcpServer() {
    g_tcp_server_instance = this;
}

bool TcpServer::init(uint16_t port) {
    m_server_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!m_server_pcb) {
        printf("Failed to create TCP PCB\n");
        return false;
    }

    err_t err = tcp_bind(m_server_pcb, IP_ADDR_ANY, port);
    if (err != ERR_OK) {
        printf("Failed to bind TCP server to port %d (err %d)\n", port, err);
        return false;
    }

    m_server_pcb = tcp_listen(m_server_pcb);
    if (!m_server_pcb) {
        printf("Failed to listen on TCP server\n");
        return false;
    }
    
    tcp_arg(m_server_pcb, this);
    tcp_accept(m_server_pcb, &TcpServer::tcp_accept_callback);
    printf("TCP Server initialized and listening on port %d\n", port);
    return true;
}

void TcpServer::setReceiveCallback(tcp_receive_callback_t callback) {
    m_receive_callback = callback;
}

err_t TcpServer::send_data(const std::vector<uint8_t>& data) {
    if (m_client_pcb == nullptr) {
        return ERR_CONN;
    }
    
    err_t err = tcp_write(m_client_pcb, data.data(), data.size(), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("Failed to write to TCP stream (err %d)\n", err);
        return err;
    }

    err = tcp_output(m_client_pcb);
    if (err != ERR_OK) {
        printf("Failed to output TCP data (err %d)\n", err);
        return err;
    }
    printf("TCP sent %zu bytes of data.\n", data.size());
    return ERR_OK;
}
