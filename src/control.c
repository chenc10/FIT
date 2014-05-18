/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <netinet/ip.h>

#include "control.h"
#include "util.h"
#include "tptd.h"
#include "wblist/wblist.h"
#include "alst/debug.h"



TPTD_Modules *modules;
TPTD_Token **tokens;
TPTD_Token **oldest_tokens;
TPTD_Detect_Header *headers;

void TPTD_Init_Plugin_End(TPTD_Modules *mptr){
	strcpy(mptr->name, "end");
}

static void TPTD_Init_Control_Modules(int num){
	TPTD_Modules *mptr;
	int i;

	modules = (TPTD_Modules *)TPTD_Sf_Alloc(sizeof(TPTD_Modules));
	memset(modules, 0, sizeof(TPTD_Modules));
	mptr = modules;
	DEBUG_MESSAGE(DebugMessage("Number of Modules is %d\n", num););
	for(i=1; i<num; i++){
		mptr->next = (TPTD_Modules *) TPTD_Sf_Alloc(sizeof(TPTD_Modules));
		mptr = mptr->next;
		memset(mptr, 0, sizeof(TPTD_Modules));
		mptr->id = i;
	}
	mptr = modules;
	if(mptr != NULL){
		TPTD_Init_Plugin_Wblist(mptr);
		mptr = mptr->next;
	}
	if(mptr != NULL){
		TPTD_Init_Plugin_Ipfragment(mptr);
		mptr = mptr->next;
	}
	if(mptr != NULL){
		TPTD_Init_Plugin_Fft(mptr);
		mptr = mptr->next;
	}
	if(mptr != NULL){
		TPTD_Init_Plugin_End(mptr);
		mptr = mptr->next;
	}

}


static void TPTD_Init_Control_Token(){
	int i;

	DEBUG_MESSAGE(DebugMessage("Number of Tokens List is %d\n", tptd_gparams.capqueue););
	tokens = (TPTD_Token **)TPTD_Sf_Alloc(tptd_gparams.capqueue * sizeof(TPTD_Token *));
	oldest_tokens = (TPTD_Token **)TPTD_Sf_Alloc(tptd_gparams.capqueue * sizeof(TPTD_Token *));

	DEBUG_MESSAGE(DebugMessage("Number of Tokens Pool for every core is %d\n", tptd_gparams.numoftokens););
	for(i=0; i<tptd_gparams.capqueue; i++){
		tokens[i] = (TPTD_Token *)TPTD_Sf_Alloc(tptd_gparams.numoftokens * sizeof(TPTD_Token));
		memset (tokens[i], 0, tptd_gparams.numoftokens * sizeof(TPTD_Token));
	}

}

static void TPTD_Init_Control_MList_Add(TPTD_Detect_Header * hdrptr, const char * name){
	TPTD_Modules *mptr = modules;
	TPTD_Detect_List *node, *lptr;

	DEBUG_MESSAGE(DebugMessage("Add module %s to header %s\n", name, hdrptr->name););
	node = (TPTD_Detect_List *)TPTD_Sf_Alloc(sizeof(TPTD_Detect_List));
	memset(node, 0, sizeof(TPTD_Detect_List));
	if(hdrptr->list == NULL){
		hdrptr->list = node;
	}else{
		lptr = hdrptr->list;
		while(lptr->next != NULL)
			lptr = lptr->next;
		lptr->next = node;
	}

	strcpy(node->name, name);
	while(mptr != NULL){
		if(!strcmp(node->name, mptr->name)){
			node->module = mptr;
			break;
		}
		mptr = mptr->next;
	}
}
#ifdef FFT
#define FT	"fft"
#endif
#ifdef HFT
#define FT   "hft"
#endif
static char detectlist[3][256] = {"start@0:wblist->ipfragment->"FT"->end$",
				  				  "apsc@0:apsc->end$",
                                  "end@0:$"};



static void TPTD_Init_Control_MList(){
	int numofheader = sizeof(detectlist)/256;
	int i;
	char tmp[256];
	char p[32];
	char *index;
	char *step;
	int count;

	TPTD_Detect_Header *hdrptr;

	for(i=0; i<numofheader; i++){
		memset(tmp, 0, 256);
		strcpy(tmp, detectlist[i]);

		if(headers == NULL){
			headers = (TPTD_Detect_Header *)TPTD_Sf_Alloc(sizeof(TPTD_Detect_Header));
			memset(headers, 0, sizeof(TPTD_Detect_Header));
			hdrptr = headers;
		}else{
			hdrptr->next = (TPTD_Detect_Header *)TPTD_Sf_Alloc(sizeof(TPTD_Detect_Header));
			hdrptr = hdrptr->next;
		}
		index = tmp;
		step = tmp;
		count = 0;
		while(*index != '\0'){

			switch (*index){
				case '@':
					strncpy(hdrptr->name, step, count);
					hdrptr->name[count] = '\0';
					step = index+1;
					count = 0;
					DEBUG_MESSAGE(DebugMessage("name : %s\n", hdrptr->name););
					break;
				case ':':
					strncpy(p, step, count);
					p[count] = '\0';
					hdrptr->level = atoi(p);
					DEBUG_MESSAGE(DebugMessage("level : %d\n", hdrptr->level););
					step = index+1;
					count = 0;
					break;
				case '-':
					if(*++index == '>'){
						strncpy(p, step, count);
						p[count] = '\0';
						DEBUG_MESSAGE(DebugMessage("%d   %s\n", count, p););
						if (count>0) TPTD_Init_Control_MList_Add(hdrptr, p);
						step = index+1;
						count = 0;
					}else {fprintf(stderr, "synax error, - must be followed by >\n"); exit(1);}
					break;
				case '$':
					strncpy(p, step, count);
					p[count] = '\0';
					DEBUG_MESSAGE(DebugMessage("%d   %s\n", count, p););
					if (count>0) TPTD_Init_Control_MList_Add(hdrptr, p);
					count = 0;
					break;
				default:
					count ++;
					break;
			}
			index ++;
		}
	}
}

void TPTD_Init_Control(int num){
	TPTD_Init_Control_Modules(num);
	TPTD_Init_Control_Token();
	TPTD_Init_Control_MList();
}

static int TPTD_Control_Find_Header(TPTD_Detect_Header **hdrptr, const char * name){

	*hdrptr = headers;
	//fprintf(stderr, "name %s\n", name);
	while(*hdrptr != NULL){
		//fprintf(stderr, "1111hdrptr->name %s\n", (*hdrptr)->name);
		if(!strcmp((*hdrptr)->name, name)){
		    //fprintf(stderr,"name is %s\n",name);
			return 1;
		}
		else
			*hdrptr = (*hdrptr)->next;
	}
	fprintf(stderr, "Error, there seems no header found\n");
	return 0;
}

/****************************************************************************
 *
 * Function: TPTD_Get_Token()
 *
 * Purpose: Get a token from token pool using hash.
 *
 * Arguments: int it, the ID of processing thread.
 *            struct cap_queue_item *qitem, the packet content popped up from asyn queue.
 *
 * Returns: TPTD_Token *ret_token, the pointer of token in token pool.
 *
 ****************************************************************************/

TPTD_Token* TPTD_Get_Token(int it, struct cap_queue_item *qitem){
	TPTD_Token * ret_token;
	//fprintf(stderr,"%d %d\n",qitem->hash,tptd_gparams.numoftokens);
	ret_token = &tokens[it][qitem->hash%tptd_gparams.numoftokens];
	fprintf(stderr,"qitem->hash index: %d\n", qitem->hash%tptd_gparams.numoftokens);
	/*
	 * Normally, the value of token_state must be the one of the following three:
     * TOKEN_STATE_FREE, TOKEN_STATE_END and TOKEN_STATE_ALIVE.
	 */
	switch(ret_token->token_state){
		case TOKEN_STATE_ALIVE:
 			/*
			 * It seems that this token can be reused.
			 * Ok then, we first check if this token is what we need.
			 * Mainly including, protocol, source and destination IP and port.
			 */
				fprintf(stderr,"qitem saddr %d source %d daddr %d dest %d\n",qitem->tuple.saddr,qitem->tuple.source,qitem->tuple.dest,qitem->tuple.daddr);
				fprintf(stderr,"token saddr %d source %d daddr %d dest %d\n",ret_token->addr.saddr,ret_token->addr.source,ret_token->addr.dest,ret_token->addr.daddr);
			if((ret_token->protocol == ((struct ip *) qitem->data)->ip_p) &&
			   (ret_token->addr.source == qitem->tuple.source) &&
			   (ret_token->addr.dest == qitem->tuple.dest) &&
			   (ret_token->addr.saddr == qitem->tuple.saddr) &&
			   (ret_token->addr.daddr = qitem->tuple.daddr)){
				/*
				 * Bingo, the update some variables.
				 * Including, detect rule chain, caplen, data.
				 */
				fprintf(stderr,"we find right token saddr %d source %d daddr %d dest %d\n",ret_token->addr.saddr,ret_token->addr.source,ret_token->addr.dest,ret_token->addr.daddr);
				DEBUG_MESSAGE(DebugMessage("Return a ALIVE TOKEN\n"););
				//TODO select detect chain, according to the detect flag and flow flag
				//TODO stycpy(ret_token->suggestname, "xxxx");
				strcpy(ret_token->suggestname, "start");
				ret_token->caplen = qitem->caplen;
				ret_token->data = qitem->data;

				/*At last, we should change the state of this token, and then return*/
				ret_token->token_state = TOKEN_STATE_NEW;
				return ret_token;
			}else{
				/*
				 * Oh no, this token is not the one we need.
				 * The reason is some other flow has taken this.
				 * So we need to initialize this token and update all variables.
				 */
				DEBUG_MESSAGE(DebugMessage("Return a TAKEN TOKEN\n"););
				fprintf(stderr,"we find a wrong token\n");
				memset(ret_token, 0, sizeof(TPTD_Token));
				ret_token->protocol = ((struct ip *) qitem->data)->ip_p;
				ret_token->hash = qitem->hash;
				ret_token->addr = qitem->tuple;
				ret_token->data = qitem->data;
				strcpy(ret_token->suggestname, "start");
				ret_token->caplen = qitem->caplen;
				ret_token->flow_state = TOKEN_FLOW_STATE_FREE;
				/*At last, we should change the state of this token, and then return*/
				ret_token->token_state = TOKEN_STATE_NEW;
				return ret_token;
			}
			break;
        case TOKEN_STATE_BREAK:
		case TOKEN_STATE_END:
 			/*
			 * It seems that the flow which used this token has some trouble.
			 * Beat it, and initiallize it.
			 */
		case TOKEN_STATE_FREE:
 			/*
			 * This token is bloodly new. Let's initialize it.
			 */
				fprintf(stderr,"qitem saddr %d source %d daddr %d dest %d\n",qitem->tuple.saddr,qitem->tuple.source,qitem->tuple.dest,qitem->tuple.daddr);
			if(ret_token->token_state == TOKEN_STATE_END)
			{
				fprintf(stderr," catch TOKEN_STATE_END!\n");
				if(ret_token->addr.saddr == qitem->tuple.saddr && ret_token->addr.source == qitem->tuple.source && ret_token->addr.dest == qitem->tuple.dest && ret_token->addr.daddr == qitem->tuple.daddr)
				// we would not accetp the packet from a finished flow, otherwise there would be core dump since the initial flow has been released
					return NULL;
			}
			DEBUG_MESSAGE(DebugMessage("Return a FREE TOKEN\n"););
			memset(ret_token, 0, sizeof(TPTD_Token));
			ret_token->protocol = ((struct ip *) qitem->data)->ip_p;
			ret_token->hash = qitem->hash;
			ret_token->addr = qitem->tuple;
			ret_token->data = qitem->data;
			ret_token->caplen = qitem->caplen;
			strcpy(ret_token->suggestname, "start");

			/*At last, we should change the state of this token, and then return*/
			ret_token->token_state = TOKEN_STATE_NEW;
			return ret_token;
	}
}
/****************************************************************************
 *
 * Function: TPTD_Control()
 *
 * Purpose: The most important function. It use token to control the whole detection process.
 *
 * Arguments: int it, the ID of processing thread.
 *            TPTD_Token *token, the pointer of token got from token pool.
 *
 * Returns: void
 *
 ****************************************************************************/

void TPTD_Control(int i, TPTD_Token *token){
	fprintf(stderr,"enter TPTD_Control\n");
	TPTD_Detect_Header *hdrptr;
	TPTD_Detect_List *lstptr;
	TPTD_Modules * mdptr;
	/*
	 * If the name of detection chain is "end", return.
	 */
	if(!strcmp(token->suggestname, "end")){
		fprintf(stderr,"333");
		token->token_state = TOKEN_STATE_FREE;
		return;
	}
	/*
	 * If the flow state is BLOCK, we invoke the block function.
	 */
	if((token->token_state==TOKEN_STATE_NEW) &&
		(token->flow_state==TOKEN_FLOW_STATE_BLOCK)){
		//TODO Do we need some strategy for blocked flows?
		return;
	}
	/*
	 * If the flow state is FORWARD, we invoke the forward function.
	 */
	if((token->token_state==TOKEN_STATE_NEW) &&
		(token->flow_state==TOKEN_FLOW_STATE_FORWARD)){
		//TODO yes, we need a forward module
		return;
	}
	/*
	 * Check if detection chain list contains the chain named as "suggestname".
	 */
	if(!TPTD_Control_Find_Header(&token->header, token->suggestname))
	{
		exit(1);
	}
	lstptr = token->header->list;

	/*
	 * OK, let's start.
	 */
	while(lstptr != NULL){
		fprintf(stderr,"enter while ,module name: %s\n",lstptr->next->name);
		if(!strcmp(lstptr->next->name,"voice"))
			{ //fprintf(stderr,"Control: find voice, exit\n");
			return;}
		/*It seems that the detection rule chain is over, then return. */
		if(!strcmp(lstptr->next->name, "end") && token->token_state==TOKEN_STATE_GOON){
            		fprintf(stderr,"Control: find end,exit\n");
			token->token_state = TOKEN_STATE_ALIVE;
			return;
		}
		//fprintf(stderr,"enter switch\n");	
		//fprintf(stderr,"%d",token->token_state);
		/*Do something according to the current token state*/
		switch(token->token_state){
			case TOKEN_STATE_NEW:
				/* For the fresh packet*/
				mdptr = lstptr->module;
				fprintf(stderr,"Control: TOKEN_STATE_NEW\n");
				mdptr->pfunction(i, token);
				break;
			case TOKEN_STATE_GOON:
				/* Next rule in chain*/
				fprintf(stderr,"ssControl: TOKEN_STATE_GOON\n");
				if(lstptr = lstptr->next){
				fprintf(stderr,"TOKEN_STATE_GOON:A\n");
				
					mdptr = lstptr->module;

					fprintf(stderr,"%s\n",mdptr->name);
					mdptr->pfunction(i, token);
					fprintf(stderr,"%s\n",mdptr->name);
				} else{
				fprintf(stderr,"TOKEN_STATE_GOON:B\n");
					TPTD_Error("The next rule function is NULL\n");
					exit(1);
				}
				fprintf(stderr,"finish control TOKEN_STATE_GOON\n");
				break;
			case TOKEN_STATE_BREAK:
				/* Changing detection rule chain, and go on*/
				if(!TPTD_Control_Find_Header(&token->header, token->suggestname))
					exit(1);
				lstptr = token->header->list;
			fprintf(stderr,"Control: TOKEN_STATE_BREAK\n");
				mdptr = lstptr->module;
				mdptr->pfunction(i, token);
				break;
			case TOKEN_STATE_END:
				/* Something happens, we break detection rule chain, and return*/
				fprintf(stderr,"Control: TOKEN_STATE_END\n");
				return;
			case TOKEN_STATE_ALIVE:
				fprintf(stderr,"TOKEN_STATE_ALIVE\n");
				/* The detect is broken, but this token can be reused*/
				return;
			case TOKEN_STATE_FREE:
				fprintf(stderr,"Control: TOKEN_STATE_FREE\n");
			//fprintf(stderr,"fcccccccccccccccccccccccc\n");
			//exit(0);
				/* Normally this state must not appear*/
				//TPTD_Error("TOKEN_STATE_FREE cannot appear here\n");
				//exit(1);
                //add by yzl
				memset(token,0,sizeof(TPTD_Token));
				return;
			default:
				fprintf(stderr,"Control: DEFAULT\n");
				return;
		}
	}
}

void TPTD_Control_Exit(){

}


