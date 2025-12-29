#ifndef DISCOVERY_SERVER_H
#define DISCOVERY_SERVER_H

#include "lwip/udp.h"
#include <cstdint>

class DiscoveryServer {
public:
    // Singleton access
    static DiscoveryServer& getInstance();
    
    void init();
    void stop();

private:
    DiscoveryServer();
    static void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);

    struct udp_pcb *m_pcb = nullptr;
    const uint16_t DISCOVERY_PORT = 4243;
};

#endif // DISCOVERY_SERVER_H
