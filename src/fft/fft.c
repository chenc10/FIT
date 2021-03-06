/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/

#include <stdio.h>
#include <netinet/ip.h>

#include "ftcp.h"
#include "fudp.h"
#include "control.h"
#include "nids.h"
#include "alst/debug.h"
#include "tptd.h"

void TPTD_Flat_pfunction(int it, TPTD_Token *token){
	DEBUG_MESSAGE(DebugMessage("TPTD_Flat_pfunction\n"););
	//fprintf(stderr,"enter TPTD_fft_pfunction:TPTD_Flat_pfunction\n");
	switch (((struct ip *) token->data)->ip_p) {
    	case IPPROTO_TCP:
			fprintf(stderr,"TCP processing\n");	
			TPTD_FFT_Process_TCP(it, token);
	//		fprintf(stderr,"finish Process_Tcp");
			return;
    	case IPPROTO_UDP:
			fprintf(stderr,"UDP processing\n");
			//token->token_state = TOKEN_STATE_END;
			TPTD_FFT_Process_UDP(it, token);
	//		fprintf(stderr,"finished processing UDP\n");
			return;
        case IPPROTO_GRE:
            TPTD_FFT_Process_GRE(it,token);
            return;
    	case IPPROTO_ICMP:
			return;

    	default:

			break;
   }
}

void TPTD_Flat_dfunction(void * data){

}

void TPTD_Init_Plugin_Fft(TPTD_Modules *module){
	DEBUG_MESSAGE(DebugMessage("TPTD_Init_Plugin_Fft\n"););
	strcpy(module->name, "fft");
	//TPTD_Init_UDP_Flat();
	//tcp_init(tptd_gparams.numofflat);
	if (TPTD_FFT_TCP_Init(40960) == -1) exit(1);
	if (TPTD_FFT_UDP_Init(40960) == -1) exit(1);
	if (TPTD_FFT_GRE_Init(4096) == -1) exit(1);
	module->pfunction = TPTD_Flat_pfunction;
	module->dfunction = TPTD_Flat_dfunction;
}


