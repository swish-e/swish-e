/*
 * DocProperties.c, DocProperties.h
 *
 * Functions to manage the index's Document Properties 
 *
 * File Created.
 * M. Gaulin 8/10/98
 * Jose Ruiz 2000/10 many modifications
 * Jose Ruiz 2001/01 many modifications
 *
 * 2001-01-26  rasc  getPropertyByname changed
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

void swapDocPropertyMetaNames (docPropertyEntry *pe, struct metaMergeEntry *me);

char **getResultProperties (SWISH *, IndexFILE *, docPropertyEntry *);
char **getResultSortProperties (SWISH *, IndexFILE *, docPropertyEntry *);
int isSortProp (SWISH *);
RESULT *sortresultsbyproperty (SWISH *, int );

int initSortResultProperties (SWISH *);

docPropertyEntry *DupProps (docPropertyEntry *);

void FreeOutputPropertiesVars (SWISH *);

PropValue * getResultPropertyByName (SWISH *, char *, RESULT *);

char *getPropAsString(IndexFILE *, docPropertyEntry *);

void getSwishInternalProperties(struct file *, IndexFILE *);




/*
   -- Mapping AutoProperties <-> METANAMES  
   -- should be the same
*/

#define AUTOPROPERTY_DOCPATH      "swishdocpath"
#define AUTOPROPERTY_TITLE        "swishtitle"
#define AUTOPROPERTY_LASTMODIFIED "swishlastmodified"
#define AUTOPROPERTY_SUMMARY      "swishdescription"
#define AUTOPROPERTY_STARTPOS     "swishstartpos"
#define AUTOPROPERTY_DOCSIZE      "swishdocsize"
#define AUTOPROPERTY_RESULT_RANK  "swishrank"
#define AUTOPROPERTY_INDEXFILE    "swishdbfile"
#define AUTOPROPERTY_REC_COUNT    "swishreccount"

   
