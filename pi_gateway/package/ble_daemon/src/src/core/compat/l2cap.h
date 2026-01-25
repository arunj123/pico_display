#ifndef COMPAT_L2CAP_H
#define COMPAT_L2CAP_H

#include "bluetooth.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sockaddr_l2 {
	sa_family_t	l2_family;
	unsigned short	l2_psm;
	bdaddr_t	l2_bdaddr;
	unsigned short	l2_cid;
	uint8_t		l2_bdaddr_type;
};

// L2CAP Address Types
#define BDADDR_LE_PUBLIC	0x01
#define BDADDR_LE_RANDOM	0x02

// ATT CID
#define L2CAP_CID_ATT		0x0004

#ifdef __cplusplus
}
#endif

#endif // COMPAT_L2CAP_H
