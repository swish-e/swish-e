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

#ifdef __cplusplus
extern "C" {
#endif

void freeProperty( propEntry *prop );
void freeDocProperties (docProperties *);
void freefileinfo(FileRec *);

unsigned char *storeDocProperties (docProperties *, int *);

propEntry *CreateProperty(struct metaEntry *meta_entry, unsigned char *propValue, int propLen, int preEncoded, int *error_flag );
void addDocProperties( INDEXDATAHEADER *header, docProperties **docProperties, unsigned char *propValue, int propLen, char *filename );
int addDocProperty (docProperties **, struct metaEntry * , unsigned char* ,int, int );
int Compare_Properties( struct metaEntry *meta_entry, propEntry *p1, propEntry *p2 );

unsigned char *fetchDocProperties ( FileRec *, char * );

void addSearchResultDisplayProperty (SWISH *, char* );
void addSearchResultSortProperty (SWISH *, char*, int );
void printStandardResultProperties(FILE *, RESULT *);

void swapDocPropertyMetaNames (docProperties **, struct metaMergeEntry *);

char *getResultPropAsString(RESULT *, int);
char *DecodeDocProperty( struct metaEntry *meta_entry, propEntry *prop );
void getSwishInternalProperties(FileRec *, IndexFILE *);


PropValue *getResultPropValue (RESULT *r, char *name, int ID);
void    freeResultPropValue(PropValue *pv);



void     WritePropertiesToDisk( SWISH *sw , FileRec *fi);
propEntry *ReadSingleDocPropertiesFromDisk( IndexFILE *indexf, FileRec *fi, int metaID, int max_size );
docProperties *ReadAllDocPropertiesFromDisk( IndexFILE *indexf, int filenum );



/*
   -- Mapping AutoProperties <-> METANAMES  
   -- should be the same
*/

/* all AutoPropteries start with this string ! */


#define AUTOPROPERTY_DEFAULT      "swishdefault"
#define AUTOPROPERTY_REC_COUNT    "swishreccount"
#define AUTOPROPERTY_RESULT_RANK  "swishrank"
#define AUTOPROPERTY_FILENUM      "swishfilenum"
#define AUTOPROPERTY_INDEXFILE    "swishdbfile"

#define AUTOPROPERTY_DOCPATH      "swishdocpath"
#define AUTOPROPERTY_TITLE        "swishtitle"
#define AUTOPROPERTY_DOCSIZE      "swishdocsize"
#define AUTOPROPERTY_LASTMODIFIED "swishlastmodified"
#define AUTOPROPERTY_SUMMARY      "swishdescription"
#define AUTOPROPERTY_STARTPOS     "swishstartpos"

#ifdef __cplusplus
}
#endif /* __cplusplus */

