#ifndef WIN32
#include <sys/times.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <time.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <inttypes.h>
#include <getopt.h>
#include <random>

extern "C" {
#include "RAxML/globalVariables.h"
#include "RAxML/rmq.h"
}

static int sortIntegers(const void *a, const void *b)
{
	int 
		ia = *(int *)(a),
	ib = *(int *)(b);

	if(ia == ib)
		return 0;

	if(ib > ia)
		return -1;
	else
		return 1;
}

void getNumberOfTrees(tree *tr, FILE *f)
{
	int 
		trees = 0,
	ch;

	while((ch = fgetc(f)) != EOF)
		if(ch == ';')
			trees++;

	assert(trees > 0);

	tr->numberOfTrees = trees;

	rewind(f);
}

void copyArgs(char **targetVariable, char *sourceArgv)
{
	
	size_t length = strlen(sourceArgv)+1;
	*targetVariable = (char *)rax_malloc(length * sizeof(char *));
	memcpy(*targetVariable, sourceArgv, length);
}

void prepareRefernceTree(tree *tr, int *taxonToReduction, int *taxonToEulerIndex, int *taxonToLabel, int *labelToTaxon,  int *eulerIndexToLabel)
{
	int 
		newcount = 0; //counter used for correct traversals

	/* Preorder-Traversal of the large tree */
	preOrderTraversal(tr->start->back,tr->mxtips, tr->start->number, taxonToLabel, labelToTaxon, &newcount);

	newcount = 0; //counter set to 0 to be now used for Eulertraversal

	/* Init taxonToEulerIndex and taxonToReduction */
	int 
		ix;

	for(ix = 0; ix < tr->mxtips; ++ix)    
		taxonToEulerIndex[ix] = -1;    
  
	for(ix = 0; ix < (2*tr->mxtips - 2); ++ix)    
		taxonToReduction[ix] = -1;    


	/* Eulertraversal of the large tree*/
	unrootedEulerTour(tr->start->back,tr->mxtips, eulerIndexToLabel, taxonToLabel, &newcount, taxonToEulerIndex);

	/* Creating RMQ Datastructure for efficient retrieval of LCAs, using Johannes Fischers Library rewritten in C
	Following Files: rmq.h,rmqs.c,rmqs.h are included in Makefile.RMQ.gcc */
	RMQ_succinct(eulerIndexToLabel,4*tr->mxtips - 5);
	
}
void setupGeneTree(tree *geneTree, int taxaGeneTree){
	nodeptr	p0, p, q;
	int
		i,
	j,	
	tips,
	inter;
		
	tips = taxaGeneTree;
	inter = tips-1;
		
	geneTree->numberOfTrees = -1;
	geneTree->rooted = FALSE;
	//gives the maximum number of branches for one leaf
	geneTree->numBranches = 0;
	//gives the total number of inner branches
	geneTree->numberOfBranches = 0;	

	geneTree->treeStringLength = 
		2 * (size_t)tips + //parentheses
			2 * (size_t)tips * 64 + //branche lengths with : and . and branch labels and
				(size_t)tips + //commas
					1 + //closing semicolon 
						(size_t)tips * nmlngth; //taxon names

	geneTree->tree_string	= (char*)rax_calloc(geneTree->treeStringLength, sizeof(char)); 
		
	if (!(p0 = (nodeptr) rax_malloc((tips + 3*inter) * sizeof(node))))
	{
		printf("ERROR: Unable to obtain sufficient tree memory\n");
		return ;
	}

	if (!(geneTree->nodep = (nodeptr *) rax_malloc((2*tips) * sizeof(nodeptr))))
	{
		printf("ERROR: Unable to obtain sufficient tree memory, too\n");
		return ;
	}
	
	geneTree->nodep[0] = (node *) NULL;		/* Use as 1-based array */

	for (i = 1; i <= tips; i++)
	{
		p = p0++;

		p->hash	 =	KISS32(); /* hast table stuff */
		p->x			=	0;
		p->number =	i;
		p->next	 =	p;
		p->back	 = (node *)NULL;
		p->bInf	 = (branchInfo *)NULL;
		p->support = -1;
		p->inTree = FALSE;

		geneTree->nodep[i] = p;
	}

	for (i = tips + 1; i <= tips + inter; i++)
	{
		q = (node *) NULL;
		for (j = 1; j <= 3; j++)
		{	 
			p = p0++;
			if(j == 1)
				p->x = 1;
			else
				p->x =	0;
				
			p->number = i;
			p->next	 = q;
			p->bInf	 = (branchInfo *)NULL;
			p->back	 = (node *) NULL;
			p->hash	 = 0;
			p->support = -2;

			q = p;
		}
		p->next->next->next 	= p;
		geneTree->nodep[i] 		= p;
	}

	geneTree->likelihood	= unlikely;
	geneTree->start			= (node *) NULL;
	geneTree->mxtips		= taxaGeneTree;
	geneTree->ntips			= 0;
	geneTree->nextnode		= 0;
}
void getTaxaDistribution(tree *tr, FILE  *input)
{
	char
		*word = NULL;
	word = (char*)rax_malloc(25*sizeof(char));
	char ch;


	unsigned long count;	
	for(int i = 1; i <= tr->mxtips; i++){
		count = 0;
		int read_counter;
		while((ch = treeGetCh(input)) == '('){}
		ungetc(ch,input);
		while(!feof(input))
		{
			read_counter = fscanf(input, "%[^,:();%f]%*[%f:.,();]", word);
			if(read_counter >= 0) 
			{
				if(strcmp(word, tr->nameList[i])==0)	count++;
			}
		}
		rewind(input);
		if(i == 1)
		{
			tr->taxaOccurencePrefixSum[i] = count;
		}
		else
		{
			tr->taxaOccurencePrefixSum[i] = tr->taxaOccurencePrefixSum[i-1] + count;
		}
		tr->nodep[i]->rec_distr = (float)count/tr->numberOfTrees;
	}
	rax_free(word);
}

void getGeneTreeStatistics(tree *tr, char *geneTreeFileName, analdef *adef, int *taxonToReduction, int *taxonToEulerIndex, int *taxonToLabel, int *labelToTaxon, int *eulerIndexToLabel)
{
	FILE 
		*input = myfopen(geneTreeFileName, "r");
    
	tree 
		*smallTree = (tree *)rax_malloc(sizeof(tree));
	
	float
		rf_dist;
	
	/* now see how many small trees we have */
	getNumberOfTrees(tr, input);
	
	tr->geneLeafDistributions = (int *)rax_calloc(tr->numberOfTrees + 1, sizeof(int)); 
	tr->geneRFDistances = (float *)rax_calloc(tr->numberOfTrees + 1, sizeof(float)); 
	tr->taxaOccurencePrefixSum = (unsigned long *)rax_calloc(tr->mxtips + 1, sizeof(unsigned long)); 

	getTaxaDistribution(tr, input);
	allocateMultifurcations(tr, smallTree);
	
	
	for(int i = 0; i < tr->numberOfTrees;  i++)
	{
		int numberOfSplits = readMultifurcatingTree(input, smallTree, adef, FALSE); //LR set to false
		if(numberOfSplits > 0)
		{
			rf_dist = calculateRFDistance(tr, smallTree, numberOfSplits, taxonToReduction, taxonToEulerIndex, taxonToLabel, labelToTaxon, eulerIndexToLabel);
		}	
		tr->geneLeafDistributions[i] = smallTree->ntips;
		tr->geneRFDistances[i] = rf_dist;
	}
	freeMultifurcations(smallTree);
	rax_free(smallTree);
	fclose(input);
}

void freeTree(tree *tr)
{
	rax_free(tr->tree_string);
	rax_free(tr->nodep[1]);
	rax_free(tr->nodep);
	rax_free(tr->taxaOccurencePrefixSum);
}

void freeReferenceTree(tree *tr)
{
	//freeHashTable((hashtable*)tr->nameHash);
	//rax_free(tr->nameHash->table);
	rax_free(tr->nameHash);
	for(int i =1; i <= tr->ntips; i++)
	{
		rax_free(tr->nameList[i]);
	}
	rax_free(tr->nameList);
	rax_free(tr->geneLeafDistributions);
	rax_free(tr->geneRFDistances);
	freeTree(tr);
	
}