/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/

#ifndef _FLAT_UDP_H_
#define _FLAT_UDP_H_

#include "nids.h"
//#include "control.h"

#include <sys/time.h>
#include <sys/types.h>
#include <glib.h>
//add by yzl
#include "fft.h"

typedef struct _TPTD_UDP_Data_Store
{
	int payloadlen;
	unsigned long long captime;
	char *payload;
	struct _TPTD_UDP_Data_Store * next;
}TPTD_UDP_Data_Store;

typedef struct _TPTD_FFT_UDP_Flow
{
  struct tuple4 addr;

  struct _TPTD_FFT_UDP_Flow *next;

  int state;
  struct timeval first;
  struct timeval last;
  #define CREATE_BY_CLIENT	1
  #define CREATE_BY_SERVER	0
  u_int8_t  is_client;
  int detect_state;
  u_int count;
  u_int server_count;
  u_int client_count;

	//add by cc
	int already_printed;
	
	struct _TPTD_UDP_Data_Store * client;
	struct _TPTD_UDP_Data_Store * server;

  void *user;
  int del_flag;
	int protocol;
  struct _TPTD_Voice_Window *c2s;
  struct _TPTD_Voice_Window *s2c;
}TPTD_FFT_UDP_Flow;

typedef struct _TPTD_FFT_UDP_Flow_Table{
	TPTD_FFT_UDP_Flow * flow;
	GStaticRWLock rwlock;
}TPTD_FFT_UDP_Flow_Table;



#endif
