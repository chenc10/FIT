/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/

#ifndef DEBUG_H
#define DEBUG_H

void DebugMessageFunc(char *fmt, ...); 
   
#ifdef DEBUG
	extern char *DebugMessageFile;
    extern int DebugMessageLine;
 	#define    DebugMessage    DebugMessageFile = __FILE__; DebugMessageLine = __LINE__; DebugMessageFunc
#endif

#ifdef DEBUG
#define DEBUG_MESSAGE(code) code
#else 
#define DEBUG_MESSAGE(code) 
#endif


#endif
