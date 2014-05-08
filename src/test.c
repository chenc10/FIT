#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>

struct tuple4
{
  u_int16_t source;
  u_int16_t dest;
  u_int32_t saddr;
  u_int32_t daddr;
};
typedef struct _TPTD_Token{
	struct tuple4 addr;
	struct tuple4 raddr;
	int procotol;
	u_int32_t hash;
	u_int32_t rhash;
	int detect_state;
	int token_state;
	int lock_state;
	void *tcpptr;
	void *tcplockptr;
	void *rtcplockptr;
	void *header;
	void *data;
	char suggestname[32];
}TPTD_Token;

int main(int args, char ** argv){
	int i;
	int temp;
	srand((unsigned)time(NULL));
	for(i=0; i<1024; i++){
		temp = rand();
		if((temp % 6)==4){
			fprintf(stderr, "%d\n", temp%19);
		}
	}
}
