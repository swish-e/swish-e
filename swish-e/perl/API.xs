#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

/* $Id$ */

#include <swish-e.h>

#ifndef newSVuv
#   define newSVuv(i) newSViv(i)
#endif



MODULE = SWISH::API        PACKAGE = SWISH::API    PREFIX = Swish



# Make sure that we have at least xsubpp version 1.922.
REQUIRE: 1.922

SW_HANDLE
new(CLASS, index_file_list )
    char *CLASS
    char *index_file_list
    CODE:
        SwishErrorsToStderr();
        RETVAL = SwishInit( index_file_list );
    OUTPUT:
        RETVAL


void
DESTROY(self)
    SW_HANDLE self
    CODE:
        SwishClose( self );



void
SwishIndexNames(self)
    SW_HANDLE self

    PREINIT:
        const char **index_name;

    PPCODE:
        index_name = SwishIndexNames( self );
        while ( *index_name )
        {
            XPUSHs(sv_2mortal(newSVpv( *index_name ,0 )));
            index_name++;
        }
        


void
SwishHeaderNames(self)
    SW_HANDLE self

    PREINIT:
        const char **name;

    PPCODE:
        name = SwishHeaderNames( self );
        while ( *name )
        {
            XPUSHs(sv_2mortal(newSVpv( *name ,0 )));
            name++;
        }



void
SwishHeaderValue(swish_handle, index_file, header_name)
    SW_HANDLE swish_handle
    char * index_file
    char * header_name

    PREINIT:
        SWISH_HEADER_TYPE  header_type;
        SWISH_HEADER_VALUE head_value;
        int i;

    PPCODE:
        head_value = SwishHeaderValue( swish_handle, index_file, header_name, &header_type );

        PUSHMARK(SP);
        XPUSHs((SV *)swish_handle);
        XPUSHs((SV *)&head_value);
        XPUSHs((SV *)&header_type);
        PUTBACK;
        i = call_pv( "SWISH::API::decode_header_value", G_ARRAY );
        SPAGAIN;
#        PUTBACK;


void decode_header_value( swish_handle, header_value, header_type )
        SV *swish_handle
        SV *header_value
        SV *header_type


    PREINIT:        
        const char **string_list;
        SWISH_HEADER_VALUE *head_value;

    PPCODE:
        head_value = (SWISH_HEADER_VALUE *)header_value;

        switch ( *(SWISH_HEADER_TYPE *)header_type )
        {
            case SWISH_STRING:
                if ( head_value->string &&  head_value->string[0] )
                    XPUSHs(sv_2mortal(newSVpv( head_value->string,0 )));
                else
                    ST(0) = &PL_sv_undef;
                break;

            case SWISH_NUMBER:
                XPUSHs(sv_2mortal(newSVuv( head_value->number )));
                break;

            case SWISH_BOOL:
                // how about pushing &PL_sv_yes and &PL_sv_no or using boolSV()?
                XPUSHs(sv_2mortal(newSViv( head_value->boolean ? 1 : 0 )));
                break;

            case SWISH_LIST:
                string_list = head_value->string_list;

                if ( !string_list ) /* Don't think this can happen */
                    XSRETURN_EMPTY;

            
                while ( *string_list )
                {
                    XPUSHs(sv_2mortal(newSVpv( *string_list ,0 )));
                    string_list++;
                }
                break;

            case SWISH_HEADER_ERROR:
                SwishAbortLastError( (SW_HANDLE)swish_handle );
                break;

            default:
                croak(" Unknown header type '%d'\n", header_type );
        }



# Error Management

void
SwishAbortLastError(self)
    SW_HANDLE self


int
SwishError(self)
    SW_HANDLE self



char *
SwishErrorString(self)
    SW_HANDLE self



char *
SwishLastErrorMsg(self)
    SW_HANDLE self

int
SwishCriticalError(self)
    SW_HANDLE self


# Return a search object

SW_SEARCH
New_Search_Object(swish_handle, query = NULL)
    SW_HANDLE swish_handle
    char *query

    PREINIT:
        char * CLASS = "SWISH::API::Search";
        


# Returns a results object

SW_RESULTS
SwishQuery( swish_handle, query = NULL )
    SW_HANDLE swish_handle
    char *query

    PREINIT:
        char * CLASS = "SWISH::API::Results";



# Misc utility routines

void
SwishWordsByLetter(handle, filename, c)
     SW_HANDLE handle
     char *filename
     char c
     
     PREINIT:
         char *Words,*tmp;
         int c2;

     PPCODE:

         if(c=='*')
         {
            for(c2=1;c2<256;c2++)
            {
               Words=(char *)SwishWordsByLetter(handle,filename,(unsigned char)c2);
               for(tmp=Words;tmp && tmp[0];tmp+=strlen(tmp)+1)
               {
                  XPUSHs(sv_2mortal(newSVpv(tmp,0)));
               }
            }
         } else {
            Words=(char *)SwishWordsByLetter(handle,filename,c);
            for(tmp=Words;tmp && tmp[0];tmp+=strlen(tmp)+1)
            {
               XPUSHs(sv_2mortal(newSVpv(tmp,0)));
            }
         }



char *
SwishStemWord(handle, word)
     SW_HANDLE handle
     char *word
            


        
# **************************************************************
# 
#             SWISH::API::Search
# 
# ***************************************************************


MODULE = SWISH::API        PACKAGE = SWISH::API::Search    PREFIX = Swish


void
DESTROY(search)
    SW_SEARCH search
    CODE:
        Free_Search_Object( search );


void
SwishSetQuery(search,query)
    SW_SEARCH search
    char * query
    

void
SwishSetStructure(search, structure)
    SW_SEARCH search
    int structure


void
SwishPhraseDelimiter(search, delimiter)
    SW_SEARCH search
    char * delimiter

    CODE:
        SwishPhraseDelimiter(search, delimiter[0] );
    

void
SwishSetSearchLimit(search, property, low, high)
    SW_SEARCH search
    char * property
    char * low
    char * high
    

void
SwishResetSearchLimit(search)
    SW_SEARCH search


void
SwishSetSort(search, sort_string)
    SW_SEARCH search
    char *sort_string


# Returns a result object 

SW_RESULTS
SwishExecute( search, query = NULL )
    SW_SEARCH search
    char *query

    PREINIT:
        char * CLASS = "SWISH::API::Results";



# **************************************************************
#
#            SWISH::API::Results
#
# ***************************************************************/


MODULE = SWISH::API        PACKAGE = SWISH::API::Results    PREFIX = Swish



void
DESTROY(results)
    SW_RESULTS results
    CODE:
        Free_Results_Object( results );

        

int
SwishHits(self)
    SW_RESULTS self




int
SwishSeekResult(self, position)
    SW_RESULTS self
    int position



SW_RESULT
SwishNextResult(results)
    SW_RESULTS results

    PREINIT:
        char * CLASS = "SWISH::API::Result";



void
SwishRemovedStopwords(results, index_name)
    SW_RESULTS results
    char * index_name


    PREINIT:
        SW_HANDLE swish_handle;
        SWISH_HEADER_TYPE  header_type;
        SWISH_HEADER_VALUE head_value;
        int i;

    PPCODE:
        swish_handle = SW_ResultsToSW_HANDLE( results );
        header_type = SWISH_LIST;
        head_value = SwishRemovedStopwords( results, index_name );

        PUSHMARK(SP);
        XPUSHs((SV *)swish_handle);
        XPUSHs((SV *)&head_value);
        XPUSHs((SV *)&header_type);
        PUTBACK;
        i = call_pv( "SWISH::API::decode_header_value", G_ARRAY );
        SPAGAIN;
#        PUTBACK;


void
SwishParsedWords(results, index_name)
    SW_RESULTS results
    char * index_name


    PREINIT:
        SW_HANDLE swish_handle;
        SWISH_HEADER_TYPE  header_type;
        SWISH_HEADER_VALUE head_value;
        int i;

    PPCODE:
        swish_handle = SW_ResultsToSW_HANDLE( results );
        header_type = SWISH_LIST;
        head_value = SwishParsedWords( results, index_name );

        PUSHMARK(SP);
        XPUSHs((SV *)swish_handle);
        XPUSHs((SV *)&head_value);
        XPUSHs((SV *)&header_type);
        PUTBACK;
        i = call_pv( "SWISH::API::decode_header_value", G_ARRAY );
        SPAGAIN;
#        PUTBACK;



        
    

# **************************************************************
#
#            SWISH::API::Result (single result)
#
#   No real object is created in C, so there's no DESTROY
#
# ***************************************************************/


MODULE = SWISH::API        PACKAGE = SWISH::API::Result    PREFIX = Swish


void
SwishProperty(result, property)
    SW_RESULT result
    char *property

    PREINIT:
         PropValue *pv;


    PPCODE:
        pv = getResultPropValue( result, property, 0 );
        if ( !pv )
            XSRETURN_UNDEF;


        switch (pv->datatype)
        {
            case PROP_INTEGER:
                PUSHs(sv_2mortal(newSViv(pv->value.v_int)));
                break;

            case PROP_ULONG:
                PUSHs(sv_2mortal(newSViv(pv->value.v_ulong)));
                break;

            case PROP_STRING:
                PUSHs(sv_2mortal(newSVpv(pv->value.v_str,0)));
                break;

            case PROP_DATE:
                PUSHs(sv_2mortal(newSViv(pv->value.v_date)));
                break;

            default:
                croak("Unknown property data type '%d' for property '%s'\n", pv->datatype, property);
        }

        freeResultPropValue(pv);


char *
SwishResultPropertyStr( result, pname)
    SW_RESULT result
    char * pname
        


void
SwishResultIndexValue(self, header_name)
    SW_RESULT self
    char * header_name


    PREINIT:
        SW_HANDLE swish_handle;
        SWISH_HEADER_TYPE  header_type;
        SWISH_HEADER_VALUE head_value;
        int i;

    PPCODE:
        swish_handle = SW_ResultToSW_HANDLE( self );
        head_value = SwishResultIndexValue( self, header_name, &header_type );

        PUSHMARK(SP);
        XPUSHs((SV *)swish_handle);
        XPUSHs((SV *)&head_value);
        XPUSHs((SV *)&header_type);
        PUTBACK;
        i = call_pv( "SWISH::API::decode_header_value", G_ARRAY );
        SPAGAIN;
#        PUTBACK;




