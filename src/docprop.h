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
 * 2001-02-09  rasc  printSearchResultProperties changed
 */


void freeDocProperties (docProperties *);
unsigned char *storeDocProperties (docProperties *, int *);

int EncodeProperty( struct metaEntry *meta_entry, char **encodedStr, char *string );
int addDocProperty (docProperties **, struct metaEntry * , unsigned char* ,int, int );

docProperties *fetchDocProperties (char * );

int initSearchResultProperties (SWISH *);
void addSearchResultDisplayProperty (SWISH *, char* );
void addSearchResultSortProperty (SWISH *, char*, int );
void printSearchResultProperties (SWISH *, FILE *f_out, char **);

docProperties *swapDocPropertyMetaNames (docProperties *, struct metaMergeEntry *);

char **getResultProperties (RESULT *);

docProperties *DupProps (docProperties *);

char *getResultPropAsString(RESULT *, int);

char *getDocPropAsString(IndexFILE*, struct file *, int);

void getSwishInternalProperties(struct file *, IndexFILE *);


PropValue * getResultPropertyByName (SWISH *sw, char *name, RESULT *r);
PropValue * getResultPropertyByName_CS (SWISH *sw, char *name, RESULT *r);
int isAutoProperty (char *propname);


/*
   -- Mapping AutoProperties <-> METANAMES  
   -- should be the same
*/

/* all AutoPropteries start with this string ! */

#define AUTOPROPERTY__ID_START__  "swish"

#define AUTOPROPERTY_REC_COUNT    "swishreccount"
#define AUTOPROP_ID__REC_COUNT    1
#define AUTOPROPERTY_RESULT_RANK  "swishrank"
#define AUTOPROP_ID__RESULT_RANK  2
#define AUTOPROPERTY_DOCPATH      "swishdocpath"
#define AUTOPROP_ID__DOCPATH      3
#define AUTOPROPERTY_TITLE        "swishtitle"
#define AUTOPROP_ID__TITLE        4
#define AUTOPROPERTY_DOCSIZE      "swishdocsize"
#define AUTOPROP_ID__DOCSIZE      5
#define AUTOPROPERTY_LASTMODIFIED "swishlastmodified"
#define AUTOPROP_ID__LASTMODIFIED 6
#define AUTOPROPERTY_SUMMARY      "swishdescription"
#define AUTOPROP_ID__SUMMARY      7
#define AUTOPROPERTY_STARTPOS     "swishstartpos"
#define AUTOPROP_ID__STARTPOS     8
#define AUTOPROPERTY_INDEXFILE    "swishdbfile"
#define AUTOPROP_ID__INDEXFILE    9




