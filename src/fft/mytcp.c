/***************************************************************************
 *   Copyright (C 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/
#include <config.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <math.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>



#include "checksum.h"
#include "util.h"
#include "nids.h"
#include "hash.h"
#include "alst/debug.h"
#include "control.h"
#include "fft.h"
#include "mytcp.h"
#include "tptd.h"

#define TPTD_FFT_UNLOCK_TCP_WRITER(token) \
		if(token->lock_state == TOKEN_LOCK_STATE_TWOWRITE){\
			if(token->tcplockptr != token->rtcplockptr){\
				g_static_rw_lock_writer_unlock(&token->tcplockptr->rwlock);\
				g_static_rw_lock_writer_unlock(&token->rtcplockptr->rwlock);\
			}else g_static_rw_lock_writer_unlock(&token->tcplockptr->rwlock);\
		} if(token->lock_state == TOKEN_LOCK_STATE_WRITE) g_static_rw_lock_writer_unlock(&token->tcplockptr->rwlock);\
		token->lock_state = TOKEN_LOCK_STATE_FREE;

enum
{
  TPTD_TCP_ESTABLISHED = 1,
  TPTD_TCP_SYN_SENT,
  TPTD_TCP_SYN_RECV,
  TPTD_TCP_FIN_WAIT1,
  TPTD_TCP_FIN_WAIT2,
  TPTD_TCP_TIME_WAIT,
  TPTD_TCP_CLOSE,
  TPTD_TCP_CLOSE_WAIT,
  TPTD_TCP_LAST_ACK,
  TPTD_TCP_LISTEN,
  TPTD_TCP_CLOSING,   /* now a valid state */
  TPTD_TCP_CLIENT_DATA,
  TPTD_TCP_SERVER_DATA
};

static TPTD_FFT_TCP_Flow_Table *tptd_fft_tcp_table;
static int tptd_fft_tcp_table_size;
static struct ip *ugly_iphdr;

inline static int TPTD_FFT_TCP_Mkhash(int hash){
	return hash %= tptd_fft_tcp_table_size;
}

inline static int TPTD_FFT_TCP_Mkhash_All(struct tuple4 *addr){
	return mkhash(addr->saddr, addr->source, addr->daddr, addr->dest)
				% tptd_fft_tcp_table_size;
}

inline static int TPTD_FFT_TCP_Compare(struct tuple4 *taddr, struct tuple4 *baddr){
	if((taddr->dest==baddr->dest) &&
	   (taddr->saddr==baddr->saddr) &&
	   (taddr->source==baddr->source) &&
	   (taddr->daddr==baddr->daddr))
	return 1;
	else
	return 0;
}
inline static void TPTD_FFT_TCP_Assign(struct tuple4 *taddr, struct tuple4 *saddr){
	taddr->dest = saddr->dest;
	taddr->saddr = saddr->saddr;
	taddr->source = saddr->source;
	taddr->daddr = saddr->daddr;
	return;
}

/****************************************************************************
 *
 * Function: TPTD_FFT_TCP_Update_Time
 *
 * Purpose: Update time of flow.
 *
 * Arguments: TPTD_Token *token, the pointer of token got from token pool.
 *            int isfirst, to show if this packet is the first one of the flow.
 *
 * Returns: void
 *
 ****************************************************************************/
static void TPTD_FFT_TCP_Update_Time(TPTD_Token *token, int isfirst){
	TPTD_FFT_TCP_Flow *flowptr = token->tcpptr;

	if(flowptr == NULL){
		TPTD_Error("TPTD_FFT_TCP_Update_Time: the flow ptr is NULL\n");
		exit(1);
	}

	struct timeval temptime;
	gettimeofday( &temptime,NULL);
	/* if it is the first packet of the half flow. */
 /*   if(isfirst)
    {
		//There may be a case that server half flow is processed first. 
		//add by yzl
		if(flowptr->first.tv_sec == 0)
		    flowptr->first = *temptime;
        if(flowptr->first.tv_sec > temptime->tv_sec)
            flowptr->first = *temptime;
        if((flowptr->first.tv_sec==temptime->tv_sec) && (flowptr->first.tv_usec>temptime->tv_usec))
            flowptr->first = *temptime;
		flowptr->last = *temptime;
	}
	else
	{
		flowptr->last = *temptime;
	}
*/
	// add by cc, real time used
	if(isfirst)
	{
		flowptr->first = temptime;
		flowptr->last = temptime;
	}
	else
		flowptr->last = temptime;
}

/****************************************************************************
 *
 * Function: TPTD_FFT_TCP_Find
 *
 * Purpose: Find a netflow from flow table.
 *
 * Arguments: TPTD_Token *token, the pointer of token got from token pool.
 *            int *from_client, to record the flow state
 *
 * Returns: int . 1 for finding one, 0 for nothing
 *
 ****************************************************************************/

int TPTD_FFT_TCP_Find(TPTD_Token *token, int *from_client){
	/*
	 * If a flow record is found in flow table, this function will
	 * lock one bucket, and store the lock pointer and flow pointer
	 * in "tcplockptr" and "tcpptr" respectively. If no flow record
     * found, this function will lock the buckets of both directions,
	 * and lock them in "tcplockptr" and "rtcplockptr" respectively.
	 */
//add by cc
//include COPY_BUFFER inside Find
	TPTD_FFT_TCP_Flow_Table *lockptr, *rlockptr;
	TPTD_FFT_TCP_Flow *tfptr, *rtfptr;
	int hash, rhash;

	rhash = token->rhash;
	hash = token->hash;

	/*lock both directions? fix me -Dawei*/
	lockptr = &tptd_fft_tcp_table[hash];
	rlockptr = &tptd_fft_tcp_table[rhash];
	/*let's try to lock the bucket of non-reverse direction*/
FFT_TCP_RELEASE:
	g_static_rw_lock_writer_lock(&lockptr->rwlock);
	if(lockptr != rlockptr){
		double pro;
		int step = 0;
		/*try to lock the bucket of reverse direction*/
		while(FALSE == g_static_rw_lock_writer_trylock(&rlockptr->rwlock)){
			pro = pow(M_E,(double)(step-5)/1.2);
			if(pro > (double)rand()/RAND_MAX){
				g_static_rw_lock_writer_unlock(&lockptr->rwlock);
				goto FFT_TCP_RELEASE;
			}
			else
				step++;
		}
	}
	/*
	 * We use "tcplockptr" and "rtcplockptr" to remember the
	 * pointer of two directions. If nothing found, we will use
	 * these two pointer to create a new flow record.
	 */
	token->lock_state = TOKEN_LOCK_STATE_TWOWRITE;
	token->tcplockptr = lockptr;
	token->rtcplockptr = rlockptr;
	token->tcpptr = NULL;

	/*check the current direction first.*/
	tfptr = lockptr->flow;
	while(tfptr != NULL){
		//fprintf(stderr,"tfptr != NULL\n");
		/*
		 * If found, we copy the lock and flow pointer to
		 * "tcplockptr and "tcpptr" respectively.
		 * Then unlock the "rlockptr", as well as change the lock state
         */
		if(TPTD_FFT_TCP_Compare(&token->addr, &tfptr->addr)){
			token->tcplockptr = lockptr;
			token->tcpptr = tfptr;
			g_static_rw_lock_writer_unlock(&rlockptr->rwlock);
			token->lock_state = TOKEN_LOCK_STATE_WRITE;
			//token->tcpptr->client.count++;
			*from_client = 1;
			TPTD_TCP_COPY_BUFFER(token,1);
			if(token->tcpptr->del_flag == TPTD_READY_DEL)
                token->token_state = TOKEN_STATE_FREE;
			fprintf(stderr,"we find flow\n");
			return 1;
		}else{
			tfptr = tfptr->next;
		}
	}
	/*check the reversed direction then.*/
	rtfptr = rlockptr->flow;
	while(rtfptr != NULL){
		fprintf(stderr,"rtfptr != NULL\n");
		/*
		 * If found, we also copy the lock and flow pointer to
		 * "tcplockptr and "tcpptr" respectively.
		 * Then unlock the "lockptr", as well as change the lock state
         */
		if(TPTD_FFT_TCP_Compare(&token->raddr, &rtfptr->addr)){
			token->tcplockptr = rlockptr;
			token->tcpptr = rtfptr;
			token->lock_state = TOKEN_LOCK_STATE_WRITE;
			g_static_rw_lock_writer_unlock(&lockptr->rwlock);
			//token->tcpptr->server.count++;
			*from_client = 0;
			TPTD_TCP_COPY_BUFFER(token,0);
			if(token->tcpptr->del_flag == TPTD_READY_DEL)
                token->token_state = TOKEN_STATE_FREE;
			fprintf(stderr,"we find flow\n");
			return 1;
		}else{
			rtfptr = rtfptr->next;
		}
	}

	return 0;
}

/****************************************************************************
 *
 * Function: TPTD_FFT_TCP_Add_New_Flow
 *
 * Purpose: Insert a netflow record into tcp flow table.
 *
 * Arguments: TPTD_Token *token, the pointer of token got from token pool.
 *            struct tcphdr *this_tcphdr, the tcp header pointer
 *            int from_client, to mark the flow state
 *
 * Returns: void
 *
 ****************************************************************************/


static void TPTD_FFT_TCP_Add_New_Flow(TPTD_Token *token, struct tcphdr *this_tcphdr, int from_client){
	fprintf(stderr,"Add_New_Flow: %d\n", from_client);
	TPTD_FFT_TCP_Flow *newflow;
	TPTD_FFT_TCP_Flow_Table *ftptr;

	newflow = (TPTD_FFT_TCP_Flow *)malloc(sizeof(TPTD_FFT_TCP_Flow));
	if(newflow == NULL){
		TPTD_Err_No_Mem("TPTD_FFT_TCP_Add_New_Flow");
		exit(1);
	}
	memset(newflow, 0, sizeof(TPTD_FFT_TCP_Flow));
	newflow->client.already_printed = 0;
	newflow->server.already_printed = 0;

	if(from_client){
		TPTD_FFT_TCP_Assign(&newflow->addr, &token->addr);
		newflow->client.state = TPTD_TCP_SYN_SENT;
		newflow->client.seq = ntohl(this_tcphdr->th_seq);
		newflow->client.next_seq = newflow->client.seq+1;
		newflow->server.state = TPTD_TCP_CLOSE;
		ftptr = token->tcplockptr;
		if(ftptr->flow == NULL){
			ftptr->flow = newflow;
		}else{
			newflow->next = ftptr->flow;
			ftptr->flow = newflow;
		}
		token->tcpptr=token->tcplockptr->flow;
		token->tcpptr->client.sPacket = NULL;
		
	}else{
		TPTD_FFT_TCP_Assign(&newflow->addr, &token->raddr);
    		newflow->server.state = TPTD_TCP_SYN_RECV;
    		newflow->server.seq = ntohl(this_tcphdr->th_seq);
		newflow->server.next_seq = newflow->server.seq+1;
    		newflow->server.ack_seq = ntohl(this_tcphdr->th_ack);
		ftptr = token->rtcplockptr;
		if(ftptr->flow == NULL){
			ftptr->flow = newflow;
		}else{
			newflow->next = ftptr->flow;
			ftptr->flow = newflow;
		}
		token->tcpptr=token->rtcplockptr->flow;
		token->tcpptr->server.sPacket = NULL;
	}

}

/****************************************************************************
 *
 * Function: TPTD_FFT_TCP_Del_Flow
 *
 * Purpose: Delete a tcp flow from flow table.
 *
 * Arguments: TPTD_Token *token, the pointer of token got from token pool.
 *
 * Returns: void
 *
 ****************************************************************************/

static void TPTD_FFT_TCP_Del_Flow(TPTD_Token *token,int from_client){
	fprintf(stderr," enter Del_FLow\n");
	TPTD_FFT_TCP_Flow *flowptr = token->tcpptr;
	TPTD_FFT_TCP_Flow_Table *lockptr;
	TPTD_FFT_TCP_Flow *tmp;
	//add by yzl
	if(from_client == 1)
        lockptr = token->tcplockptr;
    else
        lockptr = token->rtcplockptr;

	if(lockptr->flow == NULL){
		TPTD_Error("TPTD_FFT_TCP_Del_Flow: the flow ptr is NULL\n");
		return;//exit(1);
	}

	if(lockptr->flow == flowptr){
		lockptr->flow = flowptr->next;
	}else{
		tmp = lockptr->flow;
		while((tmp->next!=flowptr) && (tmp->next!=NULL))
			tmp = tmp->next;
		tmp->next = flowptr->next;
	}
	token->tcpptr == NULL;
	free(flowptr);
}

int TPTD_TCP_COPY_BUFFER(TPTD_Token *token,int client)
{
/*
	add by cc
	this function can only be cited inside TCP_Process and Find
*/
	fprintf(stderr,"enter TPTD_TCP_COPY_BUFFER\n");
	//fprintf(stderr," %d %d %d %d\n", token->tcpptr->addr.source, token->tcpptr->addr.dest, token->tcpptr->addr.saddr, token->tcpptr->addr.daddr);
    	unsigned char *data = token->data;
    	struct ip *this_iphdr = (struct ip *)data;
    	struct tcphdr *this_tcphdr = (struct tcphdr *)(data + 4 * this_iphdr->ip_hl);
    	int datalen, iplen;
    	unsigned char *puredata;
    	iplen = ntohs(this_iphdr->ip_len);
    	datalen = iplen - 4 * this_iphdr->ip_hl - 4 * this_tcphdr->th_off;
    	if(datalen<0)
        return 0;
    	TPTD_TCP_Data_Store *newpacket;
    	TPTD_TCP_Data_Store **packet;
	//fprintf(stderr," before enter %d %d %d %d\n", token->tcpptr->addr.source, token->tcpptr->addr.dest, token->tcpptr->addr.saddr, token->tcpptr->addr.daddr);

    	if(client)
    	{
        	if(/*token->tcpptr->client.count < MINI_PACKETS_NUMBER&&*/
            		((token->tcpptr->client.state == TPTD_TCP_ESTABLISHED)||
             		(token->tcpptr->client.state == TPTD_TCP_CLIENT_DATA)))
        	{
            	/*token->tcpptr->client.payloadlen[token->tcpptr->client.count] = datalen;
            	token->tcpptr->client.captime[token->tcpptr->client.count] = token->captime.tv_usec+token->captime.tv_sec*1000000;
            	token->tcpptr->client.count++;*/
            	newpacket = (TPTD_TCP_Data_Store *)malloc(sizeof(TPTD_TCP_Data_Store));
            	if(newpacket == NULL)
            	{
                	TPTD_Err_No_Mem("TPTD_FFT_TCP_Add_New_PACKET");
                	exit(1);
            	}
            	memset(newpacket, 0, sizeof(TPTD_TCP_Data_Store));

            	newpacket->payloadlen = datalen;
           	//newpacket->captime = token->captime.tv_usec+token->captime.tv_sec*1000000;
            	newpacket->payload = (char *)malloc(datalen+1);
            	memset(newpacket->payload, 0, sizeof(datalen+1));
            	puredata=data + 4 * this_iphdr->ip_hl + 4 * this_tcphdr->th_off;
            	memcpy(newpacket->payload,puredata,datalen);
		/*if(token->tcpptr->client.sPacket == NULL)
            
		{
                	token->tcpptr->client.sPacket = newpacket;
           	}
            	else
            	{
             	   newpacket->next = ftoken->tcpptr->client.sPacket;
            	    token->tcpptr->client.sPacket = newpacket;
            	}
           	token->tcpptr=token->tcplockptr->flow;*/
           	packet = &(token->tcpptr->client.sPacket);
           	while((*packet) != NULL)
            	{
			fprintf(stderr,"3M: client1\n");
			fprintf(stderr,"ww\n");
			if((*packet)->next == NULL)
				fprintf(stderr,"(*packet)->next == NULL\n");
			fprintf(stderr,"ww0\n");
            		(packet)  = &((*packet)->next);
			fprintf(stderr,"ww1\n");
            	}
		//add by cc: ?? It's strange here about the order of "newpacket->next = NULL" and "(*packet) = newpacket", the executing result is quite different  
		//newpacket->next = NULL;
            	(*packet) = newpacket;
	    	newpacket->next = NULL;
            	token->tcpptr->client.count++;
		fprintf(stderr,"finish client copy buffer\n");
        }

    	}
	else
   	{
		//fprintf(stderr," now enter%d %d %d %d\n", token->tcpptr->addr.source, token->tcpptr->addr.dest, token->tcpptr->addr.saddr, token->tcpptr->addr.daddr);
		//fprintf(stderr,"1A\n");
        	newpacket = (TPTD_TCP_Data_Store *)malloc(sizeof(TPTD_TCP_Data_Store));
        	if(
            		((token->tcpptr->client.state == TPTD_TCP_ESTABLISHED)||
             		(token->tcpptr->server.state == TPTD_TCP_SYN_RECV)||
             		(token->tcpptr->server.state == TPTD_TCP_SERVER_DATA)))
        	{
		//fprintf(stderr," enter if A%d %d %d %d\n", token->tcpptr->addr.source, token->tcpptr->addr.dest, token->tcpptr->addr.saddr, token->tcpptr->addr.daddr);
            	/* token->tcpptr->server.payloadlen[token->tcpptr->server.count] = datalen;
             	token->tcpptr->server.captime[token->tcpptr->server.count] = token->captime.tv_usec+token->captime.tv_sec*1000000;
             	token->tcpptr->server.count++;*/
            	//newpacket = (TPTD_TCP_Data_Store *)malloc(sizeof(TPTD_TCP_Data_Store));
		//fprintf(stderr," enter if B%d %d %d %d\n", token->tcpptr->addr.source, token->tcpptr->addr.dest, token->tcpptr->addr.saddr, token->tcpptr->addr.daddr);
		//fprintf(stderr,"1B\n");
		//fprintf(stderr," %d %d %d %d\n", token->tcpptr->addr.source, token->tcpptr->addr.dest, token->tcpptr->addr.saddr, token->tcpptr->addr.daddr);
            	if(newpacket == NULL)
            	{
             	   TPTD_Err_No_Mem("TPTD_FFT_TCP_Add_New_PACKET");
            	    exit(1);
            	}
            	memset(newpacket, 0, sizeof(TPTD_TCP_Data_Store));

            	newpacket->payloadlen = datalen;
            	//newpacket->captime = token->captime.tv_usec+token->captime.tv_sec*1000000;

            	newpacket->payload = (char *)malloc(datalen+1);
            	memset(newpacket->payload, 0, sizeof(datalen+1));
            	puredata=data + 4 * this_iphdr->ip_hl + 4 * this_tcphdr->th_off;
            	memcpy(newpacket->payload,puredata,datalen);
	    //fprintf(stderr,"2M %d %d %d %d\n", token->tcpptr->addr.source, token->tcpptr->addr.dest, token->tcpptr->addr.saddr, token->tcpptr->addr.daddr);
	    	packet = &(token->tcpptr->server.sPacket);
            	while((*packet) != NULL)
           	 {
		fprintf(stderr,"1M %d %d %d %d\n", token->tcpptr->addr.source, token->tcpptr->addr.dest, token->tcpptr->addr.saddr, token->tcpptr->addr.daddr);
                	(packet)  = &((*packet)->next);
            	}
		newpacket->next = NULL;
            	(*packet) = newpacket;
		(*packet)->next = NULL;
            	token->tcpptr->server.count++;
        }

    }
	fprintf(stderr,"finish TPTD_TCP_COPY_BUFFER\n");
    return 1;
}



/****************************************************************************
 *
 * Function: TPTD_FFT_TCP_Timeout
 *
 * Purpose: Delete all the timeout flow. Called by timeout thread.
 *
 * Arguments: NULL
 *
 * Returns: void
 *
 ****************************************************************************/
void TPTD_FFT_TCP_Timeout(int *process_finished){
	TPTD_FFT_TCP_Flow_Table *fft = tptd_fft_tcp_table;
	int i;
	TPTD_FFT_TCP_Flow *flow, *delpoint, *prepoint;
	struct timeval now;
	int firstval, lastval;
	*process_finished = 0;
	//if(tptd_gparams.mode==OFFLINE_MODE)
	//	now = tptd_gparams.currtime;
	//else
	gettimeofday(&now, NULL);
	fprintf(stderr,"enter Timeout\n");
	for(i=0; i<tptd_fft_tcp_table_size; i++){
		g_static_rw_lock_writer_lock(&fft[i].rwlock);
		flow = fft[i].flow;
		prepoint = flow;
		while(flow != NULL)
		{
			*process_finished = 1;
			firstval = now.tv_sec-flow->first.tv_sec;
			lastval = now.tv_sec-flow->last.tv_sec;
			fprintf(stderr,"firstval: %d, lastval: %d\n", firstval, lastval);
			if(firstval > 2000 || lastval > 1 || flow->state == TPTD_FFT_TCP_DELETE ||((flow->state==TPTD_FFT_TCP_SERVER_CLOSE || flow->state==TPTD_FFT_TCP_CLIENT_CLOSE) && lastval>20))
			{
				fprintf(stderr,"Timeout true\n");
				delpoint = flow;
				//if(delpoint->server.already_printed == 0 || delpoint->client.already_printed == 0)
				//different from udp, there is no need for check already_printed since all the printed flow will not appear here.
				TPTD_TCP_PRINTFLOW(delpoint,0);
				TPTD_TCP_PRINTFLOW(delpoint,1);
				flow = flow->next;
				if(fft[i].flow == delpoint)
				{//offline_mode + enough time -> always this branch
					fft[i].flow = delpoint->next;
				}
				else
				{
					while(prepoint->next!=delpoint)
						prepoint = prepoint->next;
					prepoint->next = delpoint->next;
				}
				//add by yzl
				if(delpoint->del_flag == TPTD_UNREADY_DEL)
                   			 delpoint->del_flag = TPTD_READY_DEL;
                		else
                		{// unify the way to del a flow? inter free Or only a total free?
                   			if(delpoint->client.aPacket != NULL)
                        			free(delpoint->client.aPacket);
                    			if(delpoint->server.aPacket != NULL)
                       				free(delpoint->server.aPacket);
		
					if(delpoint->client.sPacket != NULL)
						free(delpoint->client.sPacket);
					if(delpoint->server.sPacket != NULL)												free(delpoint->server.sPacket);	
                    			free(delpoint);
                    			delpoint = NULL;
                		}

				//continue;
			}
			else
				flow = flow->next;
		}
		g_static_rw_lock_writer_unlock(&fft[i].rwlock);
	}
}


/****************************************************************************
 *
 * Function: TPTD_FFT_TCP_Need_Ressameble
 *
 * Purpose: To check if the current flow need reassemble. This function
 *          actually realize the quit strategy, becuase in most situation I donot want
 *			keep flow with whole life, by contraries we only reassemble and detect the first
 *	        several packets, due to the limited memory and cpu resources.
 *
 * Arguments: TPTD_Token *token, the pointer of token got from token pool.
 *
 * Returns: int
 *
 ****************************************************************************/

static int TPTD_FFT_TCP_Need_Ressameble(TPTD_Token *token){
	return 1;
}

/****************************************************************************
 *
 * Function: TPTD_FFT_Process_TCP
 *
 * Purpose: Flat TCP Flow Table.
 *
 * Arguments: int it, the ID of processing thread.
 *            TPTD_Token *token, the pointer of token got from token pool.
 *
 * Description: This function has the ability to process both online and offline data.
 * 			    Because the online data has the round trip delay, its processing is much
 *		        simple. However, the offline may has some problem. Due to no round trip delay,
 * 				the two directions of a flow may be processed at the same time.
 * 			    PS: offline data not only refer to the offline dataset, but also some
 *				application context that we can only process the half flow.
 *				In addtion, this function also reassemble TCP flow.
 *
 * Returns: void
 *
 ****************************************************************************/
int count= 0;
int count1=0;

//FIXME: I think the traffic classifcation system need to identify the protocol using
//		 only first several packets. The most important of this system is processing performance.
//		 So the TCP-reassemble of this function is very simple.
//		 We only reassemble the flow using seq_num. Moreover, we only reassemble the first N packet.
//		 If a time-out resent packet comes, we only ignore it.
void TPTD_FFT_Process_TCP(int it, TPTD_Token *token){
	fprintf(stderr,"enter TPTD_FFT_Process_TCP:main\n");
	char *data = token->data;
	struct ip *this_iphdr = (struct ip *)data;
	struct tcphdr *this_tcphdr = (struct tcphdr *)(data + 4 * this_iphdr->ip_hl);
	int datalen, iplen;
	int from_client = 1;
	TPTD_FFT_TCP_Flow *a_tcp;
	TPTD_FFT_TCP_Hflow *snd, *rcv;
	u_int a_seq;
	u_int a_next_seq;
	TPTD_FFT_TCP_Hflow *a_half;
	iplen = ntohs(this_iphdr->ip_len);
	datalen = iplen - 4 * this_iphdr->ip_hl - 4 * this_tcphdr->th_off;

	/*
	 * Firstly, we search tcp flow table according to the flow state in token
	 */
	switch(token->flow_state){
		case TOKEN_FLOW_STATE_FREE:
			fprintf(stderr,"enter TOKEN_FLOW_STATE_FREE\n");
		/*
		 * This token said that the flow is fresh. OK, search the flow table.
		 */
AGAIN:
			/*calculate the hash values of both the current and reversed direction*/
			token->hash = TPTD_FFT_TCP_Mkhash(token->hash);
			token->raddr.source = token->addr.dest;
			token->raddr.dest = token->addr.source;
			token->raddr.saddr = token->addr.daddr;
			token->raddr.daddr = token->addr.saddr;
			token->rhash = TPTD_FFT_TCP_Mkhash_All(&token->raddr);

			if (!TPTD_FFT_TCP_Find(token, &from_client)) {
				/*
				 * If nothing is found, maybe we should create one.
				 */
				if ((this_tcphdr->th_flags & TH_SYN) &&
						!(this_tcphdr->th_flags & TH_ACK) &&
						!(this_tcphdr->th_flags & TH_RST)){
					/*
					 * If the current packet is the first one from client,
					 * we create a new flow record, and update the info of client.
					 */
					TPTD_FFT_TCP_Add_New_Flow(token, this_tcphdr, 1);
					/*Update time*/
					TPTD_FFT_TCP_Update_Time(token, 1);
					token->flow_state = TOKEN_FLOW_STATE_FROMCLIENT;
				fprintf(stderr,"fromclient ++\n");
					TPTD_Statistic_Flow_TCP_Incr1();
					token->tcpptr->client.count++;
					token->token_state = TOKEN_STATE_GOON;
					TPTD_FFT_UNLOCK_TCP_WRITER(token);
					return;
				}

				if((this_tcphdr->th_flags & TH_SYN) &&
						 (this_tcphdr->th_flags & TH_ACK) &&
						!(this_tcphdr->th_flags & TH_RST)){
					/*
					 * If the current packet is the first one from server,
					 * we create a new flow record, and update the info of server.
					 */
					TPTD_FFT_TCP_Add_New_Flow(token, this_tcphdr, 0);
					/*Update time*/
					TPTD_FFT_TCP_Update_Time(token, 1);
				fprintf(stderr,"fromserver ++\n");
					token->flow_state = TOKEN_FLOW_STATE_FROMSERVER;
					TPTD_Statistic_Flow_TCP_Incr1();
					token->tcpptr->server.count++;
					token->token_state = TOKEN_STATE_GOON;
					TPTD_FFT_UNLOCK_TCP_WRITER(token);
					return;
				}
				token->token_state = TOKEN_STATE_END;
				TPTD_FFT_UNLOCK_TCP_WRITER(token);
				return;
			}
			else if(token->token_state == TOKEN_STATE_FREE)
			{
			    TPTD_FFT_UNLOCK_TCP_WRITER(token);
			    return;
			}

			break;
		case TOKEN_FLOW_STATE_FROMCLIENT:
		fprintf(stderr,"enter TOKEN_FLOW_STATE_FROMCLIENT\n");
		/*
		 * The token show that this packet is the one from client.
		 * We check the bucket of flow table at first. If exits, update this bucket.
		 * If not, we turn back to TOKEN_FLOW_STATE_FREE to create one.
		 */
		 //add by yzl
            if(!token->tcpptr->del_flag)
            {
                if(token->tcplockptr && token->tcpptr){
                    /*Lock first*/
                    if(token->lock_state == TOKEN_LOCK_STATE_FREE){
                        g_static_rw_lock_writer_lock(&token->tcplockptr->rwlock);
                        token->lock_state = TOKEN_LOCK_STATE_WRITE;
			TPTD_TCP_COPY_BUFFER(token,1);
                        //token->tcpptr->client.count++;
                    }
                    /*if the current bucket is not the true one, return to find*/
                    if(!TPTD_FFT_TCP_Compare(&token->addr, &token->tcpptr->addr)){
                        g_static_rw_lock_writer_unlock(&token->tcplockptr->rwlock);	
                        token->lock_state = TOKEN_LOCK_STATE_FREE;
                        goto AGAIN;
                    }
                }else
                    goto AGAIN;
            }
            else
            {
                token->token_state = TOKEN_STATE_FREE;
                TPTD_FFT_UNLOCK_TCP_WRITER(token);
            }
		fprintf(stderr,"leave TOKEN_FLOW_STATE_FROMCLIENT\n");
            break;

        case TOKEN_FLOW_STATE_FROMSERVER:
		fprintf(stderr,"enter TOKEN_FLOW_STATE_FROMSERVER\n");
		fprintf(stderr,"%d %d %d %d\n",token->addr.saddr, token->addr.source, token->addr.daddr, token->addr.dest);
		fprintf(stderr,"token->tcpptr  %d %d %d %d\n", token->tcpptr->addr.saddr, token->tcpptr->addr.source, token->tcpptr->addr.daddr, token->tcpptr->addr.dest);
		fprintf(stderr,"TOKEN_FLOW_STATE_FROMSERVER\n");
            //add by yzl
            if(!token->tcpptr->del_flag)
            {
                if(token->tcplockptr && token->tcpptr){
                    /*Lock first*/
                    if(token->lock_state == TOKEN_LOCK_STATE_FREE){
                        g_static_rw_lock_writer_lock(&token->tcplockptr->rwlock);
                        token->lock_state = TOKEN_LOCK_STATE_WRITE;
		fprintf(stderr," %d %d %d %d\n", token->tcpptr->addr.source, token->tcpptr->addr.dest, token->tcpptr->addr.saddr, token->tcpptr->addr.daddr);
			TPTD_TCP_COPY_BUFFER(token,0);
                        //token->tcpptr->server.count++;
                    }
                    /*if the current bucket is not the true one, return to find*/
                    if(!TPTD_FFT_TCP_Compare(&token->raddr, &token->tcpptr->addr)){
                        g_static_rw_lock_writer_unlock(&token->tcplockptr->rwlock);
                        token->lock_state = TOKEN_LOCK_STATE_FREE;
                        goto AGAIN;
                    }
                    from_client = 0;
                } else goto AGAIN;
            }
            else
            {
                token->token_state = TOKEN_STATE_FREE;
                TPTD_FFT_UNLOCK_TCP_WRITER(token);
            }
		fprintf(stderr,"leave TOKEN_FLOW_STATE_FROMSERVER\n");
            break;
    }

	a_tcp = token->tcpptr;
	token->detect_state = token->tcpptr->detect_state;
	if (from_client) {
		token->flow_state = TOKEN_FLOW_STATE_FROMCLIENT;
		snd = &a_tcp->client;
		rcv = &a_tcp->server;
	}else {
		fprintf(stderr," atcp TOKEN_FLOW_STATE_FROMSERVER\n");
		token->flow_state = TOKEN_FLOW_STATE_FROMSERVER;
		fprintf(stderr,"token->tcpptr  %d %d %d %d\n", token->tcpptr->addr.saddr, token->tcpptr->addr.source, token->tcpptr->addr.daddr, token->tcpptr->addr.dest);
		rcv = &a_tcp->client;
		snd = &a_tcp->server;
	}
	/*
	 * As we receive the second SYN packet.
	 * PS: This packet may come from both client and server.
	 */
	if ((this_tcphdr->th_flags & TH_SYN)) {

		/*
		 * It comes from client
		 */
		if (from_client){
			/* A start of client half flow, let's update some state*/
			//FIXME: Here is a problem. Sometimes, a client maybe send several
			//       SYN packets, because the previous SYN packets does not get
			//       a response from server. However, as the first SYN packets is
			//       sent, a flow record has been created. If we ignore the following
			//       SYN packets, the program seems wrong, However, if we continue
			//       to process the following SYN packets, the system may be hurt by
			//		 SYN flooding. Maybe adding an flag in tcp flow is a good idea,
			//       But, I don't want to add anything into it. So far, it is too big.
			//       Damn it, maybe it is the task of IDS.
			/* If so, we just give a warning and return. -_-| */
			if(a_tcp->client.state == TPTD_TCP_SYN_SENT){
				//TPTD_Warning_General("Several SYN packets is received by client\n");
				token->token_state = TOKEN_STATE_GOON;
				TPTD_FFT_UNLOCK_TCP_WRITER(token);
				return ;
			}
			/* If the state of server is TPTD_TCP_SERVER_DATA or _RECV, discard this packet */
			if(a_tcp->client.state == TPTD_TCP_ESTABLISHED ||
				a_tcp->client.state == TPTD_TCP_CLIENT_DATA){
				//TPTD_Warning_General("Client sent a abnormal SYN packet\n");
				fprintf(stderr,"from client and A abnormal SYN come\n");
				//errors for fromserver may happen
				//TPTD_FFT_TCP_Del_Flow(token,from_client);
				token->token_state = TOKEN_STATE_END;
				TPTD_FFT_UNLOCK_TCP_WRITER(token);
				return ;
			}
			/*
			 * OK then, it seems that we have receive the first
			 * packet of client as well as the server, updating....
			 */
			if(a_tcp->client.state==0 &&
				(a_tcp->server.state == TPTD_TCP_SYN_RECV ||
				a_tcp->server.state ==TPTD_TCP_SERVER_DATA)){
				a_tcp->client.state = TPTD_TCP_SYN_SENT;
				a_tcp->client.seq = ntohl(this_tcphdr->th_seq);
				a_tcp->client.next_seq = a_tcp->client.seq+1;
				token->token_state = TOKEN_STATE_GOON;
				token->flow_state = TOKEN_FLOW_STATE_FROMCLIENT;
				TPTD_FFT_UNLOCK_TCP_WRITER(token);
				return ;
			}
		}
		/*
	 	 * It comes from server.
	 	 */
		else{
			/*
		 	 * Maybe, several SYN packets can be sent by server.
			 * We need be careful.
		 	 */
			if(a_tcp->server.state == TPTD_TCP_SYN_RECV){
				//TPTD_Warning_General("A abnormal SYN packet is sent by server\n");
				fprintf(stderr,"from server and A abnormal SYN packet\n");
				// errors may occur if flow are delete, by cc
				//TPTD_FFT_TCP_Del_Flow(token,from_client);
				token->token_state = TOKEN_STATE_END;
				TPTD_FFT_UNLOCK_TCP_WRITER(token);
				return ;
			}
			/*
			 * OK then, it seems that we have receive the first
			 * packet of client as well as the server, updating....
			 */
			if((a_tcp->client.state==TPTD_TCP_SYN_SENT ||
				a_tcp->client.state==TPTD_TCP_ESTABLISHED ||
				a_tcp->client.state==TPTD_TCP_CLIENT_DATA ) &&
				a_tcp->server.state == TPTD_TCP_CLOSE){
    				a_tcp->server.state = TPTD_TCP_SYN_RECV;
    				a_tcp->server.seq = ntohl(this_tcphdr->th_seq);
				a_tcp->server.next_seq = a_tcp->server.seq+1;
    				a_tcp->server.ack_seq = ntohl(this_tcphdr->th_ack);
				token->flow_state = TOKEN_FLOW_STATE_FROMSERVER;
				token->token_state = TOKEN_STATE_GOON;
				TPTD_FFT_UNLOCK_TCP_WRITER(token);
				return ;
			}
		}
	}
	/*
	 * Check if this seq of this packet is wrong.
     	 * Our reassemble strategy is very simple, which aim to reassemble the first
	 * several packets of one direction of TCP flow. And we only store one packet
	 * whose seq is wrong.
	 */
	if(TPTD_FFT_TCP_Need_Ressameble(token) && datalen){
		a_seq = ntohl(this_tcphdr->th_seq);
		if(from_client){
			a_next_seq = a_tcp->client.next_seq;
			a_half = &a_tcp->client;
		}else{
			a_next_seq = a_tcp->server.next_seq;
			a_half = &a_tcp->server;
		}
		if(a_next_seq != a_seq){
			/*It seems that we got a wrong packet*/
			//TPTD_Warning_General("A packet with wrong seq number.\n");
			if(a_half->aPacket == NULL){
				a_half->aPacket = (TPTD_Skbuff *)malloc(sizeof(TPTD_Skbuff));
				a_half->aPacket->seq = a_seq;
				a_half->aPacket->ack = ntohl(this_tcphdr->th_ack);
				a_half->aPacket->data = data;
			}else{
				/*Already get a wrong packet*/
				if(a_half->aPacket->seq == a_next_seq){
					//TODO update
					a_half->aPacket->seq = a_seq;
					a_half->aPacket->ack = ntohl(this_tcphdr->th_ack);
					a_half->aPacket->data = data;

				}
				if(a_half->aPacket->seq > a_seq){
					a_half->aPacket->seq = a_seq;
					a_half->aPacket->ack = ntohl(this_tcphdr->th_ack);
					a_half->aPacket->data = data;
				}
			}
			//FIXME
			token->token_state = TOKEN_STATE_GOON;
			TPTD_FFT_UNLOCK_TCP_WRITER(token);
			return ;
		}
	}

	/*This is not the first packet, let's update the time first*/
	TPTD_FFT_TCP_Update_Time(token, 0);

	/*
	 * As a RST packet appears.
	 */
	if (this_tcphdr->th_flags & TH_RST) {

		/*Updating the seq and ack_seq at first.*/
		if(from_client){
			a_tcp->client.seq = ntohl(this_tcphdr->th_seq);
			a_tcp->client.next_seq = a_tcp->client.next_seq+datalen;
			a_tcp->client.ack_seq = ntohl(this_tcphdr->th_ack);
		}else {
			a_tcp->server.seq = ntohl(this_tcphdr->th_seq);
			a_tcp->server.next_seq = a_tcp->server.next_seq+datalen;
			a_tcp->server.ack_seq = ntohl(this_tcphdr->th_ack);
		}

		/*if the server seq and ack_seq match those of client, delete it*/
		if((a_tcp->server.seq==a_tcp->client.ack_seq) &&
			(a_tcp->server.ack_seq==a_tcp->client.seq)){
			fprintf(stderr,"reset come\n");
			TPTD_FFT_TCP_Del_Flow(token,from_client);
		}
		else{
			/*If not, set the state of half close*/
			if(from_client)
				a_tcp->state = TPTD_FFT_TCP_CLIENT_CLOSE;
			else
				a_tcp->state = TPTD_FFT_TCP_SERVER_CLOSE;

		}
		/*Anyway, this token is useless for this half flow.*/
		token->token_state = TOKEN_STATE_END;
		TPTD_FFT_UNLOCK_TCP_WRITER(token);
		return;
	}

	/*
	 * As a FIN packet appears.
	 */
	if (this_tcphdr->th_flags & TH_FIN) {
	

		/*If the flow state is half close*/
		if((a_tcp->state == TPTD_FFT_TCP_CLIENT_CLOSE) && !from_client){
			fprintf(stderr,"Del_Flow: %d %d",token->tcpptr->addr.saddr,token->tcpptr->addr.daddr);
			TPTD_FFT_TCP_Del_Flow(token,from_client);
			token->token_state = TOKEN_STATE_END;
			TPTD_FFT_UNLOCK_TCP_WRITER(token);
			return ;
		}

		if((a_tcp->state == TPTD_FFT_TCP_SERVER_CLOSE) && from_client){
			fprintf(stderr,"Del_Flow: %d %d",token->tcpptr->addr.saddr,token->tcpptr->addr.daddr);
			TPTD_FFT_TCP_Del_Flow(token,from_client);
			token->token_state = TOKEN_STATE_END;
			TPTD_FFT_UNLOCK_TCP_WRITER(token);
			return ;
		}

		/*the flow change its state into half close*/
		if(from_client){
			fprintf(stderr,"from_client FIN get\n");
			a_tcp->state = TPTD_FFT_TCP_CLIENT_CLOSE;
			TPTD_TCP_PRINTFLOW(a_tcp,1);
			a_tcp->client.already_printed = 1;
		}
		else{
			fprintf(stderr,"from_server FIN get\n");
			a_tcp->state = TPTD_FFT_TCP_SERVER_CLOSE;
			TPTD_TCP_PRINTFLOW(a_tcp,0);
			a_tcp->server.already_printed = 1;
		}

		/*Anyway, this token is useless for this half flow.*/
		//changed by cc, 2014.5.8
		// do not change
		token->token_state = TOKEN_STATE_END;
		TPTD_FFT_UNLOCK_TCP_WRITER(token);
		return;
	}
	/*
	 * As a ACK packet without payload appears.
	 */
	if ((this_tcphdr->th_flags & TH_ACK) && !datalen) {
		fprintf(stderr,"no payload packet come\n");

		/*Update the seq and ack_seq first*/
		if(from_client){
			/*The ACK packet from client to finish the setup*/
			if(a_tcp->client.state == TPTD_TCP_SYN_SENT){
				a_tcp->client.state = TPTD_TCP_ESTABLISHED;
			}
			/*Update ack seq*/
			a_tcp->client.ack_seq = ntohl(this_tcphdr->th_ack);
			token->flow_state = TOKEN_FLOW_STATE_FROMCLIENT;
		}else{
			/*Update ack seq*/
			a_tcp->server.ack_seq = ntohl(this_tcphdr->th_ack);
			token->flow_state = TOKEN_FLOW_STATE_FROMSERVER;
		}

		/* Then, check if RST packet has came. */
		if(((a_tcp->state == TPTD_FFT_TCP_CLIENT_CLOSE && !from_client)||
			(a_tcp->state == TPTD_FFT_TCP_SERVER_CLOSE && from_client)) &&
		   ((a_tcp->server.seq==a_tcp->client.ack_seq) &&
			(a_tcp->server.ack_seq==a_tcp->client.seq))){
			fprintf(stderr," no payload packet come and checked rst\n");
			TPTD_FFT_TCP_Del_Flow(token,from_client);
			token->token_state = TOKEN_STATE_END;
			TPTD_FFT_UNLOCK_TCP_WRITER(token);
			return ;
		}
		token->token_state = TOKEN_STATE_GOON;
		TPTD_FFT_UNLOCK_TCP_WRITER(token);
		return ;
	}


	/*
	 * Finally, we got to face the packets with payload.
	 * But before we process the packet with payload, we should check
	 * the reassemble state and detect state to determine the processing.
	 */

	if(from_client){
		token->flow_state = TOKEN_FLOW_STATE_FROMCLIENT;
		a_tcp->client.seq = ntohl(this_tcphdr->th_seq);
		a_tcp->client.next_seq = a_tcp->client.seq+datalen;
	}
	else{
		token->flow_state = TOKEN_FLOW_STATE_FROMSERVER;
		a_tcp->server.seq = ntohl(this_tcphdr->th_seq);
		a_tcp->server.next_seq = a_tcp->server.seq+datalen;
	}
	token->token_state = TOKEN_STATE_GOON;
	TPTD_FFT_UNLOCK_TCP_WRITER(token);
	return ;
}

/****************************************************************************
 *
 * Function: TPTD_FFT_TCP_Init
 *
 * Purpose: Initialize the TCP flow table.
 *
 * Arguments: int size, the size of tcp flow table.
 *
 * Returns: int -1 for failed, 1 for success.
 *
 ****************************************************************************/

int TPTD_FFT_TCP_Init(int size)
{
	int i;

	if (!size) return -1;
	tptd_fft_tcp_table_size = size;
	tptd_fft_tcp_table = (TPTD_FFT_TCP_Flow_Table *)calloc(tptd_fft_tcp_table_size, sizeof(TPTD_FFT_TCP_Flow_Table));
	if (!tptd_fft_tcp_table) {
		perror("TPTD_FFT_TCP_Init\n");
		TPTD_Err_No_Mem("TPTD_FFT_TCP_Init");
		return -1;
	}

	for(i=0; i<tptd_fft_tcp_table_size; i++)
		g_static_rw_lock_init(&tptd_fft_tcp_table[i].rwlock);

	DEBUG_MESSAGE(DebugMessage("TPTD_FFT_TCP_Init create fft tcp flow table with size of %d\n", size););

	return 1;
}


void TPTD_TCP_PRINTFLOW(TPTD_FFT_TCP_Flow * flow, int from_client)
{
	fprintf(stderr,"enter PRINTFLOW\n");
	int clientpacketnum,serverpacketnum;
	clientpacketnum = 0;
	serverpacketnum = 0;
	int payloadnum;
	TPTD_TCP_Data_Store * packet;
	FILE * fp_server;
	FILE * fp_client;
	int signforoutput = 0;
	if(from_client)
	{
		if(flow->client.count <= MINI_PACKETS_NUMBER || flow->client.count >=  MAX_PACKETS_NUMBER)
			return;
		fp_client = fopen("nids_cTos_tcp","a+");
		if(!fp_client)
			exit(11);
		packet = flow->client.sPacket;
		clientpacketnum = 0;
		while(packet && clientpacketnum < 5)
		{
			//if(packet->payloadlen==0)
			//	fprintf(fp_client,"%02X!!!!! ",clientpacketnum);
			payloadnum = 0;
			while(payloadnum < packet->payloadlen && payloadnum < 100)
			{
				fprintf(fp_client,"%02X",clientpacketnum);
				fprintf(fp_client,"%03X",payloadnum);
				fprintf(fp_client,"%02X ",(unsigned char)packet->payload[payloadnum]);
				payloadnum ++;
				signforoutput = 1;
			}
			if(packet->payloadlen)
			{
				fprintf(fp_client,"!!!!!!! ");
				clientpacketnum ++;
			}	
			packet = packet->next;
		}
		if(signforoutput)
			fprintf(fp_client,"\n");
		fclose(fp_client);
	//	for(packet = flow->client.sPacket; packet; packet = packet->next)
	//		free(packet->payload);
	//	free(flow->client.sPacket);
	}
	
	else
	{
		if(flow->server.count <= MINI_PACKETS_NUMBER || flow->server.count >=  MAX_PACKETS_NUMBER)
			return;	
		fp_server = fopen("nids_sToc_tcp","a+");
		if(!fp_server)
			exit(12);
        	//fprintf(fp_server,"now begin write nids_sToc: %d ",serverpacketnum);
		packet = flow->server.sPacket;
		serverpacketnum = 0;
		while(packet && serverpacketnum < 5)
       		 {
			payloadnum = 0;
                	while(payloadnum < packet->payloadlen && payloadnum < 100)
               		{
                       		fprintf(fp_server,"%02X",serverpacketnum);
                        	fprintf(fp_server,"%03X",payloadnum);
                        	fprintf(fp_server,"%02X ",(unsigned char)packet->payload[payloadnum]);
				payloadnum ++;
				signforoutput = 1;
			}
			if(packet->payloadlen)
			{
				fprintf(fp_server,"!!!!!!! ");
				serverpacketnum ++;
			}
                	packet = packet->next;
        	}
		//fprintf(fp_server,"now stop\n");
		if(signforoutput)
			fprintf(fp_server,"\n");
		fclose(fp_server);
	//	for(packet = flow->server.sPacket; packet; packet = packet->next)
	//		free(packet->payload);
	//	free(flow->server.sPacket);
        }
	return;		
}
/*
void TPTD_FFT_TCP_Free_sPacket(TPTD_FFT_TCP_Flow * flow)
{
	TPTD_TCP_Data_Store * packet;
	if(flow->client && flow->client.sPacket)
	{
		for(packet = flow->client.sPacket; packet; packet = packet->next)
			if(packet->payload)
				free(packet->payload);
		free(flow->client.sPacket);
		flow->client.sPacket = NULL;
	}
	if(flow->server && flow->server.sPacket)
	{
		for(packet = flow->server.sPacket; packet; packet = packet->next)
			if(packet->payload)
				free(packet->payload);
			free(flow->server.sPacket);
			flow->server.sPacket = NULL;
	}

}
*/
