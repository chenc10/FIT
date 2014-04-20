/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/
#ifndef _MYTCP_H_
#define _MYTCP_H_

#include <sys/time.h>
#include <sys/types.h>
#include <glib.h>
#include "fft.h"

typedef struct _TPTD_TCP_Data_Store {
  int payloadlen;
  unsigned long long captime;
  char *payload;
  struct _TPTD_TCP_Data_Store *next;
}TPTD_TCP_Data_Store;


typedef struct _TPTD_Skbuff {
  void *data;
  u_int seq;
  u_int ack;
}TPTD_Skbuff;

typedef struct _TPTD_FFT_TCP_Half_Flow
{
  char state;
  u_int seq;
  u_int ack_seq;
  u_int next_seq;
  int count;
  struct _TPTD_TCP_Data_Store * sPacket;

  struct _TPTD_Skbuff *aPacket;
	// add by cc
	int already_printed;
}TPTD_FFT_TCP_Hflow;


typedef struct _TPTD_FFT_TCP_Flow
{
  struct tuple4 addr;
  struct _TPTD_FFT_TCP_Half_Flow client;
  struct _TPTD_FFT_TCP_Half_Flow server;
  struct _TPTD_FFT_TCP_Flow *next;
#define TPTD_FFT_TCP_NONE	0x0000
#define TPTD_FFT_TCP_SERVER_CLOSE	0x0001
#define TPTD_FFT_TCP_CLIENT_CLOSE	0x0002
#define TPTD_FFT_TCP_CLOSE	0x0004
#define TPTD_FFT_TCP_DELETE	0x0008
  int state;

  int detect_state;
  struct timeval first;
  struct timeval last;
  void *user;
  int del_flag;
  
  //add by yzl
}TPTD_FFT_TCP_Flow;

typedef struct _TPTD_FFT_TCP_Flow_Table{
	TPTD_FFT_TCP_Flow * flow;
	GStaticRWLock rwlock;
}TPTD_FFT_TCP_Flow_Table;




#endif
