/*
$Id$

*/

/*
   seems to be a very old module of swish
   some serious work to do in html.c and html.h!! 
   but anyway it seems to work...
 */


/* Just the prototypes */

char *parsetitle(char *, char *);
int isoktitle(SWISH *,char *);
int countwords_HTML(SWISH *, FileProp *, char *buffer);
int getstructure(char*, int);
struct metaEntry *getHTMLMeta (IndexFILE *, char *, int *, int, int, char *);
int parseMetaData (SWISH *, IndexFILE *, char *, int, int, char *, char *, struct file*, int *);
char *parseHtmlSummary (char *,char *,int, SWISH *);
int parsecomment (SWISH *, char *, int, int, int, int *);