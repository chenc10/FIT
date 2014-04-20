/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/
 
 #ifndef _GRE_H_
 #define _GRE_H_
 
#include <sys/time.h>
#include <sys/types.h>
#include <glib.h>
 
 typedef struct _TPTD_FFT_GRE_Flow{
 	u_int32_t saddr;
 	u_int32_t daddr;
 	struct timeval first;
    struct timeval last;
 	struct _TPTD_Voice_Window *c2s;
  	struct _TPTD_Voice_Window *s2c;
 	struct _TPTD_FFT_GRE_Flow * next;
 }TPTD_FFT_GRE_Flow;
 
 typedef struct _TPTD_FFT_GRE_Flow_Table{
	TPTD_FFT_GRE_Flow * flow;
	GStaticRWLock rwlock;
}TPTD_FFT_GRE_Flow_Table;
 #endif
