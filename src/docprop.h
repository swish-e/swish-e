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
unsigned char *storeDocProperties (docPropertyEntry *, int *);

unsigned char* readNextDocPropEntry(char **, int *, int *);


void addDocProperty (docPropertyEntry **, int , unsigned char* ,int );

docPropertyEntry *fetchDocProperties (char * );

int initSearchResultProperties (SWISH *);
void addSearchResultDisplayProperty (SWISH *, char* );
void addSearchResultSortProperty (SWISH *, char*, int );
unsigned char* lookupDocPropertyValue (int , char *, int *);
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

char *getPropAsString(IndexFILE *, docPropertyEntry *);
