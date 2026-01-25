#ifndef COMPAT_BLUETOOTH_H
#define COMPAT_BLUETOOTH_H

#include <stdint.h>
#include <sys/socket.h>

// Basic Bluetooth definitions usually found in bluetooth/bluetooth.h

#ifdef __cplusplus
extern "C" {
#endif

// Byte order conversion
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define htobs(d)  (d)
#define btohs(d)  (d)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define htobs(d)  bswap_16(d)
#define btohs(d)  bswap_16(d)
#else
#error "Unknown byte order"
#endif

// Address struct
typedef struct {
	uint8_t b[6];
} __attribute__((packed)) bdaddr_t;

#define BDADDR_ANY   (&(bdaddr_t) {{0, 0, 0, 0, 0, 0}})
#define BDADDR_ALL   (&(bdaddr_t) {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}})
#define BDADDR_LOCAL (&(bdaddr_t) {{0, 0, 0, 0xff, 0xff, 0xff}})

// Protocol families
#ifndef AF_BLUETOOTH
#define AF_BLUETOOTH	31
#define PF_BLUETOOTH	AF_BLUETOOTH
#endif

#ifndef BTPROTO_L2CAP
#define BTPROTO_L2CAP	0
#endif

#ifndef BTPROTO_HCI
#define BTPROTO_HCI	1
#endif

// Allocator helpers
#define bacmp(ba1, ba2)	memcmp((ba1), (ba2), sizeof(bdaddr_t))
#define bacpy(dst, src)	memcpy((dst), (src), sizeof(bdaddr_t))

#ifdef __cplusplus
}
#endif

#endif // COMPAT_BLUETOOTH_H
