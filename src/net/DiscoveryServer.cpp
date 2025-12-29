#include "DiscoveryServer.h"
#include <cstring>
#include <cstdio>

DiscoveryServer& DiscoveryServer::getInstance() {
    static DiscoveryServer instance;
    return instance;
}

DiscoveryServer::DiscoveryServer() {}

void DiscoveryServer::init() {
    if (m_pcb) return;

    m_pcb = udp_new();
    if (m_pcb) {
        udp_bind(m_pcb, IP_ADDR_ANY, DISCOVERY_PORT);
        udp_recv(m_pcb, udp_recv_callback, nullptr);
        printf("[Discovery] Listening on UDP %d\n", DISCOVERY_PORT);
    }
}

void DiscoveryServer::stop() {
    if (m_pcb) {
        udp_remove(m_pcb);
        m_pcb = nullptr;
    }
}

void DiscoveryServer::udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    if (p == nullptr) return;

    // Check content
    char buffer[32];
    uint16_t len = p->tot_len < sizeof(buffer) - 1 ? p->tot_len : sizeof(buffer) - 1;
    pbuf_copy_partial(p, buffer, len, 0);
    buffer[len] = 0;

    // Protocol: Host sends "PICO_DISCOVER", we reply "PICO_HERE"
    if (strncmp(buffer, "PICO_DISCOVER", 13) == 0) {
        // Prepare response
        const char* response = "PICO_HERE";
        struct pbuf *resp_pbuf = pbuf_alloc(PBUF_TRANSPORT, strlen(response), PBUF_RAM);
        
        if (resp_pbuf) {
            memcpy(resp_pbuf->payload, response, strlen(response));
            udp_sendto(pcb, resp_pbuf, addr, port);
            pbuf_free(resp_pbuf);
            printf("[Discovery] Responded to %s\n", ipaddr_ntoa(addr));
        }
    }

    pbuf_free(p);
}