/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/
 
 #include "gre.h"
 #include "alst/debug.h"
 #include "hash.h"
 #include "nids.h"
 #include "control.h"
 
 #include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 
 
TPTD_FFT_GRE_Flow_Table *tptd_fft_gre_table;
int tptd_fft_gre_table_size;

inline static int TPTD_FFT_GRE_Mkhash(int hash){
	return hash %= tptd_fft_gre_table_size;
}

inline static int TPTD_FFT_GRE_Mkhash_All(u_int32_t saddr, u_int32_t daddr){
	return mkhash_gre(saddr, daddr) % tptd_fft_gre_table_size;
}

static void TPTD_FFT_GRE_Update_Time(TPTD_Token *token, int isfirst){
	TPTD_FFT_GRE_Flow *flowptr = token->greptr;

	if(flowptr == NULL){
		TPTD_Error("TPTD_FFT_GRE_Update_Time: the flow ptr is NULL\n");
		exit(1);
	}

	struct timeval *temptime = &token->captime;

	/* if it is the first packet of the half flow. */
	if(isfirst){
		/*There may be a case that server half flow is processed first. */
		if(flowptr->first.tv_sec > temptime->tv_sec)
			flowptr->first = *temptime;
		if((flowptr->first.tv_sec==temptime->tv_sec) && (flowptr->first.tv_usec>temptime->tv_usec))
			flowptr->first = *temptime;
		flowptr->last = *temptime;
	}else{
		flowptr->last = *temptime;
	}

}

TPTD_FFT_GRE_Flow * TPTD_FFT_GRE_Find(TPTD_Token *token, int *is_client){
	int flag;
	int hash = TPTD_FFT_GRE_Mkhash_All(token->addr.saddr, token->addr.daddr);
	TPTD_FFT_GRE_Flow *flow = tptd_fft_gre_table[hash].flow;
	int rhash = TPTD_FFT_GRE_Mkhash_All(token->addr.daddr, token->addr.saddr);
	TPTD_FFT_GRE_Flow *rflow = tptd_fft_gre_table[rhash].flow;
	
	g_static_rw_lock_writer_lock(&tptd_fft_gre_table[hash].rwlock);
	g_static_rw_lock_writer_lock(&tptd_fft_gre_table[rhash].rwlock);
	
	flag = 0;
	while(flow != NULL){
		if(flow->saddr == token->addr.saddr && 
				flow->daddr==token->addr.daddr){
			flag = 1;
			break;		
		}
		flow = flow->next;
	}
	if(flag == 0) {
		g_static_rw_lock_writer_unlock(&tptd_fft_gre_table[hash].rwlock);
	}else {
		*is_client = 1;
		g_static_rw_lock_writer_unlock(&tptd_fft_gre_table[hash].rwlock);
		g_static_rw_lock_writer_unlock(&tptd_fft_gre_table[rhash].rwlock);
		return flow;
	}
	
	
	flag = 0;
	while(rflow != NULL){
		if(rflow->saddr == token->addr.daddr && 
				rflow->daddr==token->addr.saddr){
			flag = 1;
			break;		
		}
		rflow = rflow->next;
	}
	if(flag == 0) {
		g_static_rw_lock_writer_unlock(&tptd_fft_gre_table[rhash].rwlock);
	}
	else {
		*is_client = 0;
		g_static_rw_lock_writer_unlock(&tptd_fft_gre_table[hash].rwlock);
		g_static_rw_lock_writer_unlock(&tptd_fft_gre_table[rhash].rwlock);
		
		return rflow;
	}
	
	token->token_state  = TOKEN_STATE_END;
	return NULL;
}


void TPTD_FFT_GRE_Create(TPTD_Token *token){
	u_int hash;
	int is_client;
	TPTD_FFT_GRE_Flow *flow;
	TPTD_FFT_GRE_Flow *new;
	u_int saddr, daddr;
	int flag;
	
	if(token->flow_state == TOKEN_FLOW_STATE_FROMCLIENT){
		hash = TPTD_FFT_GRE_Mkhash_All(token->addr.saddr, token->addr.daddr);
		saddr = token->addr.saddr;
		daddr = token->addr.daddr;
	}
	if(token->flow_state == TOKEN_FLOW_STATE_FROMSERVER){
		hash = TPTD_FFT_GRE_Mkhash_All(token->addr.daddr, token->addr.saddr);
		saddr = token->addr.daddr;
		daddr = token->addr.saddr;
	}
	g_static_rw_lock_writer_lock(&tptd_fft_gre_table[hash].rwlock);
	
	flow = tptd_fft_gre_table[hash].flow;

	flag = 0;
	while(flow != NULL){
		if(flow->saddr == saddr && 
				flow->daddr==daddr){
			flag = 1;
			break;		
		}
		flow = flow->next;
	}
	
	if(flag == 0){
		

		new = (TPTD_FFT_GRE_Flow *) malloc(sizeof(TPTD_FFT_GRE_Flow));
		memset(new, 0, sizeof(TPTD_FFT_GRE_Flow ));
		
		if(token->flow_state == TOKEN_FLOW_STATE_FROMCLIENT){
			hash = TPTD_FFT_GRE_Mkhash_All(token->addr.saddr, token->addr.daddr);
			new->saddr = token->addr.saddr;
			new->daddr = token->addr.daddr;

		}
		if(token->flow_state == TOKEN_FLOW_STATE_FROMSERVER){
			hash = TPTD_FFT_GRE_Mkhash_All(token->addr.daddr, token->addr.saddr);
			new->saddr = token->addr.daddr;
			new->daddr = token->addr.saddr;

		}
		new->next = tptd_fft_gre_table[hash].flow;
		tptd_fft_gre_table[hash].flow = new;
		
	}
	g_static_rw_lock_writer_unlock(&tptd_fft_gre_table[hash].rwlock);	
}

void TPTD_FFT_Process_GRE (int it, TPTD_Token *token){
	int is_client;
	TPTD_FFT_GRE_Flow *flow = TPTD_FFT_GRE_Find(token, &is_client);
	//struct in_addr clientaddr;
	//clientaddr.s_addr = token->addr.saddr;
		
	//fprintf(stderr, "process %s   %d\n", inet_ntoa((struct in_addr)clientaddr), is_client);
	
	if(flow != NULL){

		if(is_client){
			token->flow_state = TOKEN_FLOW_STATE_FROMCLIENT;
		}else{
			token->flow_state = TOKEN_FLOW_STATE_FROMSERVER;
		}
		token->greptr = flow;
		token->detect_state = 1004;
		token->token_state  = TOKEN_STATE_BREAK;
		strcpy(token->suggestname, "voice");
	}else{
		token->token_state  = TOKEN_STATE_END;
		return ;
	}
}

int TPTD_FFT_GRE_Init(int size){
	int i;

	if (!size) return -1;
	tptd_fft_gre_table_size = size;
	tptd_fft_gre_table = (TPTD_FFT_GRE_Flow_Table *)calloc(size, sizeof(TPTD_FFT_GRE_Flow_Table));
	if (!tptd_fft_gre_table) {
		perror("TPTD_FFT_GRE_Init\n");
		TPTD_Err_No_Mem("TPTD_FFT_GRE_Init");
		return -1;
	}

	for(i=0; i<size; i++)
		g_static_rw_lock_init(&tptd_fft_gre_table[i].rwlock);

	DEBUG_MESSAGE(DebugMessage("TPTD_FFT_GRE_Init create fft gre flow table with size of %d\n", size););

	return 1;
}
