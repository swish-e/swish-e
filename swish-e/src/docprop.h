/*
 * DocProperties.c, DocProperties.h
 *
 * Functions to manage the index's Document Properties 
 *
 * File Created.
 * M. Gaulin 8/10/98
 * Jose Ruiz 2000/10 many modifications
 * Jose Ruiz 2001/01 many modifications
 */


void freeDocProperties (docPropertyEntry **);
char *storeDocProperties (docPropertyEntry *, int *);

void addDocProperty (docPropertyEntry **, int , char* );

docPropertyEntry *fetchDocProperties (char * );

int initSearchResultProperties (SWISH *);
void addSearchResultDisplayProperty (SWISH *, char* );
void addSearchResultSortProperty (SWISH *, char*, int );
char* lookupDocPropertyValue (int , char *);
void printSearchResultProperties (SWISH *, char **);

void swapDocPropertyMetaNames (docPropertyEntry *, struct metaMergeEntry *);

char **getResultProperties (SWISH *, IndexFILE *, docPropertyEntry *);
char **getResultSortProperties (SWISH *, IndexFILE *, docPropertyEntry *);
int isSortProp (SWISH *);
RESULT *sortresultsbyproperty (SWISH *, int );

int initSortResultProperties (SWISH *);

docPropertyEntry *DupProps (docPropertyEntry *);

void FreeOutputPropertiesVars (SWISH *);

char * getResultPropertyByName (SWISH *, char *, RESULT *);
