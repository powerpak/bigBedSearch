/* bigBedPrefixSearch - handles searching of bigBeds on their extraIndices
   but allows inexact matching by prefixes */

struct bigBedInterval *bigBedPrefixQuery(struct bbiFile *bbi, struct bptFile *index,
    int fieldIx, char *prefix, int maxItems, struct lm *lm);
/* Return list of intervals matching prefix in the fieldIx'th field, as indexed in index.
   These intervals will be allocated out of lm. 
   Can limit the total number of intervals by setting maxItems to a positive integer. */