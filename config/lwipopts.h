// File: config/lwipopts.h

#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// --- Use the required NO_SYS=1 setting ---
#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0

// --- Memory Options ---
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    (10 * 1024)

// --- Pbuf Options ---
#define PBUF_POOL_SIZE              24

// --- TCP Options ---
#define TCP_MSS                     1460
#define TCP_WND                     (8 * TCP_MSS)
#define TCP_SND_BUF                 (8 * TCP_MSS)
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1))/(TCP_MSS))
#define TCP_TMR_INTERVAL            100 // Required for threadsafe background mode

// Ensure we have enough memory pool entries for the TCP send queue
#define MEMP_NUM_TCP_SEG            TCP_SND_QUEUELEN

// --- Core Protocol Options ---
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_DHCP                   1
#define LWIP_DNS                    1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_STATS                  0
#define LWIP_TCP_KEEPALIVE          1

// --- Checksum options ---
#define CHECKSUM_GEN_IP             1
#define CHECKSUM_GEN_UDP            1
#define CHECKSUM_GEN_TCP            1
#define CHECKSUM_CHECK_IP           1
#define CHECKSUM_CHECK_UDP          1
#define CHECKSUM_CHECK_TCP          1
#define CHECKSUM_GEN_ICMP           1

// --- Pico-specific options for threadsafe background mode ---
#define TCPIP_THREAD_STACKSIZE      1024
#define DEFAULT_THREAD_STACKSIZE    1024
#define DEFAULT_RAW_RECVMBOX_SIZE   8
#define TCPIP_MBOX_SIZE             8
#define LWIP_TIMEVAL_PRIVATE        0
#define LWIP_TCPIP_CORE_LOCKING_INPUT 1

#endif