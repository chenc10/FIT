/*Readme:
	It is used for  judging the type of application with the features extracted.
	Input:
		1. ReadFileName -r : the txt file after exerting TPTD
		2. FeatureFileName -f : the txt file for recording featured extracted from a single application
*/
//output: 
//		result.dat
//output format description: 
//		each line describes a node in the tree. And the number of the blanks at the begining of each line is the the level of the node
//		other details are illustrated in output file
#include<stdio.h>
#include<malloc.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>

#define MEMSIZE 50000
#define PACKETNUMSIZE 20

char  ReadFileName[128] = "Dat.dat";
char FeatureFileName[128] = "udp_cs";
char  WriteFileName[128] = "MatchResult.dat";

typedef struct item
{
	int PacketSeqNum;//
	int ByteSeqNum;
	char	Pload[3];
	float Support;
	bool IsVisited;
}Item;
typedef struct transaction
{
	Item ** ItemArray;//array of pointers, each pointer refers to a packet
	//int ItemNum;
	int * PacketNum;//Currently we assign the number of packets in a flow as 20.
}Transaction;


void get_size(int & m);//get the size of the data matrix , the number of Tansactions and the number of items inside each Transaction
void make_transaction(Transaction * T, int TransactionSeqNum, char * str);
void make_item(Item *,int ,int , char *);

int check(Transaction * MyTransaction);
int hex_asc(char *);
int get_position(char *);


int main(int argc, char * argv[])
{
	int opt;
	int InitType;
	while((opt = getopt(argc,argv,"r:f:n:")) != EOF)
	{
		switch(opt)
		{
		case 'r':
			memcpy(ReadFileName,optarg,strlen(optarg));
			ReadFileName[strlen(optarg)] = '\0';
			break;
		case 'n':
			InitType = atoi(optarg);
			break;
		}
	}

	char * str;
	str = new char[MEMSIZE];
	//char str[MEMSIZE];
	FILE * wfr = fopen(WriteFileName, "w");	
	Transaction * MyTransaction;
	MyTransaction = (Transaction *)malloc(sizeof(Transaction));
	FILE * fr = fopen(ReadFileName,"r");
	InitType = 4;
	for( int i = 0; fgets(str,MEMSIZE,fr); i ++)
	{//transfer each line of the data into a Transaction, we regard that there are no more than 20000 letters each line
		fprintf(wfr,"%d %d ", InitType, InitType == atoi(str));
		if(str[1] == ' ')
			str = str + 2;
		else
			str = str + 3;
		make_transaction(MyTransaction,i,str);
		fprintf(wfr,"%d\n",check(MyTransaction));
	}
	fclose(fr);
	//till now, all the data in the file has been read into the memory
	//initial the structure of the Tree
	return 0;
}
int get_position(char * str)
{
	int num;
	num = 0;
	if(str[0] < 48 || str[0] > 57)
		exit(2);
	num += str[0] - 48;
	if(str[1] > 47 && str[1] < 58)
	{
		num *= 10;
		num += str[1] - 48;
	}
	return num;
}
int hex_asc(char * hex)
{
	int asc;
	asc = 0;
	if(hex[0] > 64)
		asc = asc + hex[0] - 55;
	else
		asc = asc + hex[0] - 48;
	asc = asc * 16;
	if(hex[1] > 64)
		asc = asc + hex[1] - 55;
	else
		asc = asc + hex[1] - 48;
	return asc;
}
int check(Transaction * MyTransaction)
{
	for( int i = 0; MyTransaction->PacketNum[i]; i ++)
	{
		if( MyTransaction->PacketNum[i] <= 3 || (MyTransaction->ItemArray[i][3].Pload[0] != '4' && MyTransaction->ItemArray[i][3].Pload[0] != '5'))
		{
			if( MyTransaction->PacketNum[i] <= 3)
				fprintf(stderr,"aa\n");
			else
				fprintf(stderr,"bb\n");
			return 0;
		}
		if( i > 0)
		{
			if( !strcmp(MyTransaction->ItemArray[i][3].Pload, MyTransaction->ItemArray[i-1][3].Pload))
				return 0;
		}
	}
		fprintf(stderr,"ok\n");
	return 1;
	/*for( int i = 0, sign = 1; i < MyFeature->FeatureNum; i ++)
	{
		for( int j = 0; j < MyFeature->FeatureSize[i]; j ++)
		{
			if( MyTransaction->PacketNum[MyFeature->FeatureItems[i][j].PacketSeqNum] <=  MyFeature->FeatureItems[i][j].ByteSeqNum )
			{
				sign = 0;
				break;
			}
			if( strcmp( MyTransaction->ItemArray[MyFeature->FeatureItems[i][j].PacketSeqNum][MyFeature->FeatureItems[i][j].ByteSeqNum].Pload, MyFeature->FeatureItems[i][j].Pload))
			{
				sign = 0;
				break;
			}
		}
		if( sign == 1 )
			return 1;
		else
			sign = 1;
	}
	return 0;*/
}
void make_transaction(Transaction * T, int TransactionSeqNum, char * str)
{//read the data in a line into a Transaction
	//extract each 6 letters into a item
	T->PacketNum = (int *)malloc(PACKETNUMSIZE*sizeof(int));
	T->ItemArray = (Item **)malloc(PACKETNUMSIZE*sizeof(Item *));
	for( int i = 0; i < PACKETNUMSIZE; i ++)
	{
		T->PacketNum[i] = 0;//initialize all the packetnumber into 0
		T->ItemArray[i] = 0;
	}
	
	for( int i = 0, j = 0; str[i*8] != 10 && j < PACKETNUMSIZE; i ++)
		{
			if(str[i*8] == '!')
				{
					j ++;
					continue;
				}
			T->PacketNum[j]++;// record the length(number of items) of current Transaction
		}
	for( int i = 0; i < PACKETNUMSIZE && T->PacketNum[i]; i ++)
		T->ItemArray[i] = (Item *)malloc(sizeof(Item) * T->PacketNum[i]);
	for( int i = 0, j = 0, k = 0; str[i*8] != 10 && j < PACKETNUMSIZE; i ++)
		{
			if(str[i*8] == '!')
				{
				j ++;//row ++
				k = 0;// column set to 0
				continue;
				}
			make_item((T->ItemArray[j]) + k,j,k, str + i*8);
			k ++;
		}
}

void make_item(Item * I,int j, int k, char * str)
{//read each letter into the item

	I->PacketSeqNum = j;
	I->ByteSeqNum = k;
	I->Pload[0] = str[5];
	I->Pload[1] = str[6];
	I->Pload[2] = '\0';
	I->Support = 0;
	I->IsVisited = 0;
}

	
