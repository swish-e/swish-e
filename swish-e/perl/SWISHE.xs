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
#include "../src/stemmer.h"
#include "../src/merge.h"
#include "../src/docprop.h"
#include "../src/search.h"


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
     int    *metaIDs;
     PropValue *pv;
     SWISH *sw;


     PPCODE:
     result = (RESULT *)SwishNext(handle);
     if(result)
     {
        sw = (SWISH *) result->sw;

        numPropertiesToDisplay = sw->Search->numPropertiesToDisplay
           ? sw->Search->numPropertiesToDisplay
           : 0;


        PUSHMARK(SP);

        if ( (pv = getResultPropValue( sw, result, AUTOPROPERTY_RESULT_RANK, 0)) )
            XPUSHs(sv_2mortal(newSViv(pv->value.v_int)));
        else
            XPUSHs(&PL_sv_undef);


        /*
        if ( (pv = getResultPropValue( sw, result, NULL, AUTOPROP_ID__INDEXFILE)) )
            XPUSHs(sv_2mortal(newSVpv(pv->value.v_str,0)));
        else
            XPUSHs(&PL_sv_undef);
        */

        if ( (pv = getResultPropValue( sw, result, AUTOPROPERTY_DOCPATH, 0)) )
            XPUSHs(sv_2mortal(newSVpv(pv->value.v_str,0)));
        else
            XPUSHs(&PL_sv_undef);

        /*
        if ( (pv = getResultPropValue( sw, result, NULL, AUTOPROP_ID__LASTMODIFIED)) )
            XPUSHs(sv_2mortal(newSViv(pv->value.v_int)));
        else
            XPUSHs(&PL_sv_undef);
        */

        if ( (pv = getResultPropValue( sw, result, AUTOPROPERTY_TITLE, 0)) )
            XPUSHs(sv_2mortal(newSVpv(pv->value.v_str,0)));
        else
            XPUSHs(&PL_sv_undef);

        /*
        if ( (pv = getResultPropValue( sw, result, NULL, AUTOPROP_ID__SUMMARY)) )
            XPUSHs(sv_2mortal(newSVpv(pv->value.v_str,0)));
        else
            XPUSHs(&PL_sv_undef);

        if ( (pv = getResultPropValue( sw, result, NULL, AUTOPROP_ID__STARTPOS)) )
            XPUSHs(sv_2mortal(newSViv(pv->value.v_int)));
        else
            XPUSHs(&PL_sv_undef);

        */
        if ( (pv = getResultPropValue( sw, result, AUTOPROPERTY_DOCSIZE, 0)) )
            XPUSHs(sv_2mortal(newSViv(pv->value.v_int)));
        else
            XPUSHs(&PL_sv_undef);

        metaIDs = result->indexf->propIDToDisplay;

        for( i=0; i < numPropertiesToDisplay; i++ )
        {
            if ( !(pv = getResultPropValue( sw, result, NULL, metaIDs[i])) )
            {
                XPUSHs(&PL_sv_undef);
                continue;
            }

            switch (pv->datatype)
            {
                case INTEGER:
                    XPUSHs(sv_2mortal(newSViv(pv->value.v_int)));
                    break;

                case ULONG:
                    XPUSHs(sv_2mortal(newSViv(pv->value.v_ulong)));
                    break;

                case STRING:
                    XPUSHs(sv_2mortal(newSVpv(pv->value.v_str,0)));
                    /* Free the string, if neede */
                    if ( pv->destroy )
                        efree( pv->value.v_str );
                    break;

                /* Let perl format the data, if needed */
                case DATE:
                    XPUSHs(sv_2mortal(newSViv(pv->value.v_date)));
                    break;

                default:
                    XPUSHs(&PL_sv_undef);
                    break;
            }
            efree(pv);
        }
        PUTBACK;
    }


void
SetLimitParameter(handle,propertyname,low,hi)
     void *handle;
     char *propertyname;
     char *low;
     char *hi;
     CODE:
     SetLimitParameter((SWISH *)handle,propertyname,low,hi);


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
     int c2;
     PPCODE:
     PUSHMARK(SP);
     if(c=='*')
     {
        for(c2=1;c2<256;c2++)
        {
           Words=(char *)SwishWords(handle,filename,(unsigned char)c2);
           for(tmp=Words;tmp && tmp[0];tmp+=strlen(tmp)+1)
           {
              XPUSHs(sv_2mortal(newSVpv(tmp,0)));
           }
        }
     } else {
        Words=(char *)SwishWords(handle,filename,c);
        for(tmp=Words;tmp && tmp[0];tmp+=strlen(tmp)+1)
        {
           XPUSHs(sv_2mortal(newSVpv(tmp,0)));
        }
     }
     PUTBACK;


SV *
SwishStem(word)
        char *word
    PREINIT:
        char *newword;
        int   length;
    CODE:
        /* default to undefined */
        ST(0) = sv_newmortal();
        
        length = strlen( word ) + 100;
        newword = (char *)emalloc( length+1 ); /* leak! */
        strcpy( newword, word );

        /* set return value only if stem returns OK */
        if ( Stem(&newword, &length) == STEM_OK )
            sv_setpv( ST(0), newword );
        efree( newword );
            

char *
SwishErrorString(number)
     int number
     PREINIT:
     char *value;
     PPCODE:
     PUSHMARK(SP);
     value = (char *)SwishErrorString(number);
     XPUSHs(sv_2mortal(newSVpv(value,0)));
     PUTBACK;

SV *
SwishHeaders(handle)
     void *handle;
     CODE:
     AV *headers;
     HV *indexheader;
     SWISH *sw = (SWISH *)handle;
     IndexFILE *indexf;

     headers = newAV();
     indexf = sw->indexlist;
     while(indexf)
     {
        indexheader = newHV();
        hv_store(indexheader,"IndexName",9,newSVpv(indexf->line,0),0);
        hv_store(indexheader,"WordCharacters",14,newSVpv(indexf->header.wordchars,0),0);
        hv_store(indexheader,"BeginCharacters",15,newSVpv(indexf->header.beginchars,0),0);
        hv_store(indexheader,"EndCharacters",13,newSVpv(indexf->header.endchars,0),0);
        hv_store(indexheader,"IgnoreLastChar",14,newSVpv(indexf->header.ignorelastchar,0),0);
        hv_store(indexheader,"IgnoreFirstChar",15,newSVpv(indexf->header.ignorefirstchar,0),0);
        hv_store(indexheader,"IndexName",9,newSVpv(indexf->header.indexn,0),0);
        hv_store(indexheader,"IndexDescription",16,newSVpv(indexf->header.indexd,0),0);
        hv_store(indexheader,"IndexPointer",12,newSVpv(indexf->header.indexp,0),0);
        hv_store(indexheader,"IndexAdmin",10,newSVpv(indexf->header.indexa,0),0);
        hv_store(indexheader,"UseStemming",11,newSViv(indexf->header.applyStemmingRules),0);
        hv_store(indexheader,"UseSoundex",10,newSViv(indexf->header.applySoundexRules),0);
        hv_store(indexheader,"TotalWords",10,newSViv(indexf->header.totalwords),0);
        hv_store(indexheader,"TotalFiles",10,newSViv(indexf->header.totalfiles),0);
        av_push(headers,newRV_noinc((SV *)indexheader));
        indexf = indexf->next;
     }
     RETVAL = newRV_noinc((SV *) headers);
     OUTPUT:
     RETVAL

