/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "alarm.h"


void TPTD_Err_No_Mem(char *func){
	fprintf(stderr, "Out of memory in %s.\n", func);
	exit(1);
}

void TPTD_Warning_General(char *msg, ...){
	va_list ap;
    va_start(ap, msg);   
	fprintf(stderr, "Here is a general warning : ");
	fflush(stderr);
	vprintf(msg, ap);
    va_end(ap);
	return ;
}

void TPTD_Error(char *msg, ...){
	va_list ap;
    va_start(ap, msg);   
	fprintf(stderr, "Error : ");
	fflush(stderr);
	vprintf(msg, ap);
    va_end(ap);
	return ;
}


