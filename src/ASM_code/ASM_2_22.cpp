/* feature description:
	1.very initial edition
		no comandline parsing
			or any later functions
	2.but it can display all the features in the ASM tree.
T-2.22
*/



//input file��
//		DATA.dat
//input format description��	
//		each 6 letters as an item, and the first 4 is the sequential num, and the last 2 letters are the payload.
//		And items are distingushed by blanks.
//		each line is a Transaction(please be noted that at the end of the line there is always a blank) 
//output: 
//		result.dat
//output format description: 
//		each line describes a node in the tree. And the number of the blanks at the begining of each line is the the level of the node
//		other details are illustrated in output file
#include<stdio.h>
#include<malloc.h>
#include<string.h>
#include<stdlib.h>

char * ReadFileName = "Dat.dat";
char * WriteFileName = "Result.result";
float FThreshold = 0.05; //The first node after spliting covers more than a certain part(FThreshold) of all the transactions in its father
float WThreshold = 0.1; //The first node after spliting covers more than a certain part(WThreshold) of all the transactions in the root node
float DiscardThreshold = 0.8; // The number of transactions discarded when a same-level spliting is made must be limited to a certain threshold(DiscardThreshold, otherwise we refuse to make that split
//#define THRESHOLD 0.1 // This threshold is to ensure that each division can extract the feasure covering enough portion of the Transaction
int SignForMatrix = 0;
typedef struct item
{
	int TransactionSeqNum;
	int ISeqNum;//IseqNum represents the sequential number of this item in the Transaction it belongs to, and it also means the subcript number
	char	Pload[3];
	int VisitedTimes;
	bool IsVisited;
}Item;
typedef struct MostItem
{
	Item * MItem;// the PTR to the item that has appeared most frequently (pointed to the position where the item appered for the first time)
	int MaxNum;//including the number of Transactions that includes this most frequent item, and this is also  MItem->VisitedTimes
}MostItem;
typedef struct transaction
{
	Item * ItemArray;
	int ItemNum;
}Transaction;
typedef struct tnode
{//the node in the tree of the level-spliting algorithm
	Transaction * TTransaction;//the Transaction saved in this node, and the elements in this array should be visited with the help of TTransactionNum
	Item * TItem;//The items as a particular feature saved in this node, and the number of the items is equal to the level
	int TTransactionNum;//to realize the visiting by PTR, we need to record the number of elements in the array
	int TLevel;//the level of current node in the whole tree, which is also the number of the items as particular features.
	int TSerialNum;//Recording the sequential num of current node, and this sequence is based on all the nodes in the tree
	struct tnode * NextTnode;//it workes mainly when node spliting is undergoing, and it is used to record the newly created node, and it is use for creating the childnodearray in the future
	struct tnode * PreviousTnode;
	struct tnode * FatherTnode;//for saving the father node of the current node
	struct tnode ** ChildTnodeArray;//a Array for saving the children of the current node, it works with the help of ChildTnodeNum
	int ChildTnodeNum;//saving the number of children, please be noted that it only counts the children that are confirmed to exist currently(those children who have features clearly)
}Tnode;
typedef struct tree
{//a tree as a result of the level-spliting algorithm
	Tnode * Root;//recording the root of the tree
	int TnodeNum;//recording the number of the nodes in the tree
}Tree;
typedef struct qnode
{//we need a assisting queue in order to realize the visiting level by level
	Tnode * QTnode;
	struct qnode * NextQnode;
	struct qnode * PreviousQnode;
}Qnode;
typedef struct queue
{
	int QnodeNum;
	Qnode * StartQnode;
	Qnode * EndQnode;
}Queue;

void get_size(int & m);//get the size of the data matrix , the number of Tansactions and the number of items inside each Transaction
void make_transaction(Transaction * T, int TransactionSeqNum, char * str);
void make_item(Item *, char *);
void copy_item(Item *, Item *);
void calculate_support(Item *);//calcalate the support of a certain item
void make_split(Tree * , Tnode *);
void step_split_differentlevel(Tree *, Tnode *, Tnode *, Tnode *);
Tnode * step_split_samelevel(Tree *, Tnode *);

int m,n;
int UsedTransactionNum; // UsedTransactionNum is for recording how many transactions are covered by current feature
Transaction * TransactionArray;
MostItem MMostItem;
// To realizing visiting level by level, we need to write a queue 
Queue * MQueue;

void en_queue(Tnode *);
Tnode * de_queue();
int b = 0;
int main(int argc, char * argv[])
{
	if(argc == 2)
		WThreshold = atof(argv[1]);
	if(argc == 3)
	{
		WThreshold = atof(argv[1]);
		FThreshold = atof(argv[2]);
	}
	if(argc == 4)
	{
		WThreshold = atof(argv[1]);
		FThreshold = atof(argv[2]);
		DiscardThreshold = atof(argv[3]);
	}
	//initialize the queue
	MQueue = (Queue *)malloc(sizeof(Queue));
	MQueue->QnodeNum = 0;
	MQueue->StartQnode = (Qnode *)malloc(sizeof(Qnode));
	MQueue->EndQnode = (Qnode *)malloc(sizeof(Qnode));
	//
	MQueue->StartQnode->NextQnode = MQueue->EndQnode;//recording the relationship of the virsital start node and the virtual end node initially
	MQueue->EndQnode->PreviousQnode = MQueue->StartQnode;
	//
	MQueue->StartQnode->PreviousQnode = NULL;//no former node to the virtual start node
	MQueue->EndQnode->NextQnode = NULL;//node latter node to the virtual end node
	MQueue->StartQnode->QTnode = NULL;//NULL in the virtual start node
	MQueue->EndQnode->QTnode = NULL;//NULL in the virtual end node

	//judge m and n
	char str[100000];
	get_size(m);
	
	UsedTransactionNum = m;
	TransactionArray = (Transaction *)malloc(m*sizeof(Transaction));
	FILE * fr = fopen(ReadFileName,"r");
	for( int i = 0; i < m; i ++)
	{//transfer each line of the data into a Transaction, we regard that there are no more than 20000 letters each line
		fgets(str,100000,fr);
		make_transaction(TransactionArray,i,str);
	}
	fclose(fr);
	//till now, all the data in the file has been read into the memory

	//initial the structure of the Tree
	Tree * MTree;
	MTree = (Tree *)malloc(sizeof(MTree));
	MTree->TnodeNum = 0;
	MTree->Root = (Tnode *)malloc(sizeof(Tnode));
	MTree->Root->TTransactionNum = m;
	MTree->Root->TTransaction = (Transaction *)malloc(MTree->Root->TTransactionNum * sizeof(Transaction));//allocate memory for the Transaction in the root
	for( int i = 0; i < MTree->Root->TTransactionNum; i ++)
	{
		MTree->Root->TTransaction[i].ItemArray = TransactionArray[i].ItemArray;//copy all the elements in the TransactionArray into the MTree
		MTree->Root->TTransaction[i].ItemNum = TransactionArray[i].ItemNum;
	}
	MTree->Root->ChildTnodeArray = NULL;
	MTree->Root->ChildTnodeNum = 0;
	MTree->Root->FatherTnode = NULL;
	MTree->Root->NextTnode = NULL;
	MTree->Root->PreviousTnode = NULL;
	MTree->Root->TItem = 0;
	MTree->Root->TLevel = 0;
	MTree->Root->TSerialNum = 0;
	en_queue(MTree->Root);
	//while(MQueue->QnodeNum)
	//{
		//make_split(MTree,de_queue());
	//}
	int ThisLevelNum = 0;
	int LastTnodeLevel = 0; // the level of last Tnode, it is used for judging whether the level has changed
	int TillLastLevelFinishedNum = 0;
	int TillThisLevelFinishedNum = 0;
	//the below code is to input all the data into a file
	FILE * fw = fopen(WriteFileName,"w");
	Tnode * TmpTnode;
	//en_queue(MTree->Root);
	while(MQueue->QnodeNum)
	{

		TmpTnode = de_queue();
		make_split(MTree,TmpTnode);
		if(LastTnodeLevel == TmpTnode->TLevel)
			ThisLevelNum += TmpTnode->TTransactionNum;
		else
		{
			for( int i = 0; i < TmpTnode->TLevel; i ++)
				fprintf(fw,"-");
			fprintf(fw,"%.4f%% of all the Transactions are covered by the features till this level\n", 100 * ((float)ThisLevelNum + TillLastLevelFinishedNum)/MTree->Root->TTransactionNum);
			TillLastLevelFinishedNum = TillThisLevelFinishedNum;
			ThisLevelNum = TmpTnode->TTransactionNum;
			LastTnodeLevel = TmpTnode->TLevel;
		}
		for( int i = 0; i < TmpTnode->TLevel; i ++)
			fprintf(fw," |");
		if(TmpTnode->TSerialNum == 0)
			fprintf(fw,"SeqN:%5d    Numof-Tran:%5d    Numof-Child:%5d    ",TmpTnode->TSerialNum,TmpTnode->TTransactionNum,TmpTnode->ChildTnodeNum);
		else
			fprintf(fw,"SeqN:%5d    F-SeqN:%5d    Numof-Tran:%5d    Numof-Child:%5d    ",TmpTnode->TSerialNum,TmpTnode->FatherTnode->TSerialNum,TmpTnode->TTransactionNum,TmpTnode->ChildTnodeNum);
		fprintf(fw,"feature item:	");
		for( int i = 0; i < TmpTnode->TLevel; i ++)
			fprintf(fw,"(@%d)%s ",TmpTnode->TItem[i].ISeqNum,TmpTnode->TItem[i].Pload);
		fprintf(fw,"\n");
		if(TmpTnode->ChildTnodeNum == 0)
			TillThisLevelFinishedNum += TmpTnode->TTransactionNum;
		//for( int i = 0; i < TmpTnode->ChildTnodeNum; i ++)
			//en_queue(TmpTnode->ChildTnodeArray[i]);
	}
	fprintf(fw,"\n\n%.4f%% of all the Transactions are covered by this feature", 100 * (float)UsedTransactionNum / MTree->Root->TTransactionNum);
	fclose(fw);
	return 0;
}

void get_size(int & m)
{
	char letter;
	m = 0;
	FILE * fr = fopen(ReadFileName,"r");
	while(letter = getc(fr))
	{
		if(letter == '\n')
			m ++;
		if(letter == EOF)
			break;
	}
	fclose(fr);
}

void make_transaction(Transaction * T,int TransactionSeqNum, char * str)
{//read the data in a line into a Transaction
	//extract each 6 letters into a item
	T[TransactionSeqNum].ItemNum = 0;
	for( int i = 0; str[i*7] != '\0'; i ++)
		T[TransactionSeqNum].ItemNum ++;// record the length(number of items) of current Transaction
	T[TransactionSeqNum].ItemArray = (Item *)malloc( sizeof(Item) * T[TransactionSeqNum].ItemNum);
	for( int i = 0; str[i*7] != '\0'; i ++)
	{
		make_item((T[TransactionSeqNum].ItemArray) + i,str + i*7);
		T[TransactionSeqNum].ItemArray[i].TransactionSeqNum = TransactionSeqNum;//for each item, mark the Transaction it belongs to
	}
}

void make_item(Item * I, char * str)
{//read each letter into the item
	int number;
	number = str[0]>96 ? (str[0] - 87):(str[0]>64 ? (str[0] - 55):(str[0] - 48));
	number *= 16;
	number += str[1]>96 ? (str[1] - 87):(str[1]>64 ? (str[1] - 55):(str[1] - 48));
	number *= 16;
	number += str[2]>96 ? (str[2] - 87):(str[2]>64 ? (str[2] - 55):(str[2] - 48));
	number *= 16;
	number += str[3]>96 ? (str[3] - 87):(str[3]>64 ? (str[3] - 55):(str[3] - 48));
	I->ISeqNum = number;
	I->Pload[0] = str[4];
	I->Pload[1] = str[5];
	I->Pload[2] = '\0';
	I->VisitedTimes = 0;
	I->IsVisited = 0;
}

void calculate_support(Item * I, Tnode * MTnode)
{//calulate the number of Transaction including a certain item
	for( int i = 0; i < MTnode->TTransactionNum; i ++)
	{//for a given item, we only need to compare the item at the corresponding position of other Transactions
		if( I->ISeqNum >= MTnode->TTransaction[i].ItemNum)
			continue;// if the sequential number of chosen item exceed the range of items in current Transaction
		if( !(strcmp(MTnode->TTransaction[i].ItemArray[I->ISeqNum].Pload,I->Pload)))
		{// if current Transaction includes the item
			MTnode->TTransaction[i].ItemArray[I->ISeqNum].IsVisited = 1;
			I->VisitedTimes ++;
			MTnode->TTransaction[i].ItemArray[I->ISeqNum].VisitedTimes = I->VisitedTimes;// It's not accurate since the same items do not have same VisitedTimes 1/11
			if(I->VisitedTimes > MMostItem.MaxNum)
			{//recording the item with the most frequency
				MMostItem.MaxNum = I->VisitedTimes;
				MMostItem.MItem = I;
			}
		}
	}
}

void make_split(Tree* MTree, Tnode * Root)
{//get all the children of a certain node
	Tnode * CreatedTnode1, * CreatedTnode2;
	CreatedTnode1 = (Tnode *)malloc(sizeof(Tnode));
	CreatedTnode2 = (Tnode *)malloc(sizeof(Tnode));


	step_split_differentlevel(MTree, Root, CreatedTnode1, CreatedTnode2);


	if(((float)CreatedTnode1->TTransactionNum) / Root->TTransactionNum < FThreshold || ((float)CreatedTnode1->TTransactionNum) / MTree->Root->TTransactionNum < WThreshold)
	{//if a spliting is failed, it is based on the rate of the number of children in created1(if the bigger node is too small)
		MTree->TnodeNum -= 2;
		Root->ChildTnodeNum = 0;
		Root->ChildTnodeArray = 0;
		return;
	}
	//below we should deal with the case where the bigger node is so big that we should discard the rest node
	else if( ((float)CreatedTnode2->TTransactionNum) / Root->TTransactionNum < FThreshold || ((float)CreatedTnode2->TTransactionNum) / MTree->Root->TTransactionNum < WThreshold)
	{//if the bigger node is too big
		MTree->TnodeNum --;
		
		UsedTransactionNum -= CreatedTnode2->TTransactionNum;
		en_queue(Root->ChildTnodeArray[0]);
		return;
	}
	Tnode * CreatedTnode3;
	CreatedTnode3 = step_split_samelevel(MTree,CreatedTnode2);

	for(;;)
	{
		if(((float)CreatedTnode2->TTransactionNum) / CreatedTnode2->FatherTnode->TTransactionNum < FThreshold || ((float)CreatedTnode2->TTransactionNum) / MTree->Root->TTransactionNum < WThreshold)
		{// if the bigger node is too small, we do retreat for this node
			if((float)CreatedTnode3->TTransactionNum / CreatedTnode2->FatherTnode->TTransactionNum > DiscardThreshold)//if the number of effective children discarded is more than certain percentage of the number in Fathernode 
			{//we refuse to discard too many children, we would rather stop growing. And this is to handle the case where there are too many 1-transaction-nodes which is too specific
				MTree->TnodeNum -= 1;
				MTree->TnodeNum -= Root->ChildTnodeNum;
				Root->ChildTnodeArray = NULL;
				Root->ChildTnodeNum = 0;
				return;
			}
			MTree->TnodeNum -= 2;
			Root->ChildTnodeNum --;
			CreatedTnode2->PreviousTnode->NextTnode = NULL;// retreat, prune at the previous node
			UsedTransactionNum -= (CreatedTnode3->TTransactionNum + CreatedTnode2->TTransactionNum);
			break;
		}
		else if(((float)CreatedTnode3->TTransactionNum) / CreatedTnode2->FatherTnode->TTransactionNum < FThreshold || ((float)CreatedTnode3->TTransactionNum) / MTree->Root->TTransactionNum < WThreshold)
		{
			MTree->TnodeNum -= 1;
			CreatedTnode2->NextTnode = NULL;
			UsedTransactionNum -= CreatedTnode3->TTransactionNum;
			break;
		}
		CreatedTnode2 = CreatedTnode3;
		CreatedTnode3 = step_split_samelevel(MTree,CreatedTnode2);
	}
	
	//we need to record the relationship of children and father
	Tnode ** ChildTnodeArray;
	ChildTnodeArray = (Tnode **)malloc(Root->ChildTnodeNum *sizeof(Tnode *));
	Tnode * FirstChild;
	FirstChild = *(Root->ChildTnodeArray);
	for( int i = 0; i < Root->ChildTnodeNum; i ++)
	{
		ChildTnodeArray[i] = FirstChild;
		FirstChild = FirstChild->NextTnode;
	}
	Root->ChildTnodeArray = ChildTnodeArray;
	for( int i = 0; i < Root->ChildTnodeNum; i ++)
	{
		SignForMatrix = 0;
		for( int j = 0; j < Root->ChildTnodeArray[i]->TTransactionNum; j ++)
		{
			if(Root->ChildTnodeArray[i]->TLevel < Root->ChildTnodeArray[i]->TTransaction[j].ItemNum)
			{
				SignForMatrix = 1;
				break;
			}
		}
		if(SignForMatrix)// if the Transactions in this node construct a matrix(all the transactions have the same number of items)
			en_queue(Root->ChildTnodeArray[i]);// if all the items in each transactions are already part of the chosen feature
		// else we needn't to deal with Root->childTnodeNum, because it has been set well in this case
	}
}

void copy_item(Item * A, Item * B)
{
	A->ISeqNum = B->ISeqNum;
	A->IsVisited = B->IsVisited;
	strcpy(A->Pload,B->Pload);
	A->TransactionSeqNum = B->TransactionSeqNum;
	A->VisitedTimes = B->VisitedTimes;
}

//all spliting can be regarded as two kinds: the spliting of the same level and the spliting acrossing levels
void step_split_differentlevel(Tree * MTree, Tnode * Root, Tnode * Created1, Tnode * Created2)
{//deviding the Transactions into to clusters and save them into Created1 and Created2 irrespectively

	MMostItem.MaxNum = 0;
	MMostItem.MItem = NULL;

	for( int i = 0; i < Root->TTransactionNum; i ++)
	{//get the item as the most frequent item
		for( int j= 0,s = 0; j < Root->TTransaction[i].ItemNum; j ++)
		{
			if(!(Root->TTransaction[i].ItemArray[j].IsVisited))
			{
				for( int k = 0; k < Root->TLevel; k ++)
					if((j == Root->TItem[k].ISeqNum) && !(strcmp(Root->TTransaction[i].ItemArray[j].Pload,Root->TItem[k].Pload)))
					{
						s = 1;//if an item has already become a feature item in its father, we then ignore it
						break;
					}
				if(s == 1)
				{
					s = 0;
					continue;
				}
				calculate_support(Root->TTransaction[i].ItemArray + j,Root);
			}
		}
	}
	for( int i = 0; i < Root->TTransactionNum; i ++)
		for( int j = 0; j < Root->TTransaction[i].ItemNum; j ++)
		{
			Root->TTransaction[i].ItemArray[j].IsVisited = 0;//reset the sign
			Root->TTransaction[i].ItemArray[j].VisitedTimes = 0;
		}

	Created1->TTransactionNum = MMostItem.MaxNum;
	Created1->TTransaction = (Transaction *)malloc(Created1->TTransactionNum * sizeof(Transaction));
	Created1->ChildTnodeArray = NULL;
	Created1->ChildTnodeNum = 0;
	Created1->FatherTnode = Root;
	Created1->PreviousTnode = NULL;
	Created1->NextTnode = Created2;
	//copy the object
	Created1->TItem = (Item *)malloc((Root->TLevel + 1)*sizeof(Item));
	for( int i = 0; i < Root->TLevel; i ++)
		copy_item(Created1->TItem + i,Root->TItem + i);
	copy_item(Created1->TItem + Root->TLevel, MMostItem.MItem);
	//
	Created1->TLevel = Root->TLevel + 1;
	MTree->TnodeNum ++;
	Created1->TSerialNum = MTree->TnodeNum;


	//record the features of Created2
	Created2->TTransactionNum = Root->TTransactionNum - MMostItem.MaxNum;
	Created2->TTransaction = (Transaction *)malloc(Created2->TTransactionNum * sizeof(Transaction));
	Created2->ChildTnodeArray = NULL;
	Created2->ChildTnodeNum = 0;
	Created2->FatherTnode = Root;
	Created2->PreviousTnode = Created1;
	Created2->NextTnode = NULL;
	//
	Created2->TItem = (Item *)malloc(Root->TLevel * sizeof(Item));
	for( int i = 0; i < Root->TLevel; i ++)
		copy_item(Created2->TItem + i,Root->TItem + i);
	//
	Created2->TLevel = Root->TLevel + 1;
	MTree->TnodeNum ++;
	Created2->TSerialNum = MTree->TnodeNum;//MTree->TnodeNum;

	Root->ChildTnodeNum = 1;
	Root->ChildTnodeArray = (Tnode **)malloc( (Root->ChildTnodeNum) * sizeof(Tnode *));
	Root->ChildTnodeArray[0] = Created1;
	
	//spliting the Transactions
	for( int i = 0,j = 0,k = 0; i < Root->TTransactionNum; i ++)
		if( Root->TTransaction[i].ItemNum > MMostItem.MItem->ISeqNum && !strcmp(Root->TTransaction[i].ItemArray[MMostItem.MItem->ISeqNum].Pload,MMostItem.MItem->Pload))
		{
			Created1->TTransaction[j].ItemArray = Root->TTransaction[i].ItemArray;
			Created1->TTransaction[j].ItemNum = Root->TTransaction[i].ItemNum;
			j ++;
		}
		else
		{
			Created2->TTransaction[k].ItemArray = Root->TTransaction[i].ItemArray;
			Created2->TTransaction[k].ItemNum = Root->TTransaction[i].ItemNum;
			k ++;
		}
}

Tnode * step_split_samelevel(Tree * MTree, Tnode * Brother)
{//spliting of the same level 
	Tnode * Created;
	MMostItem.MaxNum = 0;
	MMostItem.MItem = NULL;
	Created = (Tnode *)malloc(sizeof(Tnode));
	for( int i = 0; i < Brother->TTransactionNum; i ++)
	{
		for( int j= 0,s = 0; j < Brother->TTransaction[i].ItemNum; j ++)
		{
			if(!(Brother->TTransaction[i].ItemArray[j].IsVisited))
			{
				for( int k = 0; k < Brother->TLevel - 1; k ++)
					if( (j == Brother->TItem[k].ISeqNum) && !strcmp(Brother->TTransaction[i].ItemArray[j].Pload,Brother->TItem[k].Pload))
						s = 1;
				if(s == 1)
				{
					s = 0;
					continue;
				}
				calculate_support(Brother->TTransaction[i].ItemArray + j,Brother);
			}
		}
	}
	for( int i = 0; i < Brother->TTransactionNum; i ++)
		for( int j = 0; j < Brother->TTransaction[i].ItemNum; j ++)
		{
			Brother->TTransaction[i].ItemArray[j].IsVisited = 0;//reset
			Brother->TTransaction[i].ItemArray[j].VisitedTimes = 0;
		}

	Transaction  * BTransaction;
	BTransaction = (Transaction *)malloc(MMostItem.MaxNum*sizeof(Transaction));
	Item * BItem;
	BItem = (Item *)malloc( (Brother->TLevel)*sizeof(Item));
	//
	Brother->ChildTnodeArray = 0;
	Brother->ChildTnodeNum = 0;
	Brother->NextTnode = Created;
	for(int i = 0; i < Brother->TLevel - 1; i ++)
		copy_item(BItem+i,Brother->TItem + i);
	copy_item(BItem + Brother->TLevel - 1,MMostItem.MItem);
	Brother->TItem = BItem;
	
	//dealing with the newly created nodes
	Item * CItem;
	CItem = (Item *)malloc( (Brother->TLevel - 1) * sizeof(Item));
	//
	Created->TTransactionNum = Brother->TTransactionNum - MMostItem.MaxNum;
	Created->TTransaction = (Transaction *)malloc(Created->TTransactionNum * sizeof(Transaction));
	Created->ChildTnodeArray = 0;
	Created->ChildTnodeNum = 0;
	Created->FatherTnode = Brother->FatherTnode;
	Created->PreviousTnode = Brother;
	Created->NextTnode = 0;
	for(int i = 0; i < Brother->TLevel - 1; i ++)
		copy_item(CItem+i,Brother->TItem + i);
	Created->TItem = CItem;
	Created->TLevel = Brother->TLevel;
	MTree->TnodeNum ++;
	Created->TSerialNum = MTree->TnodeNum;//we need to record the sequential number of a newly createdly node

	Brother->FatherTnode->ChildTnodeNum ++;

	for( int i = 0,j = 0,k = 0; i < Brother->TTransactionNum; i ++)
		if( Brother->TTransaction[i].ItemNum > MMostItem.MItem->ISeqNum && !strcmp(Brother->TTransaction[i].ItemArray[MMostItem.MItem->ISeqNum].Pload,MMostItem.MItem->Pload))
		{
			BTransaction[j].ItemArray = Brother->TTransaction[i].ItemArray;
			BTransaction[j].ItemNum = Brother->TTransaction[i].ItemNum;
			j ++;
		}
		else
		{
			Created->TTransaction[k].ItemArray = Brother->TTransaction[i].ItemArray;
			Created->TTransaction[k].ItemNum = Brother->TTransaction[i].ItemNum;
			k ++;
		}

	Brother->TTransactionNum = MMostItem.MaxNum;
	Brother->TTransaction = BTransaction;

	return Created;
}


void en_queue(Tnode * MTnode)
{
	Qnode * MQnode;
	MQnode = (Qnode *)malloc(sizeof(Qnode));

	MQnode->QTnode = MTnode;
	MQnode->NextQnode = MQueue->StartQnode->NextQnode;
	MQnode->PreviousQnode = MQueue->StartQnode;

	MQnode->NextQnode->PreviousQnode = MQnode;
	MQnode->PreviousQnode->NextQnode = MQnode;
	MQueue->QnodeNum ++;
}

Tnode * de_queue()
{
	Tnode * ReturnedTnode;
	ReturnedTnode = MQueue->EndQnode->PreviousQnode->QTnode;
	MQueue->EndQnode->PreviousQnode->PreviousQnode->NextQnode = MQueue->EndQnode;
	MQueue->EndQnode->PreviousQnode = MQueue->EndQnode->PreviousQnode->PreviousQnode;
	MQueue->QnodeNum --;
	return ReturnedTnode;
}
	