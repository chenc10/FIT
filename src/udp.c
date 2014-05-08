/*
  Copyright (c) 2010 Dawei Wang <stonetools2008@gmail.com>. All rights reserved.
  See the file COPYING for license details.
*/
#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#include "udp.h"
#include "hash.h"
#include "nids.h"
#include "tptd.h"
#include "util.h"

#define UDP_LOCK_LAYER 	2

#define UDP_MAX_WAIT_STEP	5
#define UDP_STEP_CO			1.2
#define UDP_FLOW_EXITS		0x0001
#define UDP_FLOW_ISREVERSE	0x0002
#define UDP_FLOW_ISDETECTING	0x0004
#define UDP_FLOW_ISTUNNEL	0x0008
#define UDP_FLOW_ISPENE		0x0010

#define UDP_FLOW_LAST_TIMEOUT	1800000000
#define UDP_FLOW_VAL_TIMEOUT	15000000

#define UDP_FLOW_SEQ	0
#define UDP_FLOW_LINK	1


/**********Global var *********************/

/* Init UDP Flow table. In fact it is a 4-d table
 * Thus, we need to malloc x*x*x*x memory.
 */	
TPTD_UDP_Flow tptd_udp_table[UDP_CLIENT_IP_BACKETS][UDP_CLIENT_PORT_BACKETS][UDP_SERVER_IP_BACKETS][UDP_SERVER_PORT_BACKETS];

/* 
 * OK, we need a point which can help us to find the lock quickly.
 */
#if UDP_LOCK_LAYER == 0
	TPTD_UDP_Flow	(*tptd_udp_point)[UDP_CLIENT_IP_BACKETS][UDP_CLIENT_PORT_BACKETS][UDP_SERVER_IP_BACKETS][UDP_SERVER_PORT_BACKETS];
	int tptd_max_step = 1;
	int tptd_sub_steps = UDP_CLIENT_IP_BACKETS*UDP_CLIENT_PORT_BACKETS*UDP_SERVER_IP_BACKETS*UDP_SERVER_PORT_BACKETS;
#endif

#if UDP_LOCK_LAYER == 1
	TPTD_UDP_Flow	(*tptd_udp_point)[UDP_CLIENT_PORT_BACKETS][UDP_SERVER_IP_BACKETS][UDP_SERVER_PORT_BACKETS];
	int tptd_max_step = UDP_CLIENT_IP_BACKETS;
	int tptd_sub_steps = UDP_CLIENT_PORT_BACKETS*UDP_SERVER_IP_BACKETS*UDP_SERVER_PORT_BACKETS;
#endif

#if UDP_LOCK_LAYER == 2
	TPTD_UDP_Flow	(*tptd_udp_point)[UDP_SERVER_IP_BACKETS][UDP_SERVER_PORT_BACKETS];
	int tptd_max_step = UDP_CLIENT_IP_BACKETS*UDP_CLIENT_PORT_BACKETS;
	int tptd_sub_steps = UDP_SERVER_IP_BACKETS*UDP_SERVER_PORT_BACKETS;
#endif

#if UDP_LOCK_LAYER == 3
	TPTD_UDP_Flow	(*tptd_udp_point)[UDP_SERVER_PORT_BACKETS];
	int tptd_max_step = UDP_CLIENT_IP_BACKETS*UDP_CLIENT_PORT_BACKETS*UDP_SERVER_IP_BACKETS;
	int tptd_sub_steps = UDP_SERVER_PORT_BACKETS;
#endif

#if UDP_LOCK_LAYER == 4
	TPTD_UDP_Flow	(*tptd_udp_point);
	int tptd_max_step = UDP_CLIENT_IP_BACKETS*UDP_CLIENT_PORT_BACKETS*UDP_SERVER_IP_BACKETS*UDP_SERVER_PORT_BACKETS;
	int tptd_sub_steps = 1;
#endif
int live = 0;
void TPTD_UDP_Del_One(TPTD_UDP_Flow * ptr, int islink){
	if(islink == UDP_FLOW_SEQ){
//fprintf(stderr, "udp flow seq is deleted  %d\n", --live);
		ptr->flag &= 0x0000;
		return;
	}
	if(islink == UDP_FLOW_LINK){
		free(ptr);
		//fprintf(stderr, "udp flow link is deleted  %d\n", --live);
	}

}

void TPTD_UDP_Del_All(){
	/*init main ptr and branch ptr*/
	TPTD_UDP_Flow *mptr, *bptr, *lptr, *dptr;
	int i,j;
	int delflag=0;
	struct timeval currenttime;
	u_int64_t currentstamp;
	
	tptd_udp_point = tptd_udp_table;
	mptr = (TPTD_UDP_Flow *)tptd_udp_table;
	for(i=0; i<tptd_max_step; i++){
		lptr = (TPTD_UDP_Flow *)tptd_udp_point;
		omp_set_lock(&lptr->lock);
		gettimeofday(&currenttime, NULL);
		currentstamp = 1000000 * (currenttime.tv_sec-tptd_gparams.starttime.tv_sec)+
			(currenttime.tv_usec-tptd_gparams.starttime.tv_usec);
		for(j=0; j<tptd_sub_steps; j++){
			if((mptr->flag&UDP_FLOW_EXITS) > 0){
				if(mptr->last > UDP_FLOW_LAST_TIMEOUT)//time out, delete it
					TPTD_UDP_Del_One(mptr, UDP_FLOW_SEQ);
				if((currentstamp-mptr->first-mptr->last) > UDP_FLOW_VAL_TIMEOUT)
					TPTD_UDP_Del_One(mptr, UDP_FLOW_SEQ);
			}
			bptr = mptr;

			while(bptr->next!= NULL){
	//fprintf(stderr, "mptr flag %x  hash %d   erferf: %x\n", mptr->flag, mptr->hash, mptr->flag&UDP_FLOW_EXITS);
				dptr = bptr->next;
				delflag=0;
//fprintf(stderr, "a link is in detele flag %x  hash %d   erferf: %x\n", dptr->flag, dptr->hash, dptr->flag&UDP_FLOW_EXITS);
				if((dptr->flag&UDP_FLOW_EXITS) > 0){
					if(dptr->last > UDP_FLOW_LAST_TIMEOUT){//time out, delete it
	//fprintf(stderr, "a link is deleted last : %d\n", dptr->last);
						bptr->next = dptr->next;delflag=1;
						TPTD_UDP_Del_One(dptr, UDP_FLOW_LINK);
					}
					if((currentstamp-dptr->first-dptr->last) > UDP_FLOW_VAL_TIMEOUT){
						bptr->next = dptr->next;delflag=1;
//fprintf(stderr, "a link is deleted first : %d\n", currentstamp-dptr->first-dptr->last);
						TPTD_UDP_Del_One(dptr, UDP_FLOW_LINK);
					}
				}else{
					bptr->next = dptr->next;delflag=1;
					TPTD_UDP_Del_One(dptr, UDP_FLOW_LINK);
				}
				if(delflag == 0) bptr=bptr->next;
			}
			mptr++;
		}
		omp_unset_lock(&lptr->lock);
		tptd_udp_point ++;
	}
}

void TPTD_UDP_Get_Lockptr(TPTD_Key *key, TPTD_UDP_Flow **lfp){
	
	switch (UDP_LOCK_LAYER){
		case 0:
			*lfp = (TPTD_UDP_Flow *)&tptd_udp_table[0][0][0][0];
			break;
		case 1:
			*lfp = (TPTD_UDP_Flow *)&tptd_udp_table[key->sip][0][0][0];
			break;
		case 2:
			*lfp = (TPTD_UDP_Flow *)&tptd_udp_table[key->sip][key->sport][0][0];
			break;
		case 3:
			*lfp = (TPTD_UDP_Flow *)&tptd_udp_table[key->sip][key->sport][key->dip][0];
			break;
		case 4:
			*lfp = (TPTD_UDP_Flow *)&tptd_udp_table[key->sip][key->sport][key->dip][key->dport];
			//fprintf(stderr, "%d    %d    %d    %d \n", key->sip, key->sport,key->dip, key->dport);
			break;
		default:return;
	}
}

void TPTD_UDP_Create(struct cap_queue_item *qitem, TPTD_UDP_Flow * ptr){
	tptd_gparams.numflows++;
	ptr->num++;
	if((ptr->flag & UDP_FLOW_EXITS) == 0){
		/*update hash and tuple4 first*/
		ptr->hash = qitem->hash;
		ptr->addr.source=qitem->tuple.source;
		ptr->addr.dest=qitem->tuple.dest;
		ptr->addr.saddr=qitem->tuple.saddr;
		ptr->addr.daddr=qitem->tuple.daddr;
		ptr->flag |= UDP_FLOW_EXITS;
		
		bucket++;
		/*after above, we should update time feature of flow*/
		ptr->first = 1000000 * (qitem->captime.tv_sec-tptd_gparams.starttime.tv_sec)+
				(qitem->captime.tv_usec-tptd_gparams.starttime.tv_usec);
		if(ptr->first < 0) ptr->first=0;
		//fprintf(stderr, "a seq is created %d   hash is %d\n", ++live, ptr->hash);
		//TODO update future
		return ; 
	}
collision[omp_get_thread_num()]++; 
	//fprintf(stderr, "flag %x  hash %d\n", (*ptr)->flag, (*ptr)->hash);
	/*find the last flow*/
	while(ptr->next != NULL){
		//fprintf(stderr, "in while hash is %d\n", ptr->hash);
		ptr = ptr->next;
	}
	/*OK then, insert this flow into the last of the list*/
	ptr->next = (TPTD_UDP_Flow *)malloc(sizeof(TPTD_UDP_Flow));
	
	//fprintf(stderr, "this is thread %d\n", omp_get_thread_num());
	if(ptr->next == NULL) {
	//	fprintf(stderr, "create failed\n");
		return;
	}
	ptr = ptr->next;
	ptr->next = NULL;
	if(qitem == NULL){
	//	fprintf(stderr, "qitem == NULL\n");
		return;
	}
	//fprintf(stdout, "number %d pointer %p source %d dest %d  saddr %d daddr %d\n", omp_get_thread_num(), ptr, qitem->tuple.source, qitem->tuple.dest, qitem->tuple.saddr, qitem->tuple.daddr);
	ptr->hash = qitem->hash;
	ptr->addr.source=qitem->tuple.source;
	ptr->addr.dest=qitem->tuple.dest;
	ptr->addr.saddr=qitem->tuple.saddr;
	ptr->addr.daddr=qitem->tuple.daddr;
	ptr->flag |= UDP_FLOW_EXITS;
	ptr->num++;
	
	/*after above, we should update time feature of flow*/
	//ptr->first = 1000000 * (qitem->captime.tv_sec-tptd_gparams.starttime.tv_sec)+
	//		(qitem->captime.tv_usec-tptd_gparams.starttime.tv_usec);
	//if(ptr->first < 0) ptr->first=0;
	//fprintf(stderr, "a link is created %d   flag %x  hash %d\n", ++live, ptr->flag, ptr->hash);
	//TODO update future
}


void TPTD_Process_UDP(struct cap_queue_item *qitem){
	TPTD_UDP_Flow *unrelptr, *relptr, *unrecptr, *recptr, *newptr;
	u_int64_t unrekey = qitem->hash;
	u_int rekey;
	TPTD_Key *temp;
	struct tuple4 tt;
	u_int64_t  timestamp;
	u_int32_t  timelast;
	TPTD_Half_UDP_Flow *hfp;
double k;
int fflag =0;
int cflag =0;
	//tt.source = qitem->tuple.dest;
	//tt.dest = qitem->tuple.source;
	//tt.saddr = qitem->tuple.daddr;
	//tt.daddr = qitem->tuple.saddr;
	//TPTD_Mkhash(tt, &rekey);
	timestamp = 1000000 * (qitem->captime.tv_sec-tptd_gparams.starttime.tv_sec)+
					(qitem->captime.tv_usec-tptd_gparams.starttime.tv_usec);
	
	/*Get unreverse lock firstly*/
	TPTD_UDP_Get_Lockptr((TPTD_Key *)&unrekey, &unrelptr);
	
	/*
	 * Then we should get reverse lock. 
	 * to ensure the sync we must lock the reverse and unreverse at the same time.
	 */

	//TPTD_UDP_Get_Lockptr((TPTD_Key *)&rekey, &relptr);
	//fprintf(stderr, "information for unrelptr %d\n", unrelptr->test);
	//fprintf(stderr, "information for relptr %d\n", relptr->test);
	/*traffic mode is mirrio*/
//#if TPTD_TRAFFIC_MODE == 1
	/*Let's lock the unreverse firstly.*/
//RELEASE:	
	//fprintf(stderr, "adfasdfasdfasdf\n");
//temp = (TPTD_Key *)&unrekey;
//fprintf(stderr, "reverse  %x 1  %d  2  %d   3  %d   4  %d\n", unrekey, temp->sip, temp->sport, temp->dip, temp->dport);
	omp_set_lock(&unrelptr->lock);
	
	/* 
	 * Ok then, try to lock the reverse
	 * If failed to lock the reverse, it can be said that other thread has lock it.
	 * In this situation, the current thread will wait for a moment. 
	 */
	//if(unrelptr != relptr){
		
	//	double pro;
	//	int step = 0;
	//	while(0 == omp_test_lock(&relptr->lock)){
	//		pro = pow(M_E,(double)(step-UDP_MAX_WAIT_STEP)/UDP_STEP_CO);
	//		if(pro > (double)rand()/RAND_MAX){
	//			omp_unset_lock(&unrelptr->lock);
	//			goto RELEASE;
	//		}
	//		else
	//			step++;		
	//	}
	//}//else fprintf(stderr, "unrelptr == relptr\n");
//#else
	/*traffic mode is filter*/
	//omp_set_lock(&unrelptr->lock);
	//omp_set_lock(&relptr->lock);
//#endif

	/*
	 * Let's check if the UNREVERSE flow exists first.
	 * We define the c2s directionas as UNREVERSE. 
	 */
	temp = (TPTD_Key *)&unrekey;
	unrecptr = (TPTD_UDP_Flow *)&tptd_udp_table[temp->sip][temp->sport][temp->dip][temp->dport];
	newptr = (TPTD_UDP_Flow *)&tptd_udp_table[temp->sip][temp->sport][temp->dip][temp->dport];
	//fprintf(stderr, "reverse  1  %d  2  %d   3  %d   4  %d\n", temp->sip, temp->sport, temp->dip, temp->dport);
	while(unrecptr != NULL){
		//fprintf(stderr, "unrecptr hash is %d    pointer %p   qitem %p   thread num%d\n", unrecptr->hash, unrecptr, qitem, omp_get_thread_num());
		
		if((unrecptr->addr.source==qitem->tuple.source) &&
		   (unrecptr->addr.dest==qitem->tuple.dest) &&
		   (unrecptr->addr.saddr==qitem->tuple.saddr) &&
		   (unrecptr->addr.daddr==qitem->tuple.daddr)){
gotcha++;
			/*Update time feature*/
		   timelast = (u_int32_t)(timestamp-unrecptr->first);
		   if(timelast > unrecptr->last)
		   		unrecptr->last = timelast;
		   unrecptr->num ++;
			
		   //fprintf(stderr, "got a unreverse\n");

//#if TPTD_TRAFFIC_MODE == 1
			
			/*For Mirrio mode, we use SNAP*/
			//TODO update future and get SNAP of this flow
			if((unrecptr->flag & UDP_FLOW_ISREVERSE) == 0){
				/*direction is not changed*/
				hfp = &unrecptr->c2s;
			}else{
				hfp = &unrecptr->s2c;
			}
//usleep(1);
//if(fflag == 0){
	k = 0.0;
	//while(k++ < 2500.0);
	fflag = 1;
//}
			//if(unrelptr != relptr){
			//	omp_unset_lock(&unrelptr->lock);
			//	omp_unset_lock(&relptr->lock);
			//}else
				omp_unset_lock(&unrelptr->lock);
			
			//TODO detection function
//#else
			/*
			 * For filter mode, we lock unrelptr until the detection is over.
			 * Of course, we should release relptr lock. 
			 */
			//omp_unset_lock(&relptr->lock);
			//TODO update future and detection function.
			//if((unrecptr->flag & UDP_FLOW_ISREVERSE) == 0){
				/*direction is not changed*/
			//	hfp = &unrecptr->c2s;
			//}else{
			//	hfp = &unrecptr->s2c;
			//}
			//omp_unset_lock(&unrelptr->lock);			
//#endif			
			return ;
		}else{
			//fprintf(stderr, "we are in un next\n");
			unrecptr = unrecptr->next;
//if(cflag == 0) {collision++; cflag = 1;}

		}
	}

	/*check if there exits the reverse flow in table*/
	//temp = (TPTD_Key *)&rekey;
	//recptr = (TPTD_UDP_Flow *)&tptd_udp_table[temp->sip][temp->sport][temp->dip][temp->dport];
	//fprintf(stderr, "unreverse 1  %d  2  %d   3  %d   4  %d\n", temp->sip, temp->sport, temp->dip, temp->dport);
	//while(recptr != NULL){
	//	if((recptr->addr.source==qitem->tuple.dest) &&
	//	   (recptr->addr.dest==qitem->tuple.source) &&
	//	   (recptr->addr.saddr==qitem->tuple.daddr) &&
	//	   (recptr->addr.daddr==qitem->tuple.saddr)){
			
			/*
			 * Update time feature
			 * If recptr->first > timestamp, it can be said that the current direction is wrong. 
			 * Then we need to get the right direction, using flag. 
			 */
			//fprintf(stderr, "got a reverse\n");
	//	   if(recptr->first > timestamp){
	//		  recptr->last += (u_int32_t)(recptr->first-timestamp);
	//		  recptr->first = timestamp;
	//		  recptr->flag |= UDP_FLOW_ISREVERSE;
	//	   }else{
	//		  timelast = (u_int32_t)(timestamp-recptr->first);
	//	   	  if(timelast > recptr->last)
	//	   		recptr->last = timelast;
	//	   }
//#if TPTD_TRAFFIC_MODE == 1
			/*As same as the reverse*/
			//TODO update future and get SANP of this flow 
	//		if((recptr->flag & UDP_FLOW_ISREVERSE) == 0){
				/*direction is not changed*/
	//			hfp = &recptr->s2c;
	//		}else{
	//			hfp = &recptr->c2s;
	//		}
//usleep(1);
//if(fflag == 0){
	k = 0.0;

	//while(k++ < 2500.0);
//	fflag = 1;
//}
			//if(unrelptr != relptr){
			//	omp_unset_lock(&unrelptr->lock);
			//	omp_unset_lock(&relptr->lock);
			//}else
			//	omp_unset_lock(&unrelptr->lock);
			//TODO detection function
			
//#else
			/*
			 * For filter mode, we lock relptr until the detection is over.
			 * Of course, we should release unrelptr lock. 
			 */
			//omp_unset_lock(&unrelptr->lock);
			//TODO update future and detection function.
			//if((recptr->flag & UDP_FLOW_ISREVERSE) == 0){
				/*direction is not changed*/
			//	hfp = &recptr->s2c;
			//}else{
			//	hfp = &recptr->c2s;
			//}
			//omp_unset_lock(&relptr->lock);			
//#endif		
			//return ;
		//}else{
			//fprintf(stderr, "we are in re next\n");
		//	recptr = recptr->next;
		//}
//	}
	//
	/*
	 * It seems that this packets is the first one of a new flow.
	 * OK then, create a new one, c2s of course. 
	 */
	
	TPTD_UDP_Create(qitem, newptr);
//usleep(1);
//if(fflag == 0){
	k = 0.0;
	//while(k++ < 2500.0);
	fflag = 1;
//}
//#if TPTD_TRAFFIC_MODE == 1
	//TODO get SNAP
	//if(unrelptr != relptr){
		omp_unset_lock(&unrelptr->lock);
	//	omp_unset_lock(&relptr->lock);
	//}else
	//	omp_unset_lock(&unrelptr->lock);
	//TODO detection function
//#else
	//TODO update fugure and detection function
	//omp_unset_lock(&relptr->lock);
	//omp_unset_lock(&unrelptr->lock);
//#endif
	double m = 0.0;
	//while (m++ < 500.0);
	
	return ;
}

void TPTD_Init_UDP(){
	int i;
	TPTD_UDP_Flow *ptr;

	memset(&tptd_udp_table, 0, 
		UDP_CLIENT_IP_BACKETS*UDP_CLIENT_PORT_BACKETS*UDP_SERVER_IP_BACKETS*UDP_SERVER_PORT_BACKETS*sizeof(TPTD_UDP_Flow));
	
	int j,k,l, count=0;
	//for(i=0; i<UDP_CLIENT_IP_BACKETS; i++)
		//for(j=0; j<UDP_CLIENT_PORT_BACKETS; j++)
			//for(k=0; k<UDP_SERVER_IP_BACKETS; k++)
				//for(l=0; l<UDP_SERVER_PORT_BACKETS; l++)
					//tptd_udp_table[i][j][k][l].test = count++;
	/*
	 * Let's try to init the openmp lock according to the layer.
	 */
	tptd_udp_point = tptd_udp_table;
	for(i=0; i<tptd_max_step; i++){
		ptr = (TPTD_UDP_Flow *)tptd_udp_point;
		omp_init_lock(&ptr->lock);
		tptd_udp_point++;
	}	
}





