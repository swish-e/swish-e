
/* Just the prototypes */

char *parsetitle(char *, char *);
int isoktitle(SWISH *,char *);
char *convertentities(char *);
char *getent(char *, int *);
char *converttonamed(char *);
char *converttoascii(char *);
int countwords_HTML(SWISH *, FileProp *, char *buffer);
int getstructure(char*, int);
int getMeta (IndexFILE *, char *tag, int* docPropName, int *, int);
int parseMetaData (SWISH *, IndexFILE *, char *, int, int, struct file*);
char *parseHtmlSummary (char *,char *,int);
