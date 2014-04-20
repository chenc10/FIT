/***************************************************************************
 *   Copyright (C) 2011 by Dawei Wang                                      *
 *   stonetools2008@gmail.com                                              *
 *                                                                         *
 *   All rights reserved                                                   *
 ***************************************************************************/
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "nids.h"

void TPTD_Syslog(int type, int errnum, struct ip *iph, void *data)
{
    char saddr[20], daddr[20];
    char buf[1024];
    struct host *this_host;
    unsigned char flagsand = 255, flagsor = 0;
    int i;

	/*switch (type) {
    case NIDS_WARN_IP:
	if (errnum != NIDS_WARN_IP_HDR) {
	    strcpy(saddr, inet_ntoa(iph->ip_src));
	    strcpy(daddr, inet_ntoa(iph->ip_dst));
	    syslog(nids_params.syslog_level,
		   "%s, packet (apparently) from %s to %s\n",
		   nids_warnings[errnum], saddr, daddr);
	} else
	    syslog(nids_params.syslog_level, "%s\n",
		   nids_warnings[errnum]);
	break;

    case NIDS_WARN_TCP:
	strcpy(saddr, inet_ntoa(iph->ip_src.s_addr));
	strcpy(daddr, inet_ntoa(iph->ip_dst.s_addr));
	if (errnum != NIDS_WARN_TCP_HDR)
	    syslog(nids_params.syslog_level,
		   "%s,from %s:%hu to  %s:%hu\n", nids_warnings[errnum],
		   saddr, ntohs(((struct tcphdr *) data)->th_sport), daddr,
		   ntohs(((struct tcphdr *) data)->th_dport));
	else
	    syslog(nids_params.syslog_level, "%s,from %s to %s\n",
		   nids_warnings[errnum], saddr, daddr);
	break;

    case NIDS_WARN_SCAN:
	this_host = (struct host *) data;
	sprintf(buf, "Scan from %s. Scanned ports: ",
		inet_ntoa(this_host->addr));
	for (i = 0; i < this_host->n_packets; i++) {
	    strcat(buf, inet_ntoa(this_host->packets[i].addr));
	    sprintf(buf + strlen(buf), ":%hu,",
		    this_host->packets[i].port);
	    flagsand &= this_host->packets[i].flags;
	    flagsor |= this_host->packets[i].flags;
	}
	if (flagsand == flagsor) {
	    i = flagsand;
	    switch (flagsand) {
	    case 2:
		strcat(buf, "scan type: SYN");
		break;
	    case 0:
		strcat(buf, "scan type: NULL");
		break;
	    case 1:
		strcat(buf, "scan type: FIN");
		break;
	    default:
		sprintf(buf + strlen(buf), "flags=0x%x", i);
	    }
	} else
	    strcat(buf, "various flags");
	syslog(nids_params.syslog_level, "%s", buf);
	break;

    default:
	syslog(nids_params.syslog_level, "Unknown warning number ?\n");
    }*/

}
