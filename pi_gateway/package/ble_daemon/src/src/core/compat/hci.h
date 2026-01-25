#ifndef COMPAT_HCI_H
#define COMPAT_HCI_H

#include <sys/ioctl.h>
#include "bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HCI_MAX_DEV	16
#define HCI_MAX_ACL_SIZE	1024
#define HCI_MAX_SCO_SIZE	255
#define HCI_MAX_EVENT_SIZE	260
#define HCI_MAX_FRAME_SIZE	(HCI_MAX_ACL_SIZE + 4)

// HCI Socket Options
#define HCI_DATA_DIR	1
#define HCI_FILTER	2
#define HCI_TIME_STAMP	3

// HCI Packet Types
#define HCI_COMMAND_PKT		0x01
#define HCI_ACLDATA_PKT		0x02
#define HCI_SCODATA_PKT		0x03
#define HCI_EVENT_PKT		0x04
#define HCI_VENDOR_PKT		0xff

// HCI Events
#define EVT_LE_META_EVENT	0x3E
#define EVT_LE_ADVERTISING_REPORT 0x02

// HCI Ioctls
// HCI Ioctls
#define HCIDEVUP	_IOW('H', 201, int)
#define HCIDEVDOWN	_IOW('H', 202, int)
#define HCIDEVRESET	_IOW('H', 203, int)
#define HCIGETDEVLIST	_IOR('H', 210, int)
#define HCIGETDEVINFO	_IOR('H', 211, int)
#define HCIUARTSETPROTO	_IOW('U', 200, int)
#define HCIUARTGETPROTO	_IOR('U', 201, int)
#define HCIUARTGETDEVICE _IOR('U', 202, int)

// TTY Defines (from termios.h/ioctl)
#ifndef N_HCI
#define N_HCI	15
#endif
#ifndef TIOCSETD
#define TIOCSETD	0x5423
#endif

// UART Protocols
#define HCI_UART_H4	0
#define HCI_UART_BCSP	1
#define HCI_UART_3WIRE	2
#define HCI_UART_H4DS	3
#define HCI_UART_LL	4
#define HCI_UART_ATH3K	5
#define HCI_UART_INTEL	6
#define HCI_UART_BCM	7
#define HCI_UART_QCA	8
#define HCI_UART_AG6XX	9
#define HCI_UART_NOKIA	10
#define HCI_UART_MRVL	11

// HCI Device Info

// HCI Device Info
struct hci_dev_req {
	uint16_t dev_id;
	uint32_t dev_opt;
};

struct hci_dev_list_req {
	uint16_t dev_num;
	struct hci_dev_req dev_req[HCI_MAX_DEV];
};

// HCI Channel types
#define HCI_CHANNEL_RAW		0
#define HCI_CHANNEL_USER	1
#define HCI_CHANNEL_MONITOR	2
#define HCI_CHANNEL_CONTROL	3

struct hci_dev_stats {
	uint32_t err_rx;
	uint32_t err_tx;
	uint32_t cmd_tx;
	uint32_t evt_rx;
	uint32_t acl_tx;
	uint32_t acl_rx;
	uint32_t sco_tx;
	uint32_t sco_rx;
	uint32_t byte_rx;
	uint32_t byte_tx;
};

struct hci_dev_info {
	uint16_t dev_id;
	char     name[8];
	bdaddr_t bdaddr;
	uint32_t flags;
	uint8_t  type;
	uint8_t  features[8];
	uint32_t pkt_type;
	uint32_t link_policy;
	uint32_t link_mode;
	uint16_t acl_mtu;
	uint16_t acl_pkts;
	uint16_t sco_mtu;
	uint16_t sco_pkts;
	struct   hci_dev_stats stats;
};

struct sockaddr_hci {
	sa_family_t	hci_family;
	unsigned short	hci_dev;
	unsigned short  hci_channel;
};

struct hci_filter {
	uint32_t type_mask;
	uint32_t event_mask[2];
	uint16_t opcode;
};

// Filter Helpers
#define hci_filter_clear(f) memset((f), 0, sizeof(*(f)))
#define hci_filter_set_ptype(t, f) ((f)->type_mask |= (1 << ((t) & 31)))
#define hci_filter_set_event(e, f) ((f)->event_mask[1] |= (1 << ((e) & 31)))

// Socket Option
#define SOL_HCI		0


// Command Header
typedef struct {
	uint16_t	opcode;
	uint8_t		plen;
} __attribute__((packed)) hci_cmd_hdr;

// Event Header
typedef struct {
	uint8_t		evt;
	uint8_t		plen;
} __attribute__((packed)) hci_event_hdr;


// OGF/OCF Helpers
#define OGF_LE_CTL		0x08
#define cmd_opcode_pack(ogf, ocf) ((uint16_t) ((ocf & 0x03ff)|(ogf << 10)))

// LE Commands
#define OCF_LE_SET_SCAN_PARAMETERS 0x000B
typedef struct {
	uint8_t		type;
	uint16_t	interval;
	uint16_t	window;
	uint8_t		own_bdaddr_type;
	uint8_t		filter;
} __attribute__((packed)) le_set_scan_parameters_cp;

#define OCF_LE_SET_SCAN_ENABLE 0x000C
typedef struct {
	uint8_t		enable;
	uint8_t		filter_dup;
} __attribute__((packed)) le_set_scan_enable_cp;

#define OCF_LE_SET_ADVERTISING_PARAMETERS 0x0006
// ... TODO: Add Advertising Parameters struct

#define OCF_LE_SET_ADVERTISING_DATA 0x0008
// ... TODO: Add Advertising Data struct

#define OCF_LE_SET_ADVERTISE_ENABLE 0x000A
typedef struct {
	uint8_t		enable;
} __attribute__((packed)) le_set_advertise_enable_cp;

#ifdef __cplusplus
}
#endif

#endif // COMPAT_HCI_H
