/* Jose Ruiz 2000/01 Definitions for MetaNames/Fields */

#define META_INDEX    1      /* bynary 00000001 */  /* Meta is indexed */
#define META_PROP    2      /* bynary 00000010 */ /* Also stored as property */
#define META_NUMBER    4      /* bynary 00000100 */ /* Data is binary number */
#define META_DATE    8      /* bynary 00001000 */ /* Data is binary date */
#define META_INTERNAL 16    /* flag saying this is an internal metaname */


/* Macros to test the type of a MetaName */
#define is_meta_internal(x)  ((x)->metaType & META_INTERNAL)
#define is_meta_index(x)  ((x)->metaType & META_INDEX)
#define is_meta_property(x)  ((x)->metaType & META_PROP)
#define is_meta_number(x)  ((x)->metaType & META_NUMBER)
#define is_meta_date(x)  ((x)->metaType & META_DATE)
#define is_meta_string(x)  (((x)->metaType | (META_INDEX | META_PROP | META_INTERNAL)) == (META_INDEX | META_PROP | META_INTERNAL))

/* Macro to check if both metaNames are of the same type (number, date, ...) */
#define match_meta_type(x,y)  (((x)>>META_PROP)==((y)>>META_PROP))


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
int isDontBumpMetaName(SWISH *sw,char *tag);
int is_meta_entry( struct metaEntry *meta_entry, char *name );

