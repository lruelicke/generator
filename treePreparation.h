#ifndef __TREE_PREPARATION_INCLUDED__
#define __TREE_PREPARATION_INCLUDED__

extern "C" {
#include "RAxML/axml.h"
#include "RAxML/rmq.h"
}
#include <cstring>
#include <random>

int sortIntegers(const void *a, const void *b);
void getNumberOfTrees(tree *tr, FILE *f);
void copyArgs(char **targetVariable, char *sourceArgv);
int extractTaxaFromTopologyAndAddNew(tree *tr, rawdata *rdta, cruncheddata *cdta, char fileName[1024], int newTaxaCount);
void prepareReferenceTree(tree *tr, int *taxonToReduction, int *taxonToEulerIndex, int *taxonToLabel, int *labelToTaxon,  int *eulerIndexToLabel);
void setupGeneTree(tree *geneTree, int taxaGeneTree);
void getTaxaDistribution(tree *tr, FILE  *input);
float calculateRFDistance(tree *tr, tree *geneTree, int numberOfSplits, int *taxonToReduction, int *taxonToEulerIndex, int *taxonToLabel, int *labelToTaxon, int *eulerIndexToLabel);
void getGeneTreeStatistics(tree *tr, char *geneTreeFileName, analdef *adef, int *taxonToReduction, int *taxonToEulerIndex, int *taxonToLabel, int *labelToTaxon, int *eulerIndexToLabel);
void enlargeTree(tree *tr, int oldTaxaCount, std::default_random_engine *generator);
void freeTree(tree *tr);
void freeReferenceTree(tree *tr);
static char *geneTree2StringREC(char *treestr, tree *tr, nodeptr p, boolean printBranchLengths, boolean printNames, boolean printLikelihood, boolean rellTree, boolean finalPrint, int perGene, boolean branchLabelSupport, boolean printSHSupport, boolean printIC, boolean printSHSupports, int* treeTaxa);
void geneTree2String(char *treestr, tree *tr, nodeptr p, boolean printBranchLengths, boolean printNames, boolean printLikelihood, boolean rellTree, boolean finalPrint, analdef *adef, int perGene , boolean branchLabelSupport, boolean printSHSupport, boolean printIC, boolean printSHSupports, int* treeTaxa);

#endif //__TREE_PREPARATION_INCLUDED__