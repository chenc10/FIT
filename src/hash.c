#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


#include "hash.h"
#include "tptd.h"
#include "nids.h"
#include "udp.h"
// changed by cc
static u_char xor[12] = {25,37,19,21,12,13,26,35,14,20,15,17};
static u_char perm[12] = {2,4,6,8,10,12,1,3,5,7,9,11};
static u_char pip[4] = {5,6,7,8};
static u_char ipxor[4] = {13,14,15,16};
static u_char pport[2] = {17,19};
static u_char portxor[2] = {38,39};

static void
getrnd ()
{
  struct timeval s;
  u_int *ptr;
  u_int16_t *ptr16;
  int fd = open ("/dev/urandom", O_RDONLY);
  if (fd > 0)
    {
      //read (fd, xor, 12);
      //read (fd, perm, 12);
	//read (fd, pip, 4);
	  //read (fd, ipxor, 4);
	  //read (fd, pport, 2);
	  //read (fd, portxor, 2);
      close (fd);
      return;
    }

  gettimeofday (&s, 0);
  srand (s.tv_usec);
  ptr = (u_int *) xor;
  *ptr = rand ();
  *(ptr + 1) = rand ();
  *(ptr + 2) = rand ();
  ptr = (u_int *) perm;
  *ptr = rand ();
  *(ptr + 1) = rand ();
  *(ptr + 2) = rand ();
  ptr = (u_int *) pip;
  *ptr = rand();
  ptr = (u_int *) ipxor;
  *ptr = rand();
  ptr16 = (u_int16_t *) pport;
  *ptr16 = (u_int16_t)rand();
  ptr16 = (u_int16_t *) portxor;
  *ptr16 = (u_int16_t)rand();

}
void
init_hash ()
{
  int i, n, j;
  int p[12];
  int p1[4];
  int p2[2];
  getrnd ();
  for (i = 0; i < 12; i++)
    p[i] = i;
  for (i = 0; i < 12; i++)
    {
      n = perm[i] % (12 - i);
      perm[i] = p[n];
      for (j = 0; j < 11 - n; j++)
	      p[n + j] = p[n + j + 1];
    }

	for(i=0; i<4; i++)
		p1[i] = i;
	for(i=0; i<4; i++){
		n = pip[i] % (4 - i);
		pip[i] = p1[n];
		for(j=0; j<4-n; j++)
			p1[n+j] = p1[n+j+1];
	}

	for(i=0; i<2; i++)
		p2[i] = i;
	for(i=0; i<2; i++){
		n = pport[i] % (2 - i);
		pport[i] = p2[n];
		for(j=0; j<2-n; j++)
			p2[n+j] = p2[n+j+1];
	}
}

u_int
mkhash (u_int src, u_short sport, u_int dest, u_short dport)
{
  u_int res = 0;
  int i;
  u_char data[12];
  u_int *stupid_strict_aliasing_warnings=(u_int*)data;
  *stupid_strict_aliasing_warnings = src;
  *(u_int *) (data + 4) = dest;
  *(u_short *) (data + 8) = sport;
  *(u_short *) (data + 10) = dport;
  for (i = 0; i < 12; i++){
	//fprintf(stderr, "res   %d\n", res);
    res = ( (res << 8) + (data[perm[i]] ^ xor[i]))% 0xff100f;
  }
  return res;
}

u_int 
mkhash_gre (u_int src, u_int dest){
  u_int res = 0;
  int i;
  u_char data[8];
  u_int *stupid_strict_aliasing_warnings=(u_int*)data;
  *stupid_strict_aliasing_warnings = src;
  *(u_int *) (data + 4) = dest;

  for (i = 0; i < 8; i++){
	//fprintf(stderr, "res   %d\n", res);
    res = ( (res << 8) + (data[perm[i]] ^ xor[i]))% 0xff100f;
  }
  return res;
}

static void TPTD_Mkhash_Static(u_int src, u_short sport, u_int dest, u_short dport, TPTD_Key *key){
	memset(key, 0, sizeof(TPTD_Key));
	u_int32_t ip=0;
	u_int16_t  port=0;
	int i;
	
	u_char data1[4];
	u_char data2[2];

	u_int *stupid_strict_aliasing_warnings=(u_int*)data1;
	*stupid_strict_aliasing_warnings = src;
	for(i=0; i<4; i++){
		ip = ((ip<<8) + (data1[pip[i]] ^ ipxor[i])) % 0xf43;
		//fprintf(stderr, "%d     ", ip);
	}
	//fprintf(stderr, "\n");
//if(ip > 4000) fprintf(stderr, "%d\n", key->sip);
	ip %= UDP_CLIENT_IP_BACKETS;
	key->sip = ip;

	//fprintf(stdout, "%d                ", key->sip);

	*stupid_strict_aliasing_warnings = dest;
	ip = 0;

	for(i=0; i<4; i++){
		ip = ((ip<<8) + (data1[pip[i]] ^ ipxor[i])) % 0xf43;
	}	
	ip %= UDP_SERVER_IP_BACKETS;
	key->dip = ip;
	

	u_short *stupid_strict_aliasing_warnings16=(u_short*)data2;
	*stupid_strict_aliasing_warnings16 = sport;
	for(i=0; i<2; i++)
		port = ((port<<4) + (data2[pport[i]] ^ portxor[i])) % 0xE9;
	port %= UDP_CLIENT_PORT_BACKETS;
	key->sport = port;
	//fprintf(stderr, "%d\n", key->sport);

	*stupid_strict_aliasing_warnings16 = dport;
	port = 0;
	for(i=0; i<2; i++)
		port = ((port<<4) + (data2[pport[i]] ^ portxor[i])) % 0xE9;
	port %= UDP_SERVER_PORT_BACKETS;
	key->dport = port;
	//fprintf(stderr, "%d\n", key->dport);

	//fprintf(stdout, "%x    ****\n", key);
}

void TPTD_Mkhash(struct tuple4 t, TPTD_Key *key){
	TPTD_Mkhash_Static(t.saddr, t.source, t.daddr, t.dest, key);

}




