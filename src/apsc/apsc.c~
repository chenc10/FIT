/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "control.h"
#include "alst/debug.h"


int TPTD_Init_Plugin_Apsc(){
	return 1;
}

void TPTD_Apsc_pfunction(int it, TPTD_Token *token){

	token->token_state = TOKEN_STATE_GOON;
	return;
}

void TPTD_Apsc_dfunction(void *data){

}


void TPTD_Init_Plugin_Apsc(TPTD_Modules *module){
	DEBUG_MESSAGE(DebugMessage("TPTD_Init_Plugin_Apsc\n"););
	if (!TPTD_Init_Plugin_Apsc()){
		fprintf(stderr, "wblist init failed \n");
		exit(0);
	}
	strcpy(module->name, "apsc");

	module->pfunction = TPTD_Apsc_pfunction;
	module->dfunction = TPTD_Apsc_dfunction;
}


