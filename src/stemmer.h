/*
** stemmer.h
*/

typedef enum {
    STEM_OK,
    STEM_NOT_ALPHA,     /* not all alpha */
    STEM_TOO_SMALL,     /* word too small to be stemmed */
    STEM_WORD_TOO_BIG,  /* word it too large to stem, would would be too large */
    STEM_TO_NOTHING    /* word stemmed to the null string */
    
} STEM_RETURNS;
    

int Stem (char **, int *);

