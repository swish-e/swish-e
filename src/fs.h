/* 
fs.h
*/
#ifndef __HasSeenModule_FS
#define __HasSeenModule_FS       1

/*
   -- module data
*/

typedef struct
{
    regex_list  *pathname;
    regex_list  *dirname;
    regex_list  *filename;
    regex_list  *dircontains;
    regex_list  *title;
    
}
PATH_LIST;

struct MOD_FS
{
    PATH_LIST   filerules;
    PATH_LIST   filematch;
    int         followsymlinks;

};


void initModule_FS (SWISH *);
void freeModule_FS (SWISH *);
int  configModule_FS (SWISH *, StringList *);



#endif
