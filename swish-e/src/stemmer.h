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
    
int Stem (char **, int *, void *);
int Stem_snowball (char **, int *, void *);

char *fuzzy_mode_to_string( FuzzyIndexType mode );
void set_fuzzy_mode( FUZZY_INDEX *fi, char *param );
void get_fuzzy_mode( FUZZY_INDEX *fi, int fuzzy );
void free_fuzzy_mode( FUZZY_INDEX *fi );
int stemmer_applied( INDEXDATAHEADER *header );
void dump_fuzzy_list( void );
