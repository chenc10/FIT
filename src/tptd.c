/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/



#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <stdio.h>
#include <unistd.h>


#include "tptd.h"
#include "nids.h"
#include "hash.h"
#include "control.h"
#include "alst/debug.h"



/* static global parameters ***************************************************/
static int exit_signal = 0;
static int usr_signal = 0;
TPTD_Global_Params tptd_gparams;

/* Signal Handlers ************************************************************/
static void SigExitHandler(int signal)
{
    if (exit_signal != 0)
        return;
    exit_signal = signal;
	exit(1);
}

static void SigUsrHandler(int signal)
{
    if (usr_signal != 0)
        return;
    usr_signal = signal;
}

static void dump(int signo)
{
        char buf[1024];
        char cmd[1024];
        FILE *fh;

        snprintf(buf, sizeof(buf), "/proc/%d/cmdline", getpid());
        if(!(fh = fopen(buf, "r")))
                exit(0);
        if(!fgets(buf, sizeof(buf), fh))
                exit(0);
        fclose(fh);
        if(buf[strlen(buf) - 1] == '\n')
                buf[strlen(buf) - 1] = '\0';
        snprintf(cmd, sizeof(cmd), "gdb %s %d", buf, getpid());
        system(cmd);

        exit(0);
}

/*
 * Process some singals.
 * Make this prog behave nicely when signals come along.
 */
static void TPTD_Init_Signals(void){
	signal(SIGTERM, SigExitHandler);
	signal(SIGINT, SigExitHandler);
	signal(SIGQUIT, SigExitHandler);
	signal(SIGUSR1, SigUsrHandler);
	//signal(SIGSEGV, &dump);

	/* Some user singal will be added here in future. Mark here!*/
}

inline static int TPTD_Get_CPUInfo(){
	return omp_get_num_procs();
}

inline static int TPTD_Cal_CapQueue(){
	//TODO calculate capqueue according to the cores of cpu
	return 2;
}

inline static int TPTD_Get_Process_Thread(){
	return tptd_gparams.capqueue;
}

static void TPTD_Init_Globals(){
	//TODO init parameter struct and so on.
	memset(&tptd_gparams, 0, sizeof(TPTD_Global_Params));
	tptd_gparams.cores = TPTD_Get_CPUInfo();
	tptd_gparams.capqueue = TPTD_Cal_CapQueue();
	tptd_gparams.processthread = TPTD_Get_Process_Thread();
	tptd_gparams.numofflat = 1045876;
	tptd_gparams.numoftokens = 97;
	tptd_gparams.mode = OFFLINE_MODE;
	if(tptd_gparams.mode == ONLINE_MODE)
		gettimeofday(&tptd_gparams.starttime, NULL);
#ifdef DEBUG
	tptd_gparams.pktofasyn = (int *)malloc(tptd_gparams.capqueue*sizeof(int));
	memset(tptd_gparams.pktofasyn, 0, tptd_gparams.capqueue*sizeof(int));
#endif
}
static void TPTD_Parse_CmdLine(int argc, char **argv){
	//TODO chew up the command line using getopt function
	int opt;
	while((opt = getopt(argc,argv,"r:w:")) != EOF)
	{
		switch(opt)
		{
		case 'r':
			//nids_pcap_file = "\0";
			memcpy(nids_pcap_file,optarg,strlen(optarg));
			nids_pcap_file[strlen(optarg)] = '\0';
			break;
		case 'w':
			//nids_prefix_output = "\0";
			memcpy(nids_prefix_output,optarg,strlen(optarg));
			nids_prefix_output[strlen(optarg)] = '\0';
			fprintf(stderr," oname: %s\n",nids_prefix_output);
			break;
		}
	}
}

static void TPTD_Init_Plugins(){

}

void TPTD_Init(int argc, char ** argv){
	/* chew up the command line */
    TPTD_Parse_CmdLine(argc, argv);
	TPTD_Init_Globals();
	TPTD_Init_Control(4);
	init_hash();


	//TODO some other init jods will be done here
	TPTD_Init_Plugins();

	/*init libnids*/
	if (!nids_init ()){
  		fprintf(stderr,"%s\n",nids_errbuf);
  		exit(1);
    }
	nids_run();
	fprintf(stdout,"finished runing\n");
}

/* Real main fuction
 * Signal, args and init will be done in this function.
 */
int TPTD_Main(int argc, char **argv){
	TPTD_Init_Signals();

	TPTD_Init(argc, argv);
}


/*main function, just invoke TPTDmain*/
int main(int argc, char **argv){

	TPTD_Main(argc, argv);

}
