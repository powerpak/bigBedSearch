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

char *field = "name";
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
    "   -field=fieldName - use index on field name, default is \"name\"\n"
    );
}

/* Command line validation table. */
static struct optionSpec options[] = {
   {"maxItems", OPTION_INT},
   {"field", OPTION_STRING},
   {NULL, 0},
};


void bigBedSearch(char *bigBedFile, char *query, char *outFile)
/* bigBedSearch - Search bigBedFile on its extraIndices and write to outFile
    in BED format the rows that have an indexed field prefixed by the query */
{
struct bbiFile *bbi = bigBedFileOpen(bigBedFile);
FILE *f = mustOpen(outFile, "w");
struct lm *lm = lmInit(0);
struct bigBedInterval *intervalList;
int fieldIx;
struct bptFile *bpt = bigBedOpenExtraIndex(bbi, field, &fieldIx);

intervalList = bigBedPrefixQuery(bbi, bpt, fieldIx, query, maxItems, lm);

bigBedIntervalListToBedFile(bbi, intervalList, f);
carefulClose(&f);
}


int main(int argc, char *argv[])
/* Process command line. */
{
optionInit(&argc, argv, options);
if (argc != 4)
    usage();
maxItems = optionInt("maxItems", maxItems);
field = optionVal("field", field);
bigBedSearch(argv[1], argv[2], argv[3]);
return 0;
}

