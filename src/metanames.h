/* Jose Ruiz 2000/01 Definitions for MetaNames/Fields */

/* META_INDEX and META_PROP could now share the same bit, since props and metas are separated entries */
#define META_INDEX    (1<<0)      /* bynary 00000001 */  /* Meta is indexed */
#define META_PROP     (1<<1)      /* bynary 00000010 */ /* Also stored as property */
#define META_STRING   (1<<2)      /* String type of property */
#define META_NUMBER   (1<<3)      /* Data is binary number */
#define META_DATE     (1<<4)      /* Data is binary date */
#define META_INTERNAL (1<<5)      /* flag saying this is an internal metaname */
#define META_IGNORE_CASE (1<<6)  /* flag to say ignore case when comparing/sorting */


/* Macros to test the type of a MetaName */
#define is_meta_internal(x)     ((x)->metaType & META_INTERNAL)
#define is_meta_index(x)        ((x)->metaType & META_INDEX)
#define is_meta_property(x)     ((x)->metaType & META_PROP)
#define is_meta_number(x)       ((x)->metaType & META_NUMBER)
#define is_meta_date(x)         ((x)->metaType & META_DATE)
#define is_meta_string(x)       ((x)->metaType & META_STRING)
#define is_meta_ignore_case(x)  ((x)->metaType & META_IGNORE_CASE)



void add_default_metanames(IndexFILE *);

struct metaEntry * getMetaNameByNameNoAlias(INDEXDATAHEADER * header, char *word);
struct metaEntry * getMetaNameByName(INDEXDATAHEADER *, char *);
struct metaEntry * getMetaNameByID(INDEXDATAHEADER *, int);

struct metaEntry * getPropNameByNameNoAlias(INDEXDATAHEADER * header, char *word);
struct metaEntry * getPropNameByName(INDEXDATAHEADER *, char *);
struct metaEntry * getPropNameByID(INDEXDATAHEADER *, int);


struct metaEntry * addMetaEntry(INDEXDATAHEADER *header, char *metaname, int metaType, int metaID);
struct metaEntry * addNewMetaEntry(INDEXDATAHEADER *header, char *metaWord, int metaType, int metaID);

void freeMetaEntries( INDEXDATAHEADER * );
int isDontBumpMetaName(struct swline *,char *tag);
int is_meta_entry( struct metaEntry *meta_entry, char *name );
void ClearInMetaFlags(INDEXDATAHEADER * header);

void init_property_list(INDEXDATAHEADER *header);
