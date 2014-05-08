/*
  Copyright (c) 1999 Rafal Wojtczuk <nergal@7bulls.com>. All rights reserved.
  See the file COPYING for license details.
*/

#include <config.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <alloca.h>
#include <pcap.h>
#include <errno.h>
#include <omp.h>
#include <sys/time.h>

#ifdef HAVE_LIBGTHREAD_2_0
#include <glib.h>
#endif

#include "nids.h"
#include "tptd.h"
#include "util.h"
#include "hash.h"
#include "udp.h"
#include "control.h"
#include "alst/debug.h"
#include "alst/statistic.h"


static pcap_t *desc = NULL;
char nids_errbuf[PCAP_ERRBUF_SIZE];

char nids_pcap_file[128] = "./2_26.pcap";
char nids_prefix_output[128] = "result";

static struct proc_node *ip_frag_procs;
static struct proc_node *ip_procs;
static struct proc_node *udp_procs;
struct timeval start;
int collision[6];
int gotcha;
int bucket=0;
int lock=0;

struct proc_node *tcp_procs;
static int linktype;
u_int nids_linkoffset = 0;
u_char *nids_last_pcap_data = NULL;
struct pcap_pkthdr * nids_last_pcap_header = NULL;

#ifdef HAVE_LIBGTHREAD_2_0
/* async queue for multiprocessing - mcree */
static GAsyncQueue *cap_queue;

typedef struct _TPTD_Cap_Queue_List{
	int sn;
	GAsyncQueue *cap_queue;
}TPTD_Cap_Queue_List;

static TPTD_Cap_Queue_List *tptd_cplist;


/* marks end of queue */
static struct cap_queue_item EOF_item;

/* error buffer for glib calls */
static GError *gerror = NULL;

#endif


void nids_exit();
static void nids_syslog(int, int, struct ip *, void *);
static int nids_ip_filter(struct ip *, int);
static int open_live();
static void init_procs();
extern int ip_options_compile(unsigned char *);


struct nids_prm nids_params = {
    1040,			/* n_tcp_streams */
    4096,			/* n_hosts */
    NULL,			/* device */
    nids_pcap_file,			/* filename */
    168,			/* sk_buff_size */
    -1,				/* dev_addon */
    nids_syslog,		/* syslog() */
    LOG_ALERT,			/* syslog_level */
    256,			/* scan_num_hosts */
    3000,			/* scan_delay */
    10,				/* scan_num_ports */
    nids_no_mem,		/* no_mem() */
    nids_ip_filter,		/* ip_filter() */
    NULL,			/* pcap_filter */
    1,				/* promisc */
    0,				/* one_loop_less */
    1024,			/* pcap_timeout */
    1,				/* multiproc */
    20000000,			/* queue_limit */
    0,				/* tcp_workarounds */
    NULL,			/* pcap_desc */
	0			/* fileEOF */
};
static int nids_ip_filter(struct ip *x, int len)
{
    (void)x;
    (void)len;
    return 1;
}


int nids_init(){
	int i;
    /* free resources that previous usages might have allocated */
    nids_exit();

    if (nids_params.pcap_desc)
        desc = nids_params.pcap_desc;
    else if (nids_params.filename) {
	if ((desc = pcap_open_offline(nids_params.filename,
				      nids_errbuf)) == NULL)
	    return 0;
    } else if (!open_live())
	return 0;

    if (nids_params.pcap_filter != NULL) {
	u_int mask = 0;
	struct bpf_program fcode;

	if (pcap_compile(desc, &fcode, nids_params.pcap_filter, 1, mask) <
	    0) return 0;
	if (pcap_setfilter(desc, &fcode) == -1)
	    return 0;
    }
	switch ((linktype = pcap_datalink(desc))) {
#ifdef DLT_IEEE802_11
#ifdef DLT_PRISM_HEADER
    case DLT_PRISM_HEADER:
#endif
#ifdef DLT_IEEE802_11_RADIO
    case DLT_IEEE802_11_RADIO:
#endif
    case DLT_IEEE802_11:
	/* wireless, need to calculate offset per frame */
	break;
#endif
#ifdef DLT_NULL
    case DLT_NULL:
        nids_linkoffset = 4;
        break;
#endif
    case DLT_EN10MB:
		nids_linkoffset = 14;
		break;
    case DLT_PPP:
		nids_linkoffset = 4;
		break;
	/* Token Ring Support by vacuum@technotronic.com, thanks dugsong! */
    case DLT_IEEE802:
		nids_linkoffset = 22;
		break;

    case DLT_RAW:
    case DLT_SLIP:
		nids_linkoffset = 0;
		break;
#define DLT_LINUX_SLL   113
    case DLT_LINUX_SLL:
		nids_linkoffset = 16;
		break;
#ifdef DLT_FDDI
    case DLT_FDDI:
        nids_linkoffset = 21;
        break;
#endif
#ifdef DLT_PPP_SERIAL
    case DLT_PPP_SERIAL:
        nids_linkoffset = 4;
        break;
#endif
    default:
		strcpy(nids_errbuf, "link type unknown");
		return 0;
    }
    if (nids_params.dev_addon == -1) {
	if (linktype == DLT_EN10MB)
	    nids_params.dev_addon = 16;
	else
	    nids_params.dev_addon = 0;
    }
    if (nids_params.syslog == nids_syslog)
		openlog("libnids", 0, LOG_LOCAL0);

    init_procs();
	TPTD_Init_UDP();
    //tcp_init(nids_params.n_tcp_streams);
    //ip_frag_init(nids_params.n_hosts);
    //scan_init();

    if(nids_params.multiproc) {
#ifdef HAVE_LIBGTHREAD_2_0
		tptd_cplist = (TPTD_Cap_Queue_List *)calloc(tptd_gparams.capqueue, sizeof(TPTD_Cap_Queue_List));
		if(tptd_cplist == NULL){
			strcpy(nids_errbuf, "failed to create cap queue list");
			return 0;
		}
		g_thread_init(NULL);
		for(i=0; i<tptd_gparams.capqueue; i++){
			tptd_cplist[i].cap_queue = g_async_queue_new();
			tptd_cplist[i].sn = i;
		}
		cap_queue=g_async_queue_new();

#else
	 strcpy(nids_errbuf, "libnids was compiled without threads support");
	 return 0;
#endif
    }
    return 1;
}

#ifdef HAVE_LIBGTHREAD_2_0

#define START_CAP_QUEUE_PROCESS_THREAD() \
    if(nids_params.multiproc) { /* threading... */ \
			cap_queue_process_thread_init(); \
    }

#define STOP_CAP_QUEUE_PROCESS_THREAD() \
    if(nids_params.multiproc) { /* stop the capture process thread */ \
	 int w;\
	 for(w=0; w<tptd_gparams.capqueue; w++)\
	 	g_async_queue_push(tptd_cplist[w].cap_queue,&EOF_item); \
    }

/* called either directly from pcap_hand() or from cap_queue_process_thread()
 * depending on the value of nids_params.multiproc - mcree
 */
static void call_ip_frag_procs(struct cap_queue_item *qitem)
{
    struct proc_node *i;
    for (i = ip_frag_procs; i; i = i->next)
	(i->item) (qitem);
}

/* thread entry point
 * pops capture queue items and feeds them to
 * the ip fragment processors - mcree
 * We re-program it according to our needs - dawei
 */
static void cap_queue_process_thread(int i)
{// be noted: I only handle offline mode
	struct cap_queue_item *qitem;
	TPTD_Token *token;
	while(1) { /* loop "forever" */
		fprintf(stderr,"\nloop again\n");
		//if(nids_params.fileEOF)
		//{
		//	fprintf(stderr,"enter fileEOF\n");
		//	qitem = g_async_queue_try_pop(tptd_cplist[i].cap_queue);
		//	if(qitem == &EOF_item || qitem == NULL)
		//		exit(1);
		//}
		//else
		//{
		//	fprintf(stderr,"enter queue_pop\n");
		//	qitem=g_async_queue_pop(tptd_cplist[i].cap_queue);
		//	fprintf(stderr,"leave queue_pop\n");
		//}
		fprintf(stderr,"prepare to pop\n");
		qitem = g_async_queue_pop(tptd_cplist[i].cap_queue);
		fprintf(stderr,"already pop\n");
		fprintf(stderr,"qitem saddr:%d source:%d daddr:%d dest:%d  qitem->hash: %d\n",qitem->tuple.saddr, qitem->tuple.source, qitem->tuple.daddr, qitem->tuple.dest, qitem->hash);
		if (qitem == &EOF_item) 
		{
			fprintf(stderr,"EOF output\n");
			break; /* EOF item received: we should exit */
		}
		
		//if(qitem->tuple.source == 0 && qitem->tuple.dest == 0)
		//{
		//	fprintf(stderr,"zero qitem find!\n");
		//	continue;
		//}
		if(qitem == NULL) exit(1);
		if((token = TPTD_Get_Token(i, qitem))== NULL)
			continue;
		
		/*if(token->flow_state == TOKEN_FLOW_STATE_FROMSERVER)
		{
			if(token->tcpptr->addr.saddr != qitem->tuple.saddr || token->tcpptr->addr.source != qitem->tuple.source || token->tcpptr->addr.daddr != qitem->tuple.daddr || token->tcpptr->addr.dest != qitem->tuple.dest)
			if(1)
			{
				fprintf(stderr,"TOKEN_FLOW_STATE_FROMSERVER token wrong!\n");
				continue;
			}
			fprintf(stderr,"token wrong!\n");
		//	continue;
			fprintf(stderr," TOKEN_FLOW_STATE_FROMSERVER: token saddr:%d source:%d daddr:%d dest:%d  hash: %d\n",token->tcpptr->addr.saddr, token->tcpptr->addr.source, token->tcpptr->addr.daddr, token->tcpptr->addr.dest, token->rhash);		
		}*/
			
		//fprintf(stderr," token saddr:%d source:%d daddr:%d dest:%d  hash: %d\n",token->addr.saddr, token->addr.source, token->addr.daddr, token->addr.dest, token->hash);
		//DEBUG_MESSAGE(DebugMessage("tcp source IP is : %s   \n",
		//		inet_ntoa(((struct ip *) qitem->data)->ip_src)););
		//DEBUG_MESSAGE(DebugMessage("tcp destination IP is : %s   \n",
		//		inet_ntoa(((struct ip *) qitem->data)->ip_dst)););
		//DEBUG_MESSAGE(DebugMessage("tcp source port is : %d    tcp destination port is : %d\n",
		//		qitem->tuple.source, qitem->tuple.dest););
		if(tptd_gparams.mode==OFFLINE_MODE){
			tptd_gparams.currtime = qitem->captime;
			if(tptd_gparams.starttime.tv_sec == 0)
			{
				tptd_gparams.starttime = qitem->captime;
			}
		}
		token->captime = qitem->captime;
		//if(qitem->tuple.source==53351 || qitem->tuple.dest==53351)
		TPTD_Control(i, token);
		fprintf(stderr,"cap:D\n");
		free(qitem->data);
		free(qitem);
		//if(nids_params.fileEOF)
		//	break;
		fprintf(stderr,"this loop finished\n");
	}
	
	return ;
}
static void TPTD_Timeout_Process_thread(){
	int *  process_finished1, *process_finished2;
	process_finished1 = (int *)malloc(sizeof(int));
	process_finished2 = (int *)malloc(sizeof(int));
	*process_finished1 = 1;
	*process_finished2 = 1;
	//while((*process_finished1)||(*process_finished2)){
	struct timeval top_now;
	gettimeofday(&top_now,NULL);
	fprintf(stderr,"\nTimeout_Process_gettimeofday: %ld\n", top_now.tv_sec);
	sleep(2);
	gettimeofday(&top_now,NULL);
	fprintf(stderr,"Timeout_Process_after_sleep: %ld\n",top_now.tv_sec);
		//while(1){
		//usleep(200000);
		//fprintf(stderr, "this is timeout process thread\n");
		fprintf(stderr,"begin TCP_Timeout\n");
		TPTD_FFT_TCP_Timeout(process_finished1);
		fprintf(stderr,"finish TCP_Timeout\n");
		TPTD_FFT_UDP_Timeout(process_finished2);
		//fprintf(stderr,"process_finished %d %d\n\n",*process_finished1,*process_finished2);
	//}
}

static void cap_queue_process_thread_init(){
	cap_queue_process_thread(omp_get_thread_num());
}

#else
#define START_CAP_QUEUE_PROCESS_THREAD()
#define STOP_CAP_QUEUE_PROCESS_THREAD()

#endif

static int TPTD_Packet_Preprocess_TCP(struct cap_queue_item *qitem){
	struct ip *this_iphdr = (struct ip *)qitem->data;
	struct tcphdr *this_tcphdr = (struct tcphdr *)(qitem->data + 4 * this_iphdr->ip_hl);
	int datalen, iplen;

	iplen = ntohs(this_iphdr->ip_len);
	if ((unsigned)iplen < 4 * this_iphdr->ip_hl + sizeof(struct tcphdr)) {
		TPTD_Syslog(NIDS_WARN_TCP, NIDS_WARN_TCP_HDR, this_iphdr,
		       this_tcphdr);
		TPTD_Statistic_Packet_DisTCP_Incr1();
		return 1;
	}
	datalen = iplen - 4 * this_iphdr->ip_hl - 4 * this_tcphdr->th_off;

	if (datalen < 0) {
		TPTD_Syslog(NIDS_WARN_TCP, NIDS_WARN_TCP_HDR, this_iphdr,
		       this_tcphdr);
		TPTD_Statistic_Packet_DisTCP_Incr1();
		return 1;
	}
	if ((this_iphdr->ip_src.s_addr | this_iphdr->ip_dst.s_addr) == 0) {
		TPTD_Syslog(NIDS_WARN_TCP, NIDS_WARN_TCP_HDR, this_iphdr,
		       this_tcphdr);
		TPTD_Statistic_Packet_DisTCP_Incr1();
		return 1;
	}
	return 0;
}

void nids_pcap_handler(u_char * par, struct pcap_pkthdr *hdr, u_char * data){
    u_char *data_aligned;
	int sn=0;
	TPTD_Statistic_Packet_TotalPacket_Incr1();

#ifdef HAVE_LIBGTHREAD_2_0
    struct cap_queue_item *qitem;
#endif
#ifdef DLT_IEEE802_11
    unsigned short fc;
    int linkoffset_tweaked_by_prism_code = 0;
    int linkoffset_tweaked_by_radio_code = 0;
#endif

    /*
     * Check for savagely closed TCP connections. Might
     * happen only when nids_params.tcp_workarounds is non-zero;
     * otherwise nids_tcp_timeouts is always NULL.
     */
   // if (NULL != nids_tcp_timeouts)
    //  tcp_check_timeouts(&hdr->ts);

    nids_last_pcap_header = hdr;
    nids_last_pcap_data = data;
    (void)par; /* warnings... */
    switch (linktype) {
    case DLT_EN10MB:
	if (hdr->caplen < 14)
	    return;
	/* Only handle IP packets and 802.1Q VLAN tagged packets below. */
	if (data[12] == 8 && data[13] == 0) {
	    /* Regular ethernet */
	    nids_linkoffset = 14;
	} else if (data[12] == 0x81 && data[13] == 0) {
	    /* Skip 802.1Q VLAN and priority information */
	    nids_linkoffset = 18;
	} else
	    /* non-ip frame */
	    return;
	break;
#ifdef DLT_PRISM_HEADER
#ifndef DLT_IEEE802_11
#error DLT_PRISM_HEADER is defined, but DLT_IEEE802_11 is not ???
#endif
    case DLT_PRISM_HEADER:
	nids_linkoffset = 144; //sizeof(prism2_hdr);
	linkoffset_tweaked_by_prism_code = 1;
        //now let DLT_IEEE802_11 do the rest
#endif


    default:;
    }
    if (hdr->caplen < nids_linkoffset)
	return;

/*
* sure, memcpy costs. But many EXTRACT_{SHORT, LONG} macros cost, too.
* Anyway, libpcap tries to ensure proper layer 3 alignment (look for
* handle->offset in pcap sources), so memcpy should not be called.
*/
#ifdef LBL_ALIGN
    if ((unsigned long) (data + nids_linkoffset) & 0x3) {
	data_aligned = alloca(hdr->caplen - nids_linkoffset + 4);
	data_aligned -= (unsigned long) data_aligned % 4;
	memcpy(data_aligned, data + nids_linkoffset, hdr->caplen - nids_linkoffset);
    } else
#endif
    data_aligned = data + nids_linkoffset;

#ifdef HAVE_LIBGTHREAD_2_0

	if(nids_params.multiproc){
		qitem=malloc(sizeof(struct cap_queue_item));
		if (qitem && (qitem->data=malloc(hdr->caplen - nids_linkoffset))){
			qitem->caplen=hdr->caplen - nids_linkoffset;
          	memcpy(qitem->data,data_aligned,qitem->caplen);

			switch (((struct ip *) data_aligned)->ip_p) {
				case IPPROTO_TCP:
					TPTD_Statistic_Packet_TcpPacket_Incr1();
					qitem->tuple.saddr = ((struct ip *) data_aligned)->ip_src.s_addr;
		  			qitem->tuple.daddr = ((struct ip *) data_aligned)->ip_dst.s_addr;
					qitem->tuple.source = ntohs(((struct tcphdr *)
						(data_aligned + 4 * ((struct ip *) data_aligned)->ip_hl))->th_sport);
		  			qitem->tuple.dest = ntohs(((struct tcphdr *)
						(data_aligned + 4 * ((struct ip *) data_aligned)->ip_hl))->th_dport);
					qitem->captime.tv_sec = hdr->ts.tv_sec;
					qitem->captime.tv_usec = hdr->ts.tv_usec;
					if(TPTD_Packet_Preprocess_TCP(qitem)){
						return;
					}
					if(!strcmp("10.0.0.61", inet_ntoa(((struct ip *) data_aligned)->ip_src))){
					DEBUG_MESSAGE(DebugMessage("tcp source IP is : %s   \n",
						inet_ntoa(((struct ip *) data_aligned)->ip_src)););
					DEBUG_MESSAGE(DebugMessage("tcp destination IP is : %s   \n",
						inet_ntoa(((struct ip *) data_aligned)->ip_dst)););
					DEBUG_MESSAGE(DebugMessage("tcp source port is : %d    udp destination port is : %d\n",
						qitem->tuple.source, qitem->tuple.dest););
					}
#ifdef HFT
					TPTD_Mkhash(qitem->tuple, (TPTD_Key *)&qitem->hash);
#endif
#ifdef FFT
					qitem->hash = mkhash (qitem->tuple.saddr, qitem->tuple.source, qitem->tuple.daddr, qitem->tuple.dest);
				//	DEBUG_MESSAGE(DebugMessage("tcp hash is : %d\n", qitem->hash););
					//fprintf(stderr,"%d %d %d %d %d\n",qitem->tuple.saddr,qitem->tuple.source,qitem->tuple.daddr,qitem->tuple.dest,qitem->hash);
#endif

					break;
				case IPPROTO_UDP:
					TPTD_Statistic_Packet_UdpPacket_Incr1();
					qitem->tuple.saddr = ((struct ip *) data_aligned)->ip_src.s_addr;
		  			qitem->tuple.daddr = ((struct ip *) data_aligned)->ip_dst.s_addr;
					qitem->tuple.source = ntohs(((struct udphdr *)
						(data_aligned + 4 * ((struct ip *) data_aligned)->ip_hl))->uh_sport);
		  			qitem->tuple.dest = ntohs(((struct udphdr *)
						(data_aligned + 4 * ((struct ip *) data_aligned)->ip_hl))->uh_dport);
					qitem->captime.tv_sec = hdr->ts.tv_sec;
					qitem->captime.tv_usec = hdr->ts.tv_usec;
#ifdef HFT
					TPTD_Mkhash(qitem->tuple, (TPTD_Key *)&qitem->hash);
#endif
#ifdef FFT
					qitem->hash = mkhash (qitem->tuple.saddr, qitem->tuple.source, qitem->tuple.daddr, qitem->tuple.dest);	
					//fprintf(stderr,"%d %d %d %d %d\n",qitem->tuple.saddr,qitem->tuple.source,qitem->tuple.daddr,qitem->tuple.dest,qitem->hash);
					//DEBUG_MESSAGE(DebugMessage("udp hash is : %d\n", qitem->hash););
#endif
					//DEBUG_MESSAGE(DebugMessage("udp source IP is : %s   \n",
					//	inet_ntoa(((struct ip *) data_aligned)->ip_src)););
					//DEBUG_MESSAGE(DebugMessage("udp destination IP is : %s   \n",
					//	inet_ntoa(((struct ip *) data_aligned)->ip_dst)););
					//DEBUG_MESSAGE(DebugMessage("udp source port is : %d    udp destination port is : %d\n",
					//	qitem->tuple.source, qitem->tuple.dest););

					break;

				case IPPROTO_GRE:
					qitem->tuple.saddr = ((struct ip *) data_aligned)->ip_src.s_addr;
		  			qitem->tuple.daddr = ((struct ip *) data_aligned)->ip_dst.s_addr;
		  			qitem->captime.tv_sec = hdr->ts.tv_sec;
					qitem->captime.tv_usec = hdr->ts.tv_usec;
					qitem->hash = mkhash_gre(qitem->tuple.saddr, qitem->tuple.daddr);
					break;
				default:
					return;
			}

			sn = qitem->hash%tptd_gparams.capqueue;
			
			// add by cc, sn makes the statistic results different due to hash result. currently I don't know the reason for using sn
			sn = 1;

#ifdef DEBUG
			tptd_gparams.pktofasyn[sn]++;
#endif
			g_async_queue_lock(tptd_cplist[sn].cap_queue);
			if(g_async_queue_length_unlocked(tptd_cplist[sn].cap_queue) > nids_params.queue_limit) {
				TPTD_Warning_General("Asynchronous queue %d is full, packets will be dropped.\n", sn);
				free(qitem->data);
	    		free(qitem);
			}else{
				g_async_queue_push_unlocked(tptd_cplist[sn].cap_queue,qitem);
			}
			g_async_queue_unlock(tptd_cplist[sn].cap_queue);
		}
	}else{

	}
#else

#endif
	
}

int nids_run()
{
	int i,j,k,l;
    if (!desc) {
	strcpy(nids_errbuf, "Libnids not initialized");
	return 0;
    }
    //START_CAP_QUEUE_PROCESS_THREAD(); /* threading... */
struct timeval stoptime;
#ifdef HAVE_LIBGTHREAD_2_0
	omp_set_nested(1);
	gettimeofday(&start, NULL);
	fprintf(stderr,"start time: %ld",start.tv_sec);
#pragma omp parallel sections
{
	#pragma omp section
	{	
		pcap_loop(desc, -1, (pcap_handler) nids_pcap_handler, 0);
		
		STOP_CAP_QUEUE_PROCESS_THREAD();
		fprintf(stderr,"EOF item input!\n");
			//cap_queue_process_thread(1);
	}
	#pragma omp section
	{
		#pragma omp parallel num_threads(1)
		{
			//cap_queue_process_thread(omp_get_thread_num());
			cap_queue_process_thread(1);
		//	cap_queue_process_thread(0);
		}
	}
	#pragma omp section
	{
		//fprintf(stderr," haha \n");
		TPTD_Timeout_Process_thread();
		
	}

}
#else
	pcap_loop(desc, -1, (pcap_handler) nids_pcap_handler, 0);
#endif

    /* FIXME: will this code ever be called? Don't think so - mcree */
    TPTD_Statis_Printall();

    //nids_exit();
    return 0;
}

void nids_exit(){
	if (!desc) {
        strcpy(nids_errbuf, "Libnids not initialized");
	return;
    }

#ifdef HAVE_LIBGTHREAD_2_0
    if (nids_params.multiproc) {
    /* I have no portable sys_sched_yield,
       and I don't want to add more synchronization...
    */
      while (g_async_queue_length(cap_queue)>0)
        usleep(100000);
    }
#endif

	//tcp_exit();
	//ip_frag_exit();
	//scan_exit();
    strcpy(nids_errbuf, "loop: ");
    strncat(nids_errbuf, pcap_geterr(desc), sizeof nids_errbuf - 7);
    if (!nids_params.pcap_desc)
        pcap_close(desc);
    desc = NULL;

	//free(ip_procs);
    free(ip_frag_procs);
}

static int open_live()
{
    char *device;
    int promisc = 0;

    if (nids_params.device == NULL)
	nids_params.device = pcap_lookupdev(nids_errbuf);
    if (nids_params.device == NULL)
	return 0;

    device = nids_params.device;
    if (!strcmp(device, "all"))
	device = "any";
    else
	promisc = (nids_params.promisc != 0);

    if ((desc = pcap_open_live(device, 65535, promisc,
			       nids_params.pcap_timeout, nids_errbuf)) == NULL)
	return 0;

    return 1;
}
/*
static void gen_ip_frag_proc(struct cap_queue_item *qitem)
{

    struct proc_node *i;
    struct ip *iph = (struct ip *) qitem->data;
    int need_free = 0;
    int skblen;
    void (*glibc_syslog_h_workaround)(int, int, struct ip *, void*)=
       nids_params.syslog;

    if (!nids_params.ip_filter(iph, qitem->caplen))
		return;
	ip_fast_csum((unsigned char *) iph, iph->ip_hl);

    if (qitem->caplen < (int)sizeof(struct ip) || iph->ip_hl < 5 || iph->ip_v != 4 ||
	(u_short)ip_fast_csum((unsigned char *) iph, iph->ip_hl) != 0 ||
	qitem->caplen < ntohs(iph->ip_len) || ntohs(iph->ip_len) < iph->ip_hl << 2) {
	glibc_syslog_h_workaround(NIDS_WARN_IP, NIDS_WARN_IP_HDR, iph, 0);
		return;
    }
   if (iph->ip_hl > 5 && ip_options_compile((unsigned char *)qitem->data)) {
	glibc_syslog_h_workaround(NIDS_WARN_IP, NIDS_WARN_IP_SRR, iph, 0);
	return;
   }

    switch (ip_defrag_stub((struct ip *) qitem->data, &iph)) {
    	case IPF_ISF:
			return;
    	case IPF_NOTF:
			need_free = 0;
			iph = (struct ip *) qitem->data;
			break;
    	case IPF_NEW:
			need_free = 1;
			break;
    	default:;
    }
    skblen = ntohs(iph->ip_len) + 16;
    if (!need_free)
		skblen += nids_params.dev_addon;
    skblen = (skblen + 15) & ~15;
    skblen += nids_params.sk_buff_size;

    for (i = ip_procs; i; i = i->next)
		(i->item) (qitem, skblen);
    if (need_free)
		free(iph);
}
*/
static void gen_ip_proc(struct cap_queue_item *qitem, int skblen)
{
	//tokens[i].qitem = qitem;
	//TPTD_Control(i);
	//switch (((struct ip *) qitem->data)->ip_p) {
    //	case IPPROTO_TCP:
	//		break;
    //	case IPPROTO_UDP:
			//TPTD_Process_UDP(qitem);
			//process_udp_flat(qitem);
	//		break;
    //	case IPPROTO_ICMP:
	//		break;
    //	default:

	//		break;
   // }

}

static void init_procs()
{
    ///ip_frag_procs = mknew(struct proc_node);
    //ip_frag_procs->item = gen_ip_frag_proc;
   // ip_frag_procs->next = 0;
	//ip_procs = mknew(struct proc_node);
    //ip_procs->item = gen_ip_proc;
    //ip_procs->next = 0;
}

static void nids_syslog(int type, int errnum, struct ip  *iph, void *data){

}




