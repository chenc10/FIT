/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "control.h"
#include "alst/debug.h"

MYSQL *conn_ptr;
MYSQL_RES *res;
MYSQL_ROW row;

int TPTD_Init_Plugin_Wblist_Mysql(){
	/*int t,r;
	char query[256];

	conn_ptr = mysql_init(NULL);

	if(!conn_ptr){
		fprintf(stderr, "mysql_init failed\n");
		return 0;
	}

	conn_ptr = mysql_real_connect(conn_ptr, "localhost", "root", "1231234", "wblist", 0, NULL, 0);
	if(conn_ptr){
		DEBUG_MESSAGE(DebugMessage("Mysql Connection success\n"););
	}
	else{
		fprintf(stderr, "Connection failed, check your username and password\n");
		return 0;
	}

	//TODO create black and white list from mysql
	strcpy(query, "select * from iplist");

	t = mysql_real_query(conn_ptr, query, (unsigned int)strlen(query));

	res = mysql_use_result(conn_ptr);

	for(r=0; r<=mysql_field_count(conn_ptr); r++){
		row = mysql_fetch_row(res);
		if(row<=0) break;

		for(t=0; t<mysql_num_fields(res); t++)
			printf("%s  ", row[t]);
		printf("\n");
	}

	mysql_close(conn_ptr);*/
	return 1;
}

void TPTD_Wblist_pfunction(int it, TPTD_Token *token){

	token->token_state = TOKEN_STATE_GOON;
	return;
}

void TPTD_Wblist_dfunction(void *data){

}


void TPTD_Init_Plugin_Wblist(TPTD_Modules *module){
	DEBUG_MESSAGE(DebugMessage("TPTD_Init_Plugin_Wblist\n"););
	if (!TPTD_Init_Plugin_Wblist_Mysql()){
		fprintf(stderr, "wblist init failed \n");
		exit(0);
	}
	strcpy(module->name, "wblist");

	module->pfunction = TPTD_Wblist_pfunction;
	module->dfunction = TPTD_Wblist_dfunction;
}


