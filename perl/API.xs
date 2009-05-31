#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

/* $Id$ */

#include <swish-e.h>

#ifndef newSVuv
#   define newSVuv(i) newSViv(i)
#endif

#ifndef call_pv
#   define call_pv(i,j) perl_call_pv(i,j)
#endif

/* 
 * Create a typedef for managing the metanames objects. This allows storing
 * both the SV and the pointer to the parent object so its refcount can be
 * adjusted on DESTROY.  The other way is to provide a way to get to the
 * parent's SV in the swish-e library.  That means modifying the swish-e
 * library.  This is already done for most other objects -- the SV of the perl
 * swish handle is stored in the C SW_HANDLE struct -- see SwishSetRefPtr and
 * Swish*Parent functions.  It's actually much easier to provide a way to get
 * the SV via the C library since don't need to malloc and free an extra
 * structre.  But, done for the meta descriptions as an exercise.
 */

typedef struct {
    SV          *handle_sv;     /* Parent SV for DESTROY */
    SW_META     meta;           /* meta description C pointer */
} META_OBJ;






MODULE = SWISH::API        PACKAGE = SWISH::API    PREFIX = Swish



# Make sure that we have at least xsubpp version 1.922.
REQUIRE: 1.922

# This returns SW_HANDLE
void 
new(CLASS, index_file_list )
    char *CLASS
    char *index_file_list

    PREINIT:
        SW_HANDLE handle;

    PPCODE:
        SwishErrorsToStderr();
        handle = SwishInit( index_file_list );
        ST(0) = sv_newmortal();
        sv_setref_pv( ST(0), CLASS, (void *)handle );
        SwishSetRefPtr( handle, (void *)SvRV(ST(0)) );
        XSRETURN(1);



void
DESTROY(self)
        SW_HANDLE self;

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
            XPUSHs(sv_2mortal(newSVpv( (char *)*index_name ,0 )));
            index_name++;
        }

############################################
# set RankScheme
# karman - Wed Sep  1 09:22:50 CDT 2004

void
SwishRankScheme(self, scheme)
    SW_HANDLE self
    int scheme

#############################################
# set ReturnRawRank
# karman - 30 Mar 2009
void 
SwishReturnRawRank(self, flag)
    SW_HANDLE self
    int flag


#############################################


# added SwishFuzzy to give access directly from SW object
# karman - Wed Oct 27 11:16:45 CDT 2004
# This returns a fuzzy word object based on the result
# Thu Jul 14 11:33:27 CDT 2005
# fixed namespace issue: now SwishFuzzify (called like $swish->Fuzzify)

SW_FUZZYWORD
SwishFuzzify(swobj, index_name, word)
    SW_HANDLE swobj
    char * index_name
    char * word

    PREINIT:
        char * CLASS = "SWISH::API::FuzzyWord";

    CODE:
        RETVAL = SwishFuzzify(swobj, index_name, word);
		    
	    
    OUTPUT:
        RETVAL


void
SwishHeaderNames(self)
    SW_HANDLE self

    PREINIT:
        const char **name;

    PPCODE:
        name = SwishHeaderNames( self );
        while ( *name )
        {
            XPUSHs(sv_2mortal(newSVpv( (char *)*name ,0 )));
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


void
decode_header_value( swish_handle, header_value, header_type )
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
                    XPUSHs(sv_2mortal(newSVpv( (char *)head_value->string,0 )));
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
                    XPUSHs(sv_2mortal(newSVpv( (char *)*string_list ,0 )));
                    string_list++;
                }
                break;

            case SWISH_HEADER_ERROR:
                SwishAbortLastError( (SW_HANDLE)swish_handle );
                break;

            default:
                croak(" Unknown header type '%d'\n", (int)header_type ); 
                /* constants used in cases above are enum-ed in headers.h */

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


# Return a search object  (uses a typemap to bless the return object)

SW_SEARCH
New_Search_Object(swish_handle, query = NULL)
    SW_HANDLE swish_handle
    char *query

    PREINIT:
        char * CLASS = "SWISH::API::Search";

    CODE:
        RETVAL = New_Search_Object( swish_handle, query );
        if ( RETVAL )
            SvREFCNT_inc( (SV *)SwishSearch_parent( RETVAL ) );

    OUTPUT:
        RETVAL

# Returns a SW_RESULTS object

void
SwishQuery( swish_handle, query = NULL )
    SW_HANDLE swish_handle
    char *query

    PREINIT:
        char * CLASS = "SWISH::API::Results";
        SW_RESULTS results;

    PPCODE:
        results = SwishQuery( swish_handle, query );
        if ( results )
        {
            SvREFCNT_inc( (SV *)SwishResults_parent( results ) );
            ST(0) = sv_newmortal();
            sv_setref_pv( ST(0), CLASS, (void *)results );
            ResultsSetRefPtr( results, (void *)SvRV(ST(0)) );
            XSRETURN(1);
        }


# Methods to return info about MetaNames and Properties
# The C API provided by Jamie Herre in March 2004

# Returns an array of SWISH::API::MetaName objects

void
SwishMetaList( swish_handle, index_name )
    SW_HANDLE swish_handle
    char *index_name

    PREINIT:
        SWISH_META_LIST meta_list;

    PPCODE:
        /* Grab the list of pointers */
        meta_list = SwishMetaList( swish_handle, index_name );

        PUSHMARK(SP) ;      /* always need to PUSHMARK, even w/o params */
        XPUSHs( (SV *)swish_handle );
        XPUSHs( (SV *)meta_list );
        XPUSHs( (SV *)"SWISH::API::MetaName");
        PUTBACK ;           /* lets perl know how many parameters are here */

        call_pv("SWISH::API::push_meta_list", G_ARRAY );
        SPAGAIN;

# Returns an array of SWISH::API::MetaName objects

void
SwishPropertyList( swish_handle, index_name )
    SW_HANDLE swish_handle
    char *index_name

    PREINIT:
        SWISH_META_LIST meta_list;

    PPCODE:
        /* Grab the list of pointers */
        meta_list = SwishPropertyList( swish_handle, index_name );
        PUSHMARK(SP) ;
        XPUSHs( (SV *)swish_handle );
        XPUSHs( (SV *)meta_list );
        XPUSHs( (SV *)"SWISH::API::PropertyName");
        PUTBACK ;

        call_pv("SWISH::API::push_meta_list", G_ARRAY );
        SPAGAIN;





void
push_meta_list( s_handle, m_list, m_class )
    SV *s_handle
    SV *m_list
    SV *m_class

    PREINIT:
        SW_HANDLE swish_handle;
        SWISH_META_LIST meta_list;
        char *class;

    PPCODE:
        class = (char *)m_class;
        swish_handle = (SW_HANDLE)s_handle;
        meta_list = (SWISH_META_LIST)m_list;

        /* Check for an error -- typically this would be an invalid index name */
        /* Fix: calling with an invalid swish_handle will call progerr */
        if ( SwishError( swish_handle ) )
            croak("%s %s", SwishErrorString( swish_handle ), SwishLastErrorMsg( swish_handle ) );

        /* Make sure a list is returned and it's not empty */
        if ( !meta_list || !*meta_list )
            XSRETURN_EMPTY;


        while ( *meta_list )
        {
            SV *o;

            /* Create a new structure for storing the meta description and the parent SV */
            META_OBJ *object = (META_OBJ *)safemalloc(sizeof(META_OBJ));

            /* Store the meta entry */
            object->meta = *meta_list;

            /* Store the and bump the swish_handle SV */
            object->handle_sv = (SV *)SwishGetRefPtr( swish_handle );
            SvREFCNT_inc( object->handle_sv );

            /* And create the Perl object and assign the object to it */
            o = sv_newmortal();
            sv_setref_pv( o, class, (void *)object );

            /* and push onto list */
            XPUSHs( o );

            meta_list++;
        }





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
        if ( search )
        {
            SV *parent = (SV *)SwishSearch_parent( search );
            Free_Search_Object( search );
            SvREFCNT_dec( parent );
        }


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


# Returns a SW_RESULTS object

void
SwishExecute( search, query = NULL )
    SW_SEARCH search
    char *query

    PREINIT:
        char * CLASS = "SWISH::API::Results";
        SW_RESULTS results;

    PPCODE:
        results = SwishExecute( search, query );
        {
            SvREFCNT_inc( (SV *)SwishResults_parent( results ) );
            ST(0) = sv_newmortal();
            sv_setref_pv( ST(0), CLASS, (void *)results );
            ResultsSetRefPtr( results, (void *)SvRV(ST(0)) );
            XSRETURN(1);
        }



# **************************************************************
#
#            SWISH::API::Results
#
# ***************************************************************


MODULE = SWISH::API        PACKAGE = SWISH::API::Results    PREFIX = Swish



void
DESTROY(results)
    SW_RESULTS results

    CODE:
        if ( results )
        {
            SV *parent = (SV *)SwishResults_parent( results );
            Free_Results_Object( results );
            SvREFCNT_dec( parent );
        } 

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

    CODE:
        RETVAL = SwishNextResult(results);
        if ( RETVAL )
            SvREFCNT_inc( (SV *)SwishResult_parent( RETVAL ));
    OUTPUT:
        RETVAL


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
#
# ***************************************************************


MODULE = SWISH::API        PACKAGE = SWISH::API::Result    PREFIX = Swish

void
DESTROY(result)
    SW_RESULT result

    CODE:
        if ( result )
        {
            SV *parent = (SV *)SwishResult_parent(result);
            SvREFCNT_dec( parent );
        }

void
SwishProperty(result, property)
    SW_RESULT result
    char *property

    PREINIT:
         PropValue *pv;


    PPCODE:
        # This will abort swish-e if result is NULL
        pv = getResultPropValue( result, property, 0 );

        if ( !pv )
        {
            # this is always the case
            SW_HANDLE h = SW_ResultToSW_HANDLE( result );
            if ( SwishError( h ) )
                croak("%s %s", SwishErrorString( h ), SwishLastErrorMsg( h ) );

            XSRETURN_UNDEF;
        }


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

            case PROP_UNDEFINED:
                freeResultPropValue(pv);
                XSRETURN_UNDEF;
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

# This returns a fuzzy word object based on the result

SW_FUZZYWORD
SwishFuzzyWord(result, word)
    SW_RESULT result
    char *word

    PREINIT:
        char * CLASS = "SWISH::API::FuzzyWord";

    CODE:
        RETVAL = SwishFuzzyWord(result,word);

    OUTPUT:
        RETVAL



# This returns the name of the stemmer used for this index

const char*
SwishFuzzyMode(result)
    SW_RESULT result



MODULE = SWISH::API        PACKAGE = SWISH::API::Result    PREFIX = SwishResult

void
SwishResultMetaList(result)
    SW_RESULT result

    PREINIT:
        SWISH_META_LIST meta_list;
        SW_HANDLE swish_handle;

    PPCODE:
        meta_list = SwishResultMetaList( result );
        swish_handle = SW_ResultToSW_HANDLE( result );

        PUSHMARK(SP) ;
        XPUSHs( (SV *)swish_handle );
        XPUSHs( (SV *)meta_list );
        XPUSHs( (SV *)"SWISH::API::MetaName");
        PUTBACK ;

        call_pv("SWISH::API::push_meta_list", G_ARRAY );
        SPAGAIN;

void
SwishResultPropertyList(result)
    SW_RESULT result

    PREINIT:
        SWISH_META_LIST meta_list;
        SW_HANDLE swish_handle;

    PPCODE:
        meta_list = SwishResultPropertyList( result );
        swish_handle = SW_ResultToSW_HANDLE( result );

        PUSHMARK(SP) ;
        XPUSHs( (SV *)swish_handle );
        XPUSHs( (SV *)meta_list );
        XPUSHs( (SV *)"SWISH::API::PropertyName");
        PUTBACK ;

        call_pv("SWISH::API::push_meta_list", G_ARRAY );
        SPAGAIN;



# **************************************************************
#
#            SWISH::API::FuzzyWord
#
# Methods for accessing a SW_FUZZYWORD structure
#
# ***************************************************************


MODULE = SWISH::API        PACKAGE = SWISH::API::FuzzyWord    PREFIX = SwishFuzzy



# method to automatically free memory when object goes out of scope
void
DESTROY(fw)
    SW_FUZZYWORD fw

    CODE:
        if ( fw )
            SwishFuzzyWordFree( fw );

# returns number of words in the fuzzy structure

int
SwishFuzzyWordCount( fw )
    SW_FUZZYWORD fw

# returns return value from stemmer

int
SwishFuzzyWordError( fw )
    SW_FUZZYWORD fw


# returns an array of stemmed (or not) words.  
# The "or not" is because the word might not have been stemmed.

void
SwishFuzzyWordList( fw )
    SW_FUZZYWORD fw

    PREINIT:
        const char **list;

    PPCODE:
        list = SwishFuzzyWordList( fw );

        while ( *list )
        {
            XPUSHs(sv_2mortal( newSVpv( (char *)*list, 0 ) ));
            list++;
        }



# ********************************************************************
#
#                   SWISH::API::MetaName
#
#  Methods for accessing data about metanames
#
# ********************************************************************

MODULE = SWISH::API       PACKAGE = SWISH::API::MetaName  PREFIX = SwishMeta

void
DESTROY ( self )
    META_OBJ *self
    CODE:
        SvREFCNT_dec( self->handle_sv );
        safefree( self );


const char *
SwishMetaName( meta )
    SW_META meta

int
SwishMetaType( meta )
    SW_META meta

int
SwishMetaID( meta )
    SW_META meta


# ********************************************************************
#
#                   SWISH::API::PropertyName
#
#  Methods for accessing data about metanames
#  Should set a base class for both, but they are small classes
#  and may want different behavior in the future.
#
# ********************************************************************

MODULE = SWISH::API       PACKAGE = SWISH::API::PropertyName  PREFIX = SwishMeta

void
DESTROY ( self )
    META_OBJ *self
    CODE:
        SvREFCNT_dec( self->handle_sv );
        safefree( self );



const char *
SwishMetaName( meta )
    SW_META meta

int
SwishMetaType( meta )
    SW_META meta

int
SwishMetaID( meta )
    SW_META meta
