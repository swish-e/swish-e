/* 
fs.h
*/
#ifndef __HasSeenModule_FS
#define __HasSeenModule_FS       1

/*
   -- module data
*/

struct MOD_FS
{
    struct swline *pathconlist;
    struct swline *dirconlist;
    struct swline *fileconlist;
    struct swline *titconlist;
    struct swline *fileislist;

    int     followsymlinks;

};


void initModule_FS (SWISH *);
void freeModule_FS (SWISH *);
int  configModule_FS (SWISH *, StringList *);

void indexadir(SWISH *, char *);
void indexafile(SWISH *, char *);
void printfile(SWISH *, char *);
void printfiles(SWISH *, DOCENTRYARRAY *);
void printdirs(SWISH *, DOCENTRYARRAY *);

#endif
