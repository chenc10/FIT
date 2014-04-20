/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/

#ifndef _CONTROL_H_
#define _CONTROL_H_
#include "nids.h"
#include "fft/mytcp.h"
#include "fft/fudp.h"
#include "fft/gre.h"


struct _TPTD_Token;


typedef struct _TPTD_Modules{
	int id;
	char name[32];
	void (* pfunction)(int it, struct _TPTD_Token *token);
	void (* dfunction)(void *data);
	struct _TPTD_Modules * next;
} TPTD_Modules;

typedef struct _TPTD_Detect_List{
	int id;
	char name[32];
	struct _TPTD_Detect_Modules * module;
	struct _TPTD_Detect_List * next;
} TPTD_Detect_List;

typedef struct _TPTD_Detect_Header{
	int id;
	int level;
	char name[32];
	struct _TPTD_Detect_List *list;
	struct _TPTD_Detect_Header *next;
} TPTD_Detect_Header;

typedef struct _TPTD_Token{
	struct tuple4 addr;
	struct tuple4 raddr;
	int protocol;
	int caplen;
	u_int32_t hash;
	u_int32_t rhash;
#define TOKEN_FLOW_STATE_FREE		0x0000
#define TOKEN_FLOW_STATE_FROMCLIENT	0x0001
#define TOKEN_FLOW_STATE_FROMSERVER	0x0002
#define TOKEN_FLOW_STATE_BLOCK		0x0004
#define TOKEN_FLOW_STATE_FORWARD	0x0008
	int flow_state;
#define TOKEN_REAS_STATE_FREE		0x0000
#define TOKEN_REAS_STATE_ING		0x0001
#define TOKEN_REAS_STATE_DONE		0x0002
	int reas_state;

	int life_state;
#define TOKEN_DETECT_STATE_SSH		1001
#define TOKEN_DETECT_STATE_GTALK	1002
#define TOKEN_DETECT_STATE_L2TP		1003
#define TOKEN_DETECT_STATE_PPTP		1004
#define TOKEN_DETECT_STATE_OPENVPN	1005
	int detect_state;
#define TOKEN_STATE_FREE	0x0000	/*Initial Value*/
#define TOKEN_STATE_NEW		0x0001  /*Get a new token*/
#define TOKEN_STATE_ALIVE	0x0002  /*As we haven't finished the detection*/
#define TOKEN_STATE_GOON   	0x0004  /*Go on for next function*/
#define TOKEN_STATE_BREAK  	0x0008  /*As needing to change detect rule chain*/
#define TOKEN_STATE_END    	0x0010  /*Something abnormal happens, like pkg err or flow end*/
	int token_state;
#define TOKEN_LOCK_STATE_FREE		0x0000
#define TOKEN_LOCK_STATE_WRITE		0x0001
#define TOKEN_LOCK_STATE_WRITE_R	0x0002
#define TOKEN_LOCK_STATE_READ		0x0004
#define TOKEN_LOCK_STATE_TWOWRITE	0x0008
	int lock_state;
#ifdef FFT
	TPTD_FFT_TCP_Flow *tcpptr;
	TPTD_FFT_TCP_Flow_Table *tcplockptr;
	TPTD_FFT_TCP_Flow_Table *rtcplockptr;
	TPTD_FFT_UDP_Flow *udpptr;
	TPTD_FFT_GRE_Flow *greptr;
	TPTD_FFT_UDP_Flow_Table *udplockptr;
	TPTD_FFT_UDP_Flow_Table *rudplockptr;
#endif
#ifdef HFT
	
#endif
	struct _TPTD_Detect_Header *header;
	void *data;
	char suggestname[32];
	struct timeval captime;
	int test;
}TPTD_Token;

extern void TPTD_Init_Control(int);

extern TPTD_Token **tokens;


#endif
