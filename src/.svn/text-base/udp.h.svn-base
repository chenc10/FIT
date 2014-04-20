#ifndef _UDP_H_
#define _UDP_H_


#include <omp.h>
#include <sys/time.h>
#include "nids.h"
typedef struct _TPTD_Half_UDP_Flow{

}TPTD_Half_UDP_Flow;

typedef struct _TPTD_UDP_Flow{
	int test;
	struct tuple4 addr;
	u_int64_t  hash;
	omp_lock_t lock;
	u_int16_t flag;
	u_int16_t num;
	u_int64_t first;
	u_int32_t last;
	TPTD_Half_UDP_Flow c2s;
	TPTD_Half_UDP_Flow s2c;
	struct _TPTD_UDP_Flow *next;
	void *user;
}TPTD_UDP_Flow;

#define UDP_SERVER_IP_BACKETS	128
#define UDP_CLIENT_IP_BACKETS	128
#define UDP_SERVER_PORT_BACKETS		8
#define UDP_CLIENT_PORT_BACKETS		8

extern TPTD_UDP_Flow tptd_udp_table[UDP_CLIENT_IP_BACKETS][UDP_CLIENT_PORT_BACKETS][UDP_SERVER_IP_BACKETS][UDP_SERVER_PORT_BACKETS];

#endif
