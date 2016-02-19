/* bigBedPrefixSearch - handles searching of bigBeds on their extraIndices
   but allows inexact matching by prefixes */

#include "common.h"
#include "hash.h"
#include "linefile.h"
#include "obscure.h"
#include "dystring.h"
#include "rangeTree.h"
#include "cirTree.h"
#include "bPlusTree.h"
#include "basicBed.h"
#include "asParse.h"
#include "zlibFace.h"
#include "sig.h"
#include "udc.h"
#include "bbiFile.h"
#include "bigBed.h"
#include "bigBedPrefixSearch.h"

/**** The following are copied from bigBed.c ...  
 **** I would export them via bigBed.h but they are static, therefore not linkable ****/

struct offsetSize 
/* Simple file offset and file size. */
    {
    bits64 offset; 
    bits64 size;
    };


typedef boolean (*BbFirstWordMatch)(char *line, int fieldIx, void *target);
/* A function that returns TRUE if first word in tab-separated line matches target. */


static void extractField(char *line, int fieldIx, char **retField, int *retFieldSize)
/* Go through tab separated line and figure out start and size of given field. */
{
int i;
fieldIx -= 3;   /* Skip over chrom/start/end, which are not in line. */
for (i=0; i<fieldIx; ++i)
    {
    line = strchr(line, '\t');
    if (line == NULL)
        {
        warn("Not enough fields in extractField of %s", line);
        internalErr();
        }
    line += 1;
    }
char *end = strchr(line, '\t');
if (end == NULL)
    end = line + strlen(line);
*retField = line;
*retFieldSize = end - line;
}


static int cmpOffsetSizeRef(const void *va, const void *vb)
/* Compare to sort slRef pointing to offsetSize.  Sort is kind of hokey,
 * but guarantees all items that are the same will be next to each other
 * at least, which is all we care about. */
{
const struct slRef *a = *((struct slRef **)va);
const struct slRef *b = *((struct slRef **)vb);
return memcmp(a->val, b->val, sizeof(struct offsetSize));
}


static struct fileOffsetSize *fosFromRedundantBlockList(struct slRef **pBlockList, 
    boolean isSwapped)
/* Convert from list of references to offsetSize format to list of fileOffsetSize
 * format, while removing redundancy.   Sorts *pBlockList as a side effect. */
{
/* Sort input so it it easy to uniquify. */
slSort(pBlockList, cmpOffsetSizeRef);
struct slRef *blockList = *pBlockList;

/* Make new fileOffsetSize for each unique offsetSize. */
struct fileOffsetSize *fosList = NULL, *fos;
struct offsetSize lastOffsetSize = {0,0};
struct slRef *blockRef;
for (blockRef = blockList; blockRef != NULL; blockRef = blockRef->next)
    {
    if (memcmp(&lastOffsetSize, blockRef->val, sizeof(lastOffsetSize)) != 0)
        {
        memcpy(&lastOffsetSize, blockRef->val, sizeof(lastOffsetSize));
        AllocVar(fos);
        if (isSwapped)
            {
            fos->offset = byteSwap64(lastOffsetSize.offset);
            fos->size = byteSwap64(lastOffsetSize.size);
            }
        else
            {
            fos->offset = lastOffsetSize.offset;
            fos->size = lastOffsetSize.size;
            }
        slAddHead(&fosList, fos);
        }
    }
slReverse(&fosList);
return fosList;
}


static struct bigBedInterval *bigBedIntervalsMatchingNameUpToLimit(struct bbiFile *bbi, 
    struct fileOffsetSize *fosList, BbFirstWordMatch matcher, int fieldIx, 
    void *target, int maxItems, struct lm *lm)
/* Return list of intervals inside of sectors of bbiFile defined by fosList where the name 
 * matches target somehow. */
{
struct bigBedInterval *interval, *intervalList = NULL;
struct fileOffsetSize *fos;
boolean isSwapped = bbi->isSwapped;
for (fos = fosList; fos != NULL; fos = fos->next)
    {
    /* Read in raw data */
    udcSeek(bbi->udc, fos->offset);
    char *rawData = needLargeMem(fos->size);
    udcRead(bbi->udc, rawData, fos->size);

    /* Optionally uncompress data, and set data pointer to uncompressed version. */
    char *uncompressedData = NULL;
    char *data = NULL;
    int dataSize = 0;
    if (bbi->uncompressBufSize > 0)
        {
        data = uncompressedData = needLargeMem(bbi->uncompressBufSize);
        dataSize = zUncompress(rawData, fos->size, uncompressedData, bbi->uncompressBufSize);
        }
    else
        {
        data = rawData;
        dataSize = fos->size;
        }

    /* Set up for "memRead" routines to more or less treat memory block like file */
    char *blockPt = data, *blockEnd = data + dataSize;
    struct dyString *dy = dyStringNew(32); // Keep bits outside of chrom/start/end here


    /* Read next record into local variables. */
    while (blockPt < blockEnd && maxItems != 0)
        {
        bits32 chromIx = memReadBits32(&blockPt, isSwapped);
        bits32 s = memReadBits32(&blockPt, isSwapped);
        bits32 e = memReadBits32(&blockPt, isSwapped);
        int c;
        dyStringClear(dy);
        while ((c = *blockPt++) >= 0)
            {
            if (c == 0)
                break;
            dyStringAppendC(dy, c);
            }
        if ((*matcher)(dy->string, fieldIx, target))
            {
            lmAllocVar(lm, interval);
            interval->start = s;
            interval->end = e;
            interval->rest = cloneString(dy->string);
            interval->chromId = chromIx;
            slAddHead(&intervalList, interval);
            if (maxItems > 0)
                maxItems -= 1;
            }
        }

    /* Clean up temporary buffers. */
    dyStringFree(&dy);
    freez(&uncompressedData);
    freez(&rawData);
    }
slReverse(&intervalList);
return intervalList;
}


/**** The following code is new.
 ****                            ****/


static void rFindMultiPrefix(struct bptFile *bpt, bits64 blockStart, void *prefix, 
    int prefixSize, int *pMaxItems, struct slRef **pList)
/* Find values corresponding to keys with the prefix and add them to pList.  You'll need to 
 * do a slRefFreeListAndVals() on the list when done. */
{
/* Seek to start of block. */
udcSeek(bpt->udc, blockStart);

/* Read block header. */
UBYTE isLeaf;
UBYTE reserved;
bits16 i, childCount;
udcMustReadOne(bpt->udc, isLeaf);
udcMustReadOne(bpt->udc, reserved);
boolean isSwapped = bpt->isSwapped;
childCount = udcReadBits16(bpt->udc, isSwapped);

int keySize = bpt->keySize;
UBYTE keyBuf[keySize];          /* Place to put a key, buffered on stack. */
UBYTE valBuf[bpt->valSize];     /* Place to put a value, buffered on stack. */

if (isLeaf)
    {
    for (i=0; i<childCount; ++i)
        {
        if (*pMaxItems == 0)
            return;
        udcMustRead(bpt->udc, keyBuf, keySize);
        udcMustRead(bpt->udc, valBuf, bpt->valSize);
        if (memcmp(prefix, keyBuf, prefixSize) == 0)
            {
            void *val = cloneMem(valBuf, bpt->valSize);
            refAdd(pList, val);
            if (*pMaxItems > 0)
                *pMaxItems -= 1;
            }
        }
    }
else
    {
    /* Read first key and first file offset. */
    udcMustRead(bpt->udc, keyBuf, keySize);
    bits64 lastFileOffset = udcReadBits64(bpt->udc, isSwapped);
    bits64 fileOffset = lastFileOffset;
    int lastCmp = memcmp(prefix, keyBuf, prefixSize);

    /* Loop through remainder. */
    for (i=1; i<childCount; ++i)
        {
        if (*pMaxItems == 0)
            return;
        udcMustRead(bpt->udc, keyBuf, keySize);
        fileOffset = udcReadBits64(bpt->udc, isSwapped);
        int cmp = memcmp(prefix, keyBuf, prefixSize);
        if (lastCmp >= 0 && cmp <= 0)
            {
            bits64 curPos = udcTell(bpt->udc);
            rFindMultiPrefix(bpt, lastFileOffset, prefix, prefixSize, pMaxItems, pList);
            udcSeek(bpt->udc, curPos);
            }
        if (cmp < 0)
            return;
        lastCmp = cmp;
        lastFileOffset = fileOffset;
        }
    
    if (*pMaxItems == 0)
        return;
    /* If made it all the way to end, do last one too. */
    rFindMultiPrefix(bpt, fileOffset, prefix, prefixSize, pMaxItems, pList);
    }
}


static struct slRef *bptFileFindMultiplePrefix(struct bptFile *bpt, void *prefix, 
    int prefixSize, int maxItems, int valSize)
/* Find all values associated with a key matching prefix.  Store this in ->val item of returned list. 
 * Do a slRefFreeListAndVals() on list when done. */
{
struct slRef *list = NULL;

if (prefixSize > bpt->keySize)
    return list;

/* Make sure the valSize matches what's in file. */
if (valSize != bpt->valSize)
    errAbort("Value size mismatch between bptFileFind (valSize=%d) and %s (valSize=%d)",
            valSize, bpt->fileName, bpt->valSize);

rFindMultiPrefix(bpt, bpt->rootOffset, prefix, prefixSize, &maxItems, &list);

slReverse(&list);
return list;
}


static boolean bbWordMatchesPrefix(char *line, int fieldIx, void *target)
/* Return true if first word of line is same as target, which is just a string. */
{
char *query = target;
int fieldSize;
char *field;
extractField(line, fieldIx, &field, &fieldSize);
return strlen(query) <= fieldSize && memcmp(query, field, strlen(query)) == 0;
}


static struct fileOffsetSize *bigBedChunksMatchingPrefix(struct bbiFile *bbi, 
    struct bptFile *index, char *prefix, int maxItems)
/* Get list of file chunks that match prefix.  Can slFreeList this when done. */
{
struct slRef *blockList = bptFileFindMultiplePrefix(index, 
    prefix, strlen(prefix), maxItems, sizeof(struct offsetSize));
struct fileOffsetSize *fosList = fosFromRedundantBlockList(&blockList, bbi->isSwapped);
slRefFreeListAndVals(&blockList);
return fosList;
}


struct bigBedInterval *bigBedPrefixQuery(struct bbiFile *bbi, struct bptFile *index,
    int fieldIx, char *prefix, int maxItems, struct lm *lm)
/* Return list of intervals matching prefix in the fieldIx'th field, as indexed in index.
   These intervals will be allocated out of lm. 
   Can limit the total number of intervals by setting maxItems to a positive integer. */
{
/* This will still overfetch because there can be multiple items per chunk
   but at the very least, we definitely do not want to fetch more chunks than items */
struct fileOffsetSize *fosList = bigBedChunksMatchingPrefix(bbi, index, prefix, maxItems);

struct bigBedInterval *intervalList = bigBedIntervalsMatchingNameUpToLimit(bbi, fosList, 
    bbWordMatchesPrefix, fieldIx, prefix, maxItems, lm);

slFreeList(&fosList);
return intervalList;
}