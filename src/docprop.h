/*
 * DocProperties.c, DocProperties.h
 *
 * Functions to manage the index's Document Properties 
 *
 * File Created.
 * M. Gaulin 8/10/98
 */

#ifndef _DOCPROP_H_
#define _DOCPROP_H_

/*
** use _AP() for easier cross-compiler (non-ANSI) porting 
** <return value> <functionname> _AP( (<arg prototypes>) );
*/

void freeDocProperties _AP ((struct docPropertyEntry **docProperties));
char *storeDocProperties _AP ((struct docPropertyEntry *, int *));

void addDocProperty _AP ((struct docPropertyEntry **, int , char* ));

struct docPropertyEntry *fetchDocProperties _AP ((char * ));

int initSearchResultProperties _AP ((SWISH *));
void addSearchResultDisplayProperty _AP ((SWISH *, char* ));
void addSearchResultSortProperty _AP ((SWISH *, char*, int ));
char* lookupDocPropertyValue _AP ((int , char *));
void printSearchResultProperties _AP ((SWISH *, char **));

void swapDocPropertyMetaNames _AP ((struct docPropertyEntry *, struct metaMergeEntry *));

char **getResultProperties _AP ((SWISH *, struct docPropertyEntry *));
char **getResultSortProperties _AP ((SWISH *, struct docPropertyEntry *));
int isSortProp _AP ((SWISH *));
RESULT *sortresultsbyproperty _AP ((SWISH *, int ));

int initSortResultProperties _AP ((SWISH *));

struct docPropertyEntry *DupProps _AP ((struct docPropertyEntry *));
#endif 
