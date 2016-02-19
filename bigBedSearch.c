/* bigBedSearch - Searches the extraIndexes in a bigBed file for a prefix and
 *    prints out the matching rows in BED format. Kind of like bigBedNamedItems but
 *    allows matching on a _prefix_ instead of the entire term, and searches over
 *    multiple fields at once.
 *
 *    See include/bbiFile.h for a complete description of the bigBed/bigWig format. */

/* Copyright (C) 2016 Ted Pak
 * See README in this directory for licensing information. */

#include "common.h"
#include "linefile.h"
#include "hash.h"
#include "options.h"
#include "localmem.h"
#include "udc.h"
#include "bPlusTree.h"
#include "bigBed.h"
#include "obscure.h"
#include "zlibFace.h"
#include "bigBedPrefixSearch.h"

char *fields = NULL;
int maxItems = -1;

void usage()
/* Explain usage and exit. */
{
errAbort(
    "bigBedSearch - Search for items that begin with the given name in a bigBed file\n"
    "usage:\n"
    "   bigBedSearch file.bb query output.bed\n"
    "options:\n"
    "   -maxItems=N - if set, restrict output to first N items\n"
    "   -fields=fieldList - search on this field name (OR field names, separated by commas).\n"
    "        Default is to search all indexes, in the order they were saved.\n"
    );
}

/* Command line validation table. */
static struct optionSpec options[] = {
   {"maxItems", OPTION_INT},
   {"fields", OPTION_STRING},
   {NULL, 0},
};


void bigBedSearch(char *bigBedFile, char *query, char *outFile)
/* bigBedSearch - Search bigBedFile on its extraIndices and write to outFile
    in BED format the rows that have an indexed field prefixed by the query */
{
struct bbiFile *bbi = bigBedFileOpen(bigBedFile);
FILE *f = mustOpen(outFile, "w");
struct lm *lm = lmInit(0);
struct bigBedInterval *intervalList, *interval;
int fieldIx;
struct slName *fieldList = slNameListFromString(fields, ',');
struct slName *currIndex, *extraIndices;

if (fields != NULL)
    extraIndices = fieldList;
else
    extraIndices = bigBedListExtraIndexes(bbi);

for (currIndex = extraIndices; currIndex != NULL && maxItems != 0; currIndex = currIndex->next)
    {
    struct bptFile *bpt = bigBedOpenExtraIndex(bbi, currIndex->name, &fieldIx);
    intervalList = bigBedPrefixQuery(bbi, bpt, fieldIx, query, maxItems, lm);
    int intervalListCount = 0;
    
    if (maxItems > 0)
        maxItems -= slCount(intervalList);
    
    bigBedIntervalListToBedFile(bbi, intervalList, f);    
    }

carefulClose(&f);
}


int main(int argc, char *argv[])
/* Process command line. */
{
optionInit(&argc, argv, options);
if (argc != 4)
    usage();
maxItems = optionInt("maxItems", maxItems);
fields = optionVal("fields", fields);
bigBedSearch(argv[1], argv[2], argv[3]);
return 0;
}

