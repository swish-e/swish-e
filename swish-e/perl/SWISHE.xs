#ifdef __cplusplus
extern "C" {
#endif
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#ifdef __cplusplus
}
#endif

#include "../src/swish.h"

MODULE = SWISHE  PACKAGE = SWISHE
PROTOTYPES: DISABLE

void *
SwishOpen(IndexFiles)
     char *IndexFiles
     CODE:
     RETVAL = (void *)SwishOpen(IndexFiles);
     OUTPUT:
     RETVAL

void
SwishClose(handle)
     void *handle
     CODE:
     SwishClose(handle);

int
SwishSearch(handle,words,structure,properties,sortspec)
     void *handle
     char *words
     int structure
     char *properties
     char *sortspec
     CODE:
     RETVAL = SwishSearch(handle,words,structure,properties,sortspec);
     OUTPUT:
     RETVAL

void
SwishNext(handle)
     void *handle
     PREINIT:
     RESULT *result;
     int i;
     int numPropertiesToDisplay;
     PPCODE:
     result = (RESULT *)SwishNext(handle);
     if(result) {
	numPropertiesToDisplay=getnumPropertiesToDisplay(handle);
        PUSHMARK(SP);
        XPUSHs(sv_2mortal(newSViv(result->rank)));
        XPUSHs(sv_2mortal(newSVpv(result->indexf->line,0)));
        XPUSHs(sv_2mortal(newSVpv(result->filename,0)));
        XPUSHs(sv_2mortal(newSVpv(result->title,0)));
        XPUSHs(sv_2mortal(newSViv(result->start)));
        XPUSHs(sv_2mortal(newSViv(result->size)));
        for(i=0;i<numPropertiesToDisplay;i++)
             XPUSHs(sv_2mortal(newSVpv(result->Prop[i],0)));
        PUTBACK;
     }

int 
SwishSeek(handle,number)
     void *handle
     int number
     CODE:
     RETVAL = SwishSeek(handle,number);
     OUTPUT:
     RETVAL

int
SwishError(handle)
     void *handle
     CODE:
     RETVAL = SwishError(handle);
     OUTPUT:
     RETVAL

char *
SwishHeaderParameter(handle,parameter_name)
     void *handle
     char *parameter_name
     PREINIT:
     IndexFILE *indexlist;
     char *value;
     PPCODE:
     indexlist=((SWISH *)handle)->indexlist;
     PUSHMARK(SP);
     while(indexlist)
     {
        value = (char *)SwishHeaderParameter(indexlist,parameter_name);
        XPUSHs(sv_2mortal(newSVpv(value,0)));
        indexlist=indexlist->next;
     }
     PUTBACK;


void
SwishStopWords(handle, filename)
     void *handle
     char *filename
     PREINIT:
     int NumStopWords;
     int i;
     char **StopWords;
     PPCODE:
     StopWords=(char **)SwishStopWords(handle,filename,&NumStopWords);
     PUSHMARK(SP);
     for(i=0;i<NumStopWords;i++)
     {
        XPUSHs(sv_2mortal(newSVpv(StopWords[i],0)));
     }
     PUTBACK;

void
SwishWords(handle, filename, c)
     void *handle
     char *filename
     char c
     PREINIT:
     char *Words,*tmp;
     PPCODE:
     Words=(char *)SwishWords(handle,filename,c);
     PUSHMARK(SP);
     for(tmp=Words;tmp && tmp[0];tmp+=strlen(tmp)+1)
     {
        XPUSHs(sv_2mortal(newSVpv(tmp,0)));
     }
     PUTBACK;


char *
SwishStem(word)
     char *word
     CODE:
     char *newword;
     newword=(char *)estrdup(word);
     Stem_it(&newword);
     RETVAL = newword;
     OUTPUT:
     RETVAL
