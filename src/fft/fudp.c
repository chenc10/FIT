/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "nids.h"
#include "tptd.h"
#include "fudp.h"
#include "alst/debug.h"
#include "control.h"
#include "alst/statistic.h"

#define TIME_TICK	1000000

//add by cc
#define OUTPUT_FLOW_SIZE 20
#define OUTPUT_PACKET_SIZE 100

TPTD_FFT_UDP_Flow_Table *tptd_fft_udp_table;
int tptd_fft_udp_table_size;

#define TPTD_FFT_UNLOCK_UDP_WRITER(token) \
		if(token->lock_state == TOKEN_LOCK_STATE_TWOWRITE){\
			if(token->udplockptr != token->rudplockptr){\
				g_static_rw_lock_writer_unlock(&token->udplockptr->rwlock);\
				g_static_rw_lock_writer_unlock(&token->rudplockptr->rwlock);\
			}else g_static_rw_lock_writer_unlock(&token->udplockptr->rwlock);\
		} if(token->lock_state == TOKEN_LOCK_STATE_WRITE) g_static_rw_lock_writer_unlock(&token->udplockptr->rwlock);\
		token->lock_state = TOKEN_LOCK_STATE_FREE;

inline static int TPTD_FFT_UDP_Mkhash(int hash){
	return hash %= tptd_fft_udp_table_size;
}

inline static int TPTD_FFT_UDP_Mkhash_All(struct tuple4 *addr){
	return mkhash(addr->saddr, addr->source, addr->daddr, addr->dest)
				% tptd_fft_udp_table_size;
}

inline static int TPTD_FFT_UDP_Compare(struct tuple4 *taddr, struct tuple4 *baddr){
	if((taddr->dest==baddr->dest) &&
	   (taddr->saddr==baddr->saddr) &&
	   (taddr->source==baddr->source) &&
	   (taddr->daddr==baddr->daddr))
		return 1;
	else
		return 0;
}

inline static void TPTD_FFT_UDP_Assign(struct tuple4 *taddr, struct tuple4 *saddr){
	taddr->dest = saddr->dest;
	taddr->saddr = saddr->saddr;
	taddr->source = saddr->source;
	taddr->daddr = saddr->daddr;
	return;
}

int TPTD_UDP_COPY_BUFFER(TPTD_Token * token, int client)
{
/* 
	add by cc
	this function can only be cited inside TPTD_FFT_UDP_Process.
*/
	//if(client && token->udpptr->client_count > 20)
	//{
		//token->udpptr->client_count ++;
	//	return;
	//}
	//if(!client && token->udpptr->server_count > 20)
	//{
		//token->udpptr->server_count ++;
	//	return;
	//}
	fprintf(stderr,"enter UDP_COPY_BUFFER\n");
	unsigned char *data = token->data;
	struct ip * this_iphdr = (struct ip *)data;
	struct udphdr * this_udphdr = (struct udphdr *)(data + 4 * this_iphdr->ip_hl);
	int iplen = ntohs(this_iphdr->ip_len);

	int datalen = iplen - 4 * this_iphdr->ip_hl - 8;
	if(datalen < 0)
		return 0;
	unsigned char * puredata;
	TPTD_UDP_Data_Store * newpacket;
	TPTD_UDP_Data_Store ** packet;
	if(client)
	{
		token->udpptr->client_count ++;
		if(token->udpptr->client_count > OUTPUT_FLOW_SIZE)
			return;
		newpacket = (TPTD_UDP_Data_Store*)malloc(sizeof(TPTD_UDP_Data_Store));
		if(newpacket == NULL)
		{
			TPTD_Err_No_Mem("TPTD_FFT_UDP_Add_New_PACKET");
			exit(1);
		}
		memset(newpacket,0,sizeof(TPTD_UDP_Data_Store));
		newpacket->payloadlen = datalen;
		newpacket->payload = (char *)malloc(datalen + 1);
		memset(newpacket->payload, 0, sizeof(datalen+1));
		puredata = data + 4*this_iphdr->ip_hl + 8;
		memcpy(newpacket->payload, puredata, datalen);
		packet = &(token->udpptr->client);
		//fprintf(stderr,"copy: C+\n");
		//fprintf(stderr,"client_count: %d \n", token->udpptr->client_count);
		//fprintf(stderr,"%d yes\n",token->udpptr->client == NULL);
		while((*packet)!= NULL)
		{
		//	fprintf(stderr,"5E: udp client\n");
			packet = &((*packet)->next);
		//	fprintf(stderr,"%d\n",((*packet)==NULL));
		}
		//fprintf(stderr,"copy: D\n");
		(*packet) = newpacket;
		(*packet)->next = NULL;
		//token->udpptr->client_count ++;
		if(token->udpptr->client_count == OUTPUT_FLOW_SIZE)
		{
			fprintf(stderr,"count enough PRINTFLOW\n");
			TPTD_UDP_PRINTFLOW(token->udpptr, 1);
			token->udpptr->already_printed = 1;
		}
	}

	else
        {
		token->udpptr->server_count ++;
		if(token->udpptr->server_count > OUTPUT_FLOW_SIZE)
			return;
                newpacket = (TPTD_UDP_Data_Store*)malloc(sizeof(TPTD_UDP_Data_Store));
                if(newpacket == NULL)
                {
                        TPTD_Err_No_Mem("TPTD_FFT_UDP_Add_New_PACKET");
                        exit(1);
                }
                memset(newpacket,0,sizeof(TPTD_UDP_Data_Store));
		newpacket->payloadlen = datalen;
                newpacket->payload = (char *)malloc(datalen + 1);

                memset(newpacket->payload, 0, sizeof(datalen+1));
                puredata = data + 4*this_iphdr->ip_hl + 8;

                memcpy(newpacket->payload, puredata, datalen);
		packet = &(token->udpptr->server);
		
		while((*packet)!= NULL)
                {   
                        packet = &((*packet)->next);
                }   
                (*packet) = newpacket;
		(*packet)->next = NULL;
                //token->udpptr->server_count ++; 
		if(token->udpptr->server_count == OUTPUT_FLOW_SIZE)
		{
			TPTD_UDP_PRINTFLOW(token->udpptr, 0);
			token->udpptr->already_printed = 1;
		}
	   }
	return 1;
}

void TPTD_UDP_PRINTFLOW( TPTD_FFT_UDP_Flow * flow, int from_client)
{// be noted that I don't consider blank packet here.
	FILE * fp_client;
	FILE * fp_server;
	TPTD_UDP_Data_Store * packet;
	int payloadnum;
	int clientpacketnum,serverpacketnum;
	if( from_client)
	{
		//fprintf(stderr,"print TPTD_FFT_UDP_Flow\n");
		fp_client = fopen("nids_cTos_udp", "a+");
		if(!fp_client)
			exit(12);
		for(packet = flow->client,clientpacketnum = 0;packet && clientpacketnum < flow->client_count; packet = packet->next, clientpacketnum ++)
		{
			for(payloadnum = 0; payloadnum < packet->payloadlen && payloadnum < OUTPUT_PACKET_SIZE; payloadnum ++)
			{
				fprintf(fp_client, "%02X", clientpacketnum);
				fprintf(fp_client, "%03X", payloadnum);
				fprintf(fp_client, "%02X ", (unsigned char)packet->payload[payloadnum]);
			}
			fprintf(fp_client, "!!!!!!! ");
		}
		fprintf(fp_client,"\n");
		fclose(fp_client);
	//	for(packet = flow->client; packet; packet = packet->next)
	//		free(packet->payload);
	//	free(flow->client);
	}
	else
	{
		fp_server = fopen("nids_sToc_udp", "a+");
		if(!fp_server)
			exit(12);
		for( packet = flow->server, serverpacketnum = 0; packet && serverpacketnum < flow->server_count; packet = packet->next, serverpacketnum ++)
		{
			for(payloadnum = 0; payloadnum < packet->payloadlen && payloadnum < OUTPUT_PACKET_SIZE; payloadnum ++)
			{
				fprintf(fp_server, "%02X", serverpacketnum);
				fprintf(fp_server, "%03X", payloadnum);
				fprintf(fp_server, "%02X ", (unsigned char)packet->payload[payloadnum]);
			}
			fprintf(fp_server, "!!!!!!! ");
		}
		fprintf(fp_server,"\n");
		fclose(fp_server);
	//	for(packet = flow->server; packet; packet = packet->next)
	//		free(packet->payload);
	//	free(flow->server);
	}
	return;
}	
		
			
static int TPTD_FFT_UDP_Find(TPTD_Token *token, int *is_reverse){
	fprintf(stderr," enter UDP_Find\n");
	TPTD_FFT_UDP_Flow_Table *lockptr, *rlockptr;
	TPTD_FFT_UDP_Flow *ufptr, *rufptr;
	int hash, rhash;
	char *data = token->data;
	struct ip *this_iphdr = (struct ip *)data;
	struct udphdr *this_udphdr = (struct udphdr *)(data + 4 * this_iphdr->ip_hl);
    u_int32_t iplen = ntohs(this_iphdr->ip_len);
	u_int32_t datalen = iplen- 4 * this_iphdr->ip_hl - 8;
	rhash = token->rhash;
	hash = token->hash;
	lockptr = &tptd_fft_udp_table[hash];
	rlockptr = &tptd_fft_udp_table[rhash];
FFT_UDP_RELEASE:
	g_static_rw_lock_writer_lock(&lockptr->rwlock);

	if(lockptr != rlockptr){
		double pro;
		int step = 0;
		/*try to lock the bucket of reverse direction*/
		while(FALSE == g_static_rw_lock_writer_trylock(&rlockptr->rwlock)){
			pro = pow(M_E,(double)(step-5)/1.2);
			if(pro > (double)rand()/RAND_MAX){
				g_static_rw_lock_writer_unlock(&lockptr->rwlock);
				goto FFT_UDP_RELEASE;
			}
			else
				step++;
		}
	}
	token->lock_state = TOKEN_LOCK_STATE_TWOWRITE;
	token->udplockptr = lockptr;
	token->rudplockptr = rlockptr;
	token->udpptr = NULL;
	ufptr = lockptr->flow;
	
	
	while(ufptr != NULL){
		if(TPTD_FFT_UDP_Compare(&token->addr, &ufptr->addr)){
			fprintf(stderr,"UDP_Find ufptr match\n");
			token->udplockptr = lockptr;
			token->udpptr = ufptr;
			//add by cc
			TPTD_UDP_COPY_BUFFER(token,1);
			g_static_rw_lock_writer_unlock(&rlockptr->rwlock);
			token->lock_state = TOKEN_LOCK_STATE_WRITE;
			*is_reverse = 0;
			if(token->udpptr->del_flag == TPTD_READY_DEL)
                token->token_state = TOKEN_STATE_FREE;
			return 1;
		}else{
			ufptr = ufptr->next;
		}
	}


	rufptr = rlockptr->flow;
	while(rufptr != NULL){
		if(TPTD_FFT_UDP_Compare(&token->raddr, &rufptr->addr)){
			fprintf(stderr,"UDP_Find rufptr match\n");
			token->udplockptr = rlockptr;
			token->udpptr = rufptr;
			//add by cc
			token->lock_state = TOKEN_LOCK_STATE_WRITE;
			TPTD_UDP_COPY_BUFFER(token,0);
			g_static_rw_lock_writer_unlock(&lockptr->rwlock);
			*is_reverse = 1;
			if(token->udpptr->del_flag == TPTD_READY_DEL)
                token->token_state = TOKEN_STATE_FREE;
			return 1;
		}else{
			rufptr = rufptr->next;
		}
	}
	return 0;
}

void TPTD_FFT_UDP_Timeout(int * process_finished){
	*process_finished = 0;
	//fprintf(stderr,"enter TPTD_FFT_UDP_Timeout\n");
	TPTD_FFT_UDP_Flow_Table *fft = tptd_fft_udp_table;
	int i;
	TPTD_FFT_UDP_Flow *flow, *delpoint, *prepoint;
	struct timeval now; 
	u_int64_t firstval;
	u_int64_t lastval;
	//changed by cc, I assign the real time to now
	/*
	if(tptd_gparams.mode==OFFLINE_MODE)
		now = tptd_gparams.currtime;
	else
		gettimeofday(&now, NULL);
	*/
	gettimeofday(&now,NULL);
	fprintf(stderr,"UDP_now: %ld\n",now.tv_sec);
    	struct in_addr tad;
	for(i=0; i<tptd_fft_udp_table_size; i++){
		g_static_rw_lock_writer_lock(&fft[i].rwlock);
		flow = fft[i].flow;
		//fprintf(stderr,"tptd_fft_udp_table_size:%d\n",i);
		prepoint = flow;
		while(flow != NULL)
		{
			//fprintf(stderr,"here is udp flow\n");
			*process_finished = 1;
			//fprintf(stderr,"flow->first.tv_sec %ld flow->last.tv_sec %ld\n",flow->first.tv_sec,flow->last.tv_sec);
			firstval = now.tv_sec-flow->first.tv_sec;
			lastval = now.tv_sec-flow->last.tv_sec;
			fprintf(stderr,"flow!=NULL %d %d\n",firstval,lastval);
			if(firstval > 2000 || lastval > 1 )
			{
				delpoint = flow;
				if(delpoint->already_printed == 0)
				{
					//fprintf(stderr,"udp timeout and output\n");
					fprintf(stderr,"timeout PRINTFLOW\n");
					TPTD_UDP_PRINTFLOW(delpoint,1);
					//TPTD_UDP_PRINTFLOW(delpoint,0);
				}
				flow = flow->next;
				if(fft[i].flow == delpoint)
				{
					fft[i].flow = delpoint->next;
				}
				else
				{
					while(prepoint->next!=delpoint)
						prepoint = prepoint->next;
					prepoint->next = delpoint->next;
				}
               			 if(delpoint->del_flag == TPTD_UNREADY_DEL)
                   			 delpoint->del_flag = TPTD_READY_DEL;
                		else
               			{
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
	//fprintf(stderr,"finish UDP_Timeout\n\n");
}

static void TPTD_FFT_UDP_Update_Time(TPTD_Token *token, int isfirst){
	TPTD_FFT_UDP_Flow *flowptr = token->udpptr;

	if(flowptr == NULL){
		TPTD_Error("TPTD_FFT_UDP_Update_Time: the flow ptr is NULL\n");
		exit(1);
	}

	//struct timeval *temptime = &token->captime;
	//fprintf(stderr,"token->captime: %ld\n", temptime->tv_sec);
	struct timeval temptime;
	gettimeofday( &temptime, NULL);
	//fprintf(stderr,"temptime: %ld\n", temptime.tv_sec);
	/* if it is the first packet of the half flow. */
    /*if(isfirst)
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
	
	// add by cc
	// I use the real time
	
	if(isfirst)
	{
		flowptr->first = temptime;
		flowptr->last = temptime;
	}
	else
		flowptr->last = temptime;
}

static int TPTD_FFT_UDP_Create(TPTD_Token *token, struct ip *this_iphdr){
	//fprintf(stderr,"HEY: UDP_Create\n");
	TPTD_FFT_UDP_Flow *newflow;
	TPTD_FFT_UDP_Flow_Table *fuptr;
	u_int32_t iplen = ntohs(this_iphdr->ip_len);
	u_int32_t datalen = iplen- 4 * this_iphdr->ip_hl - 8;

	newflow = (TPTD_FFT_UDP_Flow *)malloc(sizeof(TPTD_FFT_UDP_Flow));
	if(newflow == NULL){
		TPTD_Err_No_Mem("TPTD_FFT_UDP_Add_New_Flow");
		exit(1);
	}
	memset(newflow, 0, sizeof(TPTD_FFT_UDP_Flow));
	TPTD_FFT_UDP_Assign(&newflow->addr, &token->addr);
	fuptr = token->udplockptr;
	// add new flow into the flowtable
	if(fuptr->flow == NULL){
		fuptr->flow = newflow;
	}else{
		newflow->next = fuptr->flow;
		fuptr->flow = newflow;
	}
	token->udpptr=token->udplockptr->flow;
	// we regard that each first udp flow is from client.
	fuptr->flow->is_client = CREATE_BY_CLIENT;
	// add by cc
	//fuptr->flow->client = NULL;
	//uptr->flow->server = NULL;
	if(fuptr->flow->already_printed != 0)
		fprintf(stderr," error - udp_create_already_printed != 0\n");
	TPTD_FFT_UDP_Update_Time(token, 1);
}

void TPTD_FFT_Process_UDP(int it, TPTD_Token *token){
	//fprintf(stderr,"enter TPTD_FFT_Process_UDP:main\n");
	char *data = token->data;
	struct ip *this_iphdr = (struct ip *)data;
	struct udphdr *this_udphdr = (struct udphdr *)(data + 4 * this_iphdr->ip_hl);
	u_int32_t iplen = ntohs(this_iphdr->ip_len);
	u_int32_t datalen = iplen- 4 * this_iphdr->ip_hl - 8;

	TPTD_FFT_UDP_Flow *a_udp;
	int is_reverse = 0;
	u_int64_t curtime;
	struct timeval *temptime;

	temptime = &token->captime;

	switch(token->flow_state){
	
		case TOKEN_FLOW_STATE_FREE:
			fprintf(stderr,"TOKEN_FLOW_STATE_FREE\n");
			token->hash = TPTD_FFT_UDP_Mkhash(token->hash);
			token->raddr.source = token->addr.dest;
			token->raddr.dest = token->addr.source;
			token->raddr.saddr = token->addr.daddr;
			token->raddr.daddr = token->addr.saddr;
			token->rhash = TPTD_FFT_UDP_Mkhash_All(&token->raddr);
			if(!TPTD_FFT_UDP_Find(token, &is_reverse))
			{
				fprintf(stderr,"now TPTD_FFT_UDP_Create\n");
				TPTD_FFT_UDP_Create(token, this_iphdr);
				TPTD_Statistic_Flow_UDP_Incr1();
				TPTD_UDP_COPY_BUFFER(token,1);
				//token->udpptr->client_count++;
				token->token_state = TOKEN_STATE_GOON;

				token->udpptr->is_client = CREATE_BY_CLIENT;
				TPTD_FFT_UNLOCK_UDP_WRITER(token);
				token->flow_state = TOKEN_FLOW_STATE_FROMCLIENT;
			}
			else if(token->token_state == TOKEN_STATE_FREE)
			{
			    	TPTD_FFT_UNLOCK_UDP_WRITER(token);
                		return;
			}

			else
			{//if already exist in flow table

				if(is_reverse)
				{// if this is s->c
					if(token->udpptr->server_count==0)
					{
						curtime = 1000000*(token->captime.tv_sec - token->udpptr->first.tv_sec)+
								(token->captime.tv_usec - token->udpptr->first.tv_usec);
						if(curtime < 0)
						{
							/**
							 * It's seems there is a mistake. Amending.
							 **/
							token->udpptr->is_client = CREATE_BY_SERVER;
							/*But we still intend to update server.*/
							TPTD_FFT_UDP_Update_Time(token, 1);
						}
					}
					else
					{
                    				TPTD_FFT_UDP_Update_Time(token,0);
                 			}
                 			token->flow_state = TOKEN_FLOW_STATE_FROMSERVER;
					//to avoid redundancy, we only add server_count when copying buffer
					token->token_state = TOKEN_STATE_GOON;
                 			TPTD_FFT_UNLOCK_UDP_WRITER(token);
                		}
				else
				{// if this is c->s
					//fprintf(stderr,"flow:A\n");
                			//token->udpptr->client_count++;
					token->token_state = TOKEN_STATE_GOON;
					TPTD_FFT_UNLOCK_UDP_WRITER(token);
					token->flow_state = TOKEN_FLOW_STATE_FROMCLIENT;
               			}

            		}
            		break;

	case TOKEN_FLOW_STATE_FROMCLIENT:
		//add by yzl
	//	fprintf(stderr,"TOKEN_FLOW_STATE_FROMCLIENT\n");
            if(!token->udpptr->del_flag)
            {	
		//fprintf(stderr,"flow: B\n");
                if(token->udplockptr && token->udpptr ){
                    if(token->lock_state == TOKEN_LOCK_STATE_FREE){
                        g_static_rw_lock_writer_lock(&token->udplockptr->rwlock);
                        token->lock_state = TOKEN_LOCK_STATE_WRITE;
			
			TPTD_UDP_COPY_BUFFER(token,1);
                        //token->udpptr->client_count++;
			
                        TPTD_FFT_UDP_Update_Time(token, 0);
                        token->token_state = TOKEN_STATE_GOON;
                        TPTD_FFT_UNLOCK_UDP_WRITER(token);
                    }
                }else
                    token->token_state = TOKEN_STATE_END;
            }
            else
            {
                 token->token_state = TOKEN_STATE_FREE;
                 TPTD_FFT_UNLOCK_UDP_WRITER(token);
            }
            break;

        case TOKEN_FLOW_STATE_FROMSERVER:
	//	fprintf(stderr,"TOKEN_FLOW_STATE_FROMSERVER\n");
            if(!token->udpptr->del_flag)
            {
	//	fprintf(stderr,"flow: C\n");
                if(token->udplockptr && token->udpptr){
                    if(token->lock_state == TOKEN_LOCK_STATE_FREE){
                        g_static_rw_lock_writer_lock(&token->udplockptr->rwlock);
                        token->lock_state = TOKEN_LOCK_STATE_WRITE;
			// in fact currently it is not visited
			TPTD_UDP_COPY_BUFFER(token,0);
                        TPTD_FFT_UDP_Update_Time(token, 0);
                        token->token_state = TOKEN_STATE_GOON;
                        TPTD_FFT_UNLOCK_UDP_WRITER(token);
                    }
                }else
                    token->token_state = TOKEN_STATE_END;
            }
            else
            {
                 token->token_state = TOKEN_STATE_FREE;
                 TPTD_FFT_UNLOCK_UDP_WRITER(token);
            }
            break;
    }
}

int TPTD_FFT_UDP_Init(int size){
	int i;

	if (!size) return -1;
	tptd_fft_udp_table_size = size;
	tptd_fft_udp_table = (TPTD_FFT_UDP_Flow_Table *)calloc(size, sizeof(TPTD_FFT_UDP_Flow_Table));
	if (!tptd_fft_udp_table) {
		perror("TPTD_FFT_UDP_Init\n");
		TPTD_Err_No_Mem("TPTD_FFT_UDP_Init");
		return -1;
	}

	for(i=0; i<size; i++)
		g_static_rw_lock_init(&tptd_fft_udp_table[i].rwlock);

	DEBUG_MESSAGE(DebugMessage("TPTD_FFT_UDP_Init create fft udp flow table with size of %d\n", size););

	return 1;
}

