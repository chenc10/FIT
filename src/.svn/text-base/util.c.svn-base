/*
  Copyright (c) 1999 Rafal Wojtczuk <nergal@7bulls.com>. All rights reserved.
  See the file COPYING for license details.
*/

#include <config.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


#include "util.h"
#include "nids.h"
#define STD_BUF  1024
void
nids_no_mem(char *func)
{
  fprintf(stderr, "Out of memory in %s.\n", func);
  exit(1);
}

char *
test_malloc(int x)
{
  char *ret = malloc(x);
  
  if (!ret)
    nids_params.no_mem("test_malloc");

  return ret;
}

void TPTD_Fatal_Error(const char *format,...)
{
    char buf[STD_BUF+1];
    va_list ap;

    va_start(ap, format);
    vsnprintf(buf, STD_BUF, format, ap);
    va_end(ap);

	buf[STD_BUF] = '\0';

	fprintf(stderr, "ERROR: %s", buf);
    fprintf(stderr,"Fatal Error, Quitting..\n");

	exit(1);
}

void * TPTD_Sf_Alloc(unsigned long size){
	void *tmp;

	tmp = (void *) malloc(size);
	memset(tmp, 0, size);
	if(tmp == NULL){
		 TPTD_Fatal_Error("Unable to allocate memory!  (%lu requested)\n", size);
	}
	return tmp;
}

void
register_callback(struct proc_node **procs, void (*x))
{

}

void
unregister_callback(struct proc_node **procs, void (*x))
{

}
