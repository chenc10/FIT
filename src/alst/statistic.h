/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/

#ifndef _STATISTICAL_H_
#define _STATISTICAL_H_
#include <sys/types.h>

typedef struct   _TPTD_Statistic_Flows{
	u_int64_t  tcpflows;
	u_int64_t  udpflows;
}TPTD_Statistic_Flows;

typedef struct _TPTD_Statistic_Inden_TCP{
	u_int64_t  non;
}TPTD_Statistic_Inden_TCP;

typedef struct  _TPTD_Statistic_Packets{
	u_int64_t totalpackets;
	u_int64_t tcppackets;
	u_int64_t udppackets;
	u_int64_t tcpdiscard;
}TPTD_Statistic_Packets;



#endif
