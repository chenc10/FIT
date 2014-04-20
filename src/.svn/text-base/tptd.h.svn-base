/***************************************************************************
 *   Copyright (C) 2010 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/
#ifndef _TPTD_H_
#define _TPTD_H_

#include <sys/time.h>
#include <sys/types.h>
#include <omp.h>

#define TPTD_TRAFFIC_MODE	1

typedef  struct   _TPTD_Global_Params{
	int cores;
	int capqueue;
	int processthread;
#ifdef DEBUG
	int *pktofasyn;
#endif
	int numpackets;
	int numflows;
	int numofflat;
	int numoftokens;
#define ONLINE_MODE	1
#define OFFLINE_MODE 2
	int mode;
	omp_lock_t ipflock;
	struct timeval starttime;
	struct timeval currtime;
}TPTD_Global_Params;

typedef struct   _TPTD_Global_Counts{
	u_int64_t numpackets;
	u_int64_t numflows;
}TPTD_Global_Counts;

extern TPTD_Global_Params tptd_gparams;

#endif
