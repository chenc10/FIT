#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef DEBUG
   
  
char *DebugMessageFile = NULL;
int DebugMessageLine = 0;

void DebugMessageFunc(char *fmt, ...)   
{
    va_list ap;

    va_start(ap, fmt);        

	if (DebugMessageFile != NULL)
		fprintf(stdout, "%s:%d: ", DebugMessageFile, DebugMessageLine);
	fflush(stderr);
	vprintf(fmt, ap);

    va_end(ap);
}


#endif
