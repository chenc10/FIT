#ifndef _HASH_H_
#define _HASH_H_

typedef struct _TPTD_Key{
	int sip:12;
	int sport:4;
	int dip:12;
	int dport:4;
}TPTD_Key;

void init_hash();
u_int
mkhash (u_int , u_short , u_int , u_short);
u_int mkhash_gre (u_int src, u_int dest);

//TPTD_Key TPTD_Mkhash(struct tuple4 );

#endif
