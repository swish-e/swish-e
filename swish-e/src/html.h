
/* Just the prototypes */

char *parsetitle(char *, char *);
int isoktitle(SWISH *,char *);
unsigned char *convertentities(unsigned char *, SWISH *);
char *getent(char *, int *);
char *converttonamed(char *);
char *converttoascii(char *);
int countwords_HTML(SWISH *, FileProp *, char *buffer);
int getstructure(char*, int);
struct metaEntry *getHTMLMeta (IndexFILE *, char *, int *, int, int, char *);
int parseMetaData (SWISH *, IndexFILE *, char *, int, int, char *, char *, struct file*);
char *parseHtmlSummary (char *,char *,int, SWISH *);
struct EntitiesHashTable *buildEntitiesHashTable(void);
