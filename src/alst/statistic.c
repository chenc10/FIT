/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/
#include <stdio.h>
#include "statistic.h"
 
TPTD_Statistic_Flows tptd_statis_flow;
TPTD_Statistic_Packets tptd_statis_packet;
TPTD_Statistic_Inden_TCP	tptd_statis_inden_tcp;


static void TPTD_Statis_Print_Flow(){
	fprintf(stderr, "The tcp flow : %ld\n", tptd_statis_flow.tcpflows);
	fprintf(stderr, "The udp flow : %ld\n", tptd_statis_flow.udpflows);
}

static void TPTD_Statis_Print_Packet(){
	fprintf(stderr, "The total packet : %ld\n", tptd_statis_packet.totalpackets);
	fprintf(stderr, "The tcp packet : %ld\n", tptd_statis_packet.tcppackets);
	fprintf(stderr, "The udp packet : %ld\n", tptd_statis_packet.udppackets);
	fprintf(stderr, "The discard tcp packet : %ld\n", tptd_statis_packet.tcpdiscard);
}


inline void TPTD_Statistic_Inden_TCP_Non_Incr1(){
#pragma omp atomic
	tptd_statis_inden_tcp.non++;
}

inline void TPTD_Statistic_Packet_DisTCP_Incr1(){
#pragma omp atomic
	tptd_statis_packet.tcpdiscard++;
}

inline void TPTD_Statistic_Flow_TCP_Incr1(){
#pragma omp atomic 
	tptd_statis_flow.tcpflows++;
}
inline void TPTD_Statistic_Flow_TCP_IncrN(int n){
#pragma omp atomic
	tptd_statis_flow.tcpflows += n;
}
inline void TPTD_Statistic_Flow_TCP_Decr1(){
#pragma omp atomic
	tptd_statis_flow.tcpflows--;
}
inline void TPTD_Statistic_Flow_TCP_DecrN(int n){
#pragma omp atomic
	tptd_statis_flow.tcpflows -= n;
}

inline void TPTD_Statistic_Flow_UDP_Incr1(){
#pragma omp atomic
	tptd_statis_flow.udpflows ++;
}
inline void TPTD_Statistic_Flow_UDP_IncrN(int n){
#pragma omp atomic
	tptd_statis_flow.udpflows += n;
}
inline void TPTD_Statistic_Flow_UDP_Decr1(){
#pragma omp atomic
	tptd_statis_flow.udpflows --;
}
inline void TPTD_Statistic_Flow_UDP_DecrN(int n){
#pragma omp atomic
	tptd_statis_flow.udpflows -= n;
}

inline void TPTD_Statistic_Packet_TotalPacket_Incr1(){
#pragma omp atomic
	tptd_statis_packet.totalpackets++;
}
inline void TPTD_Statistic_Packet_TcpPacket_Incr1(){
#pragma omp atomic
	tptd_statis_packet.tcppackets++;
}
inline void TPTD_Statistic_Packet_UdpPacket_Incr1(){
#pragma omp atomic
	tptd_statis_packet.udppackets++;
}

void TPTD_Statis_Printall(){
	TPTD_Statis_Print_Packet();
	TPTD_Statis_Print_Flow();
}
