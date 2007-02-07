/*
$Id$
**
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**


    This file is part of Swish-e.

    Swish-e is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Swish-e is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along  with Swish-e; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    See the COPYING file that accompanies the Swish-e distribution for details
    of the GNU GPL and the special exception available for linking against
    the Swish-e library.
    
** Mon May  9 12:50:43 CDT 2005 - added GPL statement, removed LGPL
** karman: how much of this is still original HP stuff?


** Changes in expandstar and parseterm to fix the wildcard * problem.
** G. Hill, ghill@library.berkeley.edu  3/11/97
**
** Changes in notresultlist, parseterm, and fixnot to fix the NOT problem
** G. Hill, ghill@library.berkeley.edu 3/13/97
**
** Changes in search, parseterm, fixnot, operate, getfileinfo
** to support METADATA
** G. Hill 3/18/97 ghill@library.berkeley.edu
**
** Change in search to allow for search with a list including
** also some empty indexes.
** G. Hill after a suggestion by J. Winstead 12/18/97
**
** Created countResults for number of hits in search
** G. Hill 12/18/97
**
**
** Change in search to allow maxhits to return N number
** of results for each index specified
** D. Norris after suggestion by D. Chrisment 08/29/99
**
** Created resultmaxhits as a global, renewable maxhits
** D. Norris 08/29/99
**
** added word length arg to Stem() call for strcat overflow checking in stemmer.c
** added safestrcpy() macro to avoid corruption from strcpy overflow
** SRE 11/17/99
**
** 10/10/99 & 11/23/99 - Bill Moseley (merged by SRE)
**   - Changed to stem words *before* expanding with expandstar
**     so can find words in the index
**   - Moved META tag check before expandstar so META names don't get
**     expanded!
**
** fixed cast to int problems pointed out by "gcc -Wall"
** SRE 2/22/00
**
** fixed search() for case where stopword is followed by rule:
**   stopword was removed, rule was left, no matches ever found
** added "# Stopwords removed:" to output header so caller can
**   trap actions of IGNORE_STOPWORDS_IN_QUERY
** SRE 2/25/00
**
** 04/00 - Jose Ruiz
** Added code for phrase search
**     - New function phraseresultlists
**     - New function expandphrase
**
** 04/00 - Jose Ruiz
** Added freeresult function for freing results memory
** Also added changes to orresultlists andresultlists notresultlist
**  for freing memory
**
** 04/00 - Jose Ruiz
** Now use bighash instead of hash for better performance in
** orresultlist (a* or b*). Also changed hash.c
**
** 04/00 - Jose Ruiz
** Function getfileinfo rewrite
**     - Now use a hash approach for faster searching
**     - Solves the long timed searches (a* or b* or c*)
**
** 04/00 - Jose Ruiz
** Ordering of result rewrite
** Now builtin C function qsort is used for faster ordering of results
** This is useful when lots of results are found
** For example: not (axf) -> This gives you all the documents!!
**
** 06/00 - Jose Ruiz
** Rewrite of andresultlits and phraseresultlists for better permonace
** New function notresultlits for better performance
**
** 07/00 and 08/00 - Jose Ruiz
** Many modifications to make all search functions thread safe
**
** 08/00 - Added ascending and descending capabilities in results sorting
**
** 2001-02-xx rasc  search call changed, tolower changed...
** 2001-03-03 rasc  altavista search, translatechar in headers
** 2001-03-13 rasc  definable logical operators via  sw->SearchAlt->srch_op.[and|or|nor]
**                  bugfix in parse_search_string handling...
** 2001-03-14 rasc  resultHeaderOutput  -H <n>
**
** 2001-05-23 moseley - replace parse_search_string with new parser
**
** 2002-09-27 moseley - major rewrite to change to localized data in a SEARCH_OBJECT, thin out casts.
**
*/

#include "swish.h"
#include "mem.h"
#include "swstring.h"
#include "metanames.h"
#include "search.h"
#include "index.h"
#include "file.h"
#include "list.h"
#include "merge.h"
#include "hash.h"
#include "docprop.h"
#include "error.h"
#include "compress.h"
#include "result_sort.h"
#include "db.h"
#include "swish_words.h"
#include "swish_qsort.h"

#include "proplimit.h"

#include "rank.h"


/* ------ static fucntions ----------- */
static int init_sort_propIDs( DB_RESULTS *db_results, struct swline *sort_word, DB_RESULTS *last );
static void query_index( DB_RESULTS *db_results );
static int isbooleanrule(char *);
static int isunaryrule(char *);
static int getrulenum(char *);
static RESULT_LIST *sortresultsbyfilenum(RESULT_LIST *r);

static RESULT_LIST *parseterm(DB_RESULTS *db_results, int parseone, int metaID, IndexFILE * indexf, struct swline **searchwordlist);
static RESULT_LIST *operate(DB_RESULTS *db_results, RESULT_LIST * l_rp, int rulenum, char *wordin, int metaID, int andLevel, IndexFILE * indexf, int distance);
static RESULT_LIST *getfileinfo(DB_RESULTS *db_results, char *word, int metaID);
static RESULT_LIST *andresultlists(DB_RESULTS *db_results, RESULT_LIST *, RESULT_LIST *, int);
static RESULT_LIST *nearresultlists(DB_RESULTS *db_results, RESULT_LIST * l_r1, RESULT_LIST * l_r2, int andLevel, int distance);
static RESULT_LIST *orresultlists(DB_RESULTS *db_results, RESULT_LIST *, RESULT_LIST *);
static RESULT_LIST *notresultlist(DB_RESULTS *db_results, RESULT_LIST *, IndexFILE *);
static RESULT_LIST *notresultlists(DB_RESULTS *db_results, RESULT_LIST *, RESULT_LIST *);
static RESULT_LIST *phraseresultlists(DB_RESULTS *db_results, RESULT_LIST *, RESULT_LIST *, int);
static RESULT_LIST *mergeresulthashlist(DB_RESULTS *db_results, RESULT_LIST *r);
static void addtoresultlist(RESULT_LIST * l_rp, int filenum, int rank, int tfrequency, int frequency, DB_RESULTS * db_results);
static void freeresultlist(DB_RESULTS *db_results);
static void freeresult(RESULT *);
static void make_db_res_and_free(RESULT_LIST *l_res);


/**********************************************************************
* SwishRankScheme -- set the ranking scheme to use when sorting
* karman - Wed Sep  1 11:55:46 CDT 2004
*
***********************************************************************/

void SwishRankScheme(SWISH *sw, int scheme)
{
	
    sw->RankScheme = scheme;
    
}




/****************************************************************
*  New_Search_Object - Create a new search object
*
*   Pass in:
*       SWISH *sw - swish handle (database handle, if you like)
*       query - query string
*
*   Returns:
*       SEARCH_OBJECT
*
*   Notes:
*       This does not run the search, but just creates an object
*       than can generate a set of results.
*
*****************************************************************/

SEARCH_OBJECT *New_Search_Object( SWISH *sw, char *query )
{
    int         index_count;
    IndexFILE  *indexf = sw->indexlist;

    
    SEARCH_OBJECT *srch = (SEARCH_OBJECT *)emalloc( sizeof(SEARCH_OBJECT) );
    memset( srch, 0, sizeof(SEARCH_OBJECT) );

    reset_lasterror( sw );
    

    srch->sw = sw;  /* parent object */
    srch->PhraseDelimiter = PHRASE_DELIMITER_CHAR;
    srch->structure = IN_FILE;

    if ( query )
        SwishSetQuery( srch, query );


    index_count = 0;
    /* Allocate place to hold the limit arrays */
    while( indexf )
    {
        index_count++;
        indexf = indexf->next;
    }


    /* Create a table index by indexf number */
    srch->prop_limits = (PROP_LIMITS **)emalloc( sizeof(PROP_LIMITS **) * index_count );

    indexf = sw->indexlist;
    index_count = 0;

    while( indexf )
    {
        int     table_size = sizeof(PROP_LIMITS) * (indexf->header.metaCounter + 1); /* metaID start at one */
        PROP_LIMITS *index_limits;  /* an array of limits data */

        /* Create a table indexed by meta ID */
        index_limits = (PROP_LIMITS *)emalloc( table_size );
        memset( index_limits, 0, table_size );
        
        srch->prop_limits[index_count++] = index_limits;

        indexf = indexf->next;
    }


    return srch;
}

/****************************************************************
* These three funtions return the swish object's ref_count_ptr
****************************************************************/
void *SwishSearch_parent ( SEARCH_OBJECT *srch )
{
    return srch ? srch->sw->ref_count_ptr : NULL;
}

void *SwishResults_parent ( RESULTS_OBJECT *results )
{
    return results ? results->sw->ref_count_ptr : NULL;
}


/*** and this is same for the results ***/

void ResultsSetRefPtr( RESULTS_OBJECT *results, void *address )
{
    if ( !address )
        progerr("ResultsSetRefPtr - passed null address");

    results->ref_count_ptr = address;
}

void *SwishResult_parent ( RESULT *result )
{
    return result ? result->db_results->results->ref_count_ptr : NULL;
}


/********** Search object methods *************************/

void SwishSetStructure( SEARCH_OBJECT *srch, int structure )
{
    if ( srch )
        srch->structure = structure;
}

int SwishGetStructure( SEARCH_OBJECT *srch )
{
    return srch ? srch->structure : 0;
}

void SwishPhraseDelimiter( SEARCH_OBJECT *srch, char delimiter )
{
    if ( srch && delimiter && !isspace( (int)delimiter ) )
        srch->PhraseDelimiter = (int)delimiter;
}


char SwishGetPhraseDelimiter(SEARCH_OBJECT *srch)
{
    return srch ? srch->PhraseDelimiter : 0;
}


void SwishSetSort( SEARCH_OBJECT *srch, char *sort )
{
    StringList  *slsort = NULL;
    int          i;

    if ( !srch || !sort || !*sort )
        return;

    if ( srch->sort_params )
    {
        freeswline( srch->sort_params );
        srch->sort_params = NULL;
    }


    if ( !(slsort = parse_line(sort)) )
        return;

    for (i = 0; i < slsort->n; i++)
        srch->sort_params = addswline( srch->sort_params, slsort->word[i] );

    freeStringList(slsort);

}

/****************************************************************
*  Free_Search_Object - Frees a search object
*
*   Pass in:
*       SEARCH_OBJECT
*
*   Frees up memory associated with a search (not results)
*
*****************************************************************/

void Free_Search_Object( SEARCH_OBJECT *srch )
{
    int         index_count;
    IndexFILE   *indexf;
    
    if ( !srch )
        return;

    /* Free up any query parameters */

    if ( srch->query )
        efree( srch->query );


    if ( srch->sort_params )
        freeswline( srch->sort_params );



    SwishResetSearchLimit( srch );  /* clears data associated with the parameters and the processed data */

    /* Free up the limit tables */

    indexf = srch->sw->indexlist;
    index_count = 0;
    while ( indexf )
    {
        PROP_LIMITS *index_limits = srch->prop_limits[index_count++];
        efree( index_limits );
        indexf = indexf->next;
    }

    efree ( srch->prop_limits );

    efree (srch);
}



void SwishSetQuery(SEARCH_OBJECT *srch, char *words )
{
    if ( srch->query )
        efree( srch->query );

    srch->query = words ? estrdup( words ) : NULL;
}



/****************************************************************
*  New_Results_Object - Creates a structure for holding a set of results
*
*   Pass in:
*       SEARCH_OBJECT - parameters for running the search
*
*   Returns:
*       Initialized RESULTS_OBJECT
*
*   Notes:
*       This does not run the query.
*
*****************************************************************/

static RESULTS_OBJECT *New_Results_Object( SEARCH_OBJECT *srch )
{
    RESULTS_OBJECT  *results;
    IndexFILE       *indexf;
    DB_RESULTS      *last = NULL;
    int             indexf_count;

    reset_lasterror( srch->sw );
    
    
    results = (RESULTS_OBJECT *)emalloc( sizeof(RESULTS_OBJECT) );
    memset( results, 0, sizeof(RESULTS_OBJECT) );

    results->sw = srch->sw;

    /* Create place to store results */
    results->resultSearchZone = Mem_ZoneCreate("resultSearch Zone", 0, 0);

    /* Create place to store sort keys */
    results->resultSortZone = Mem_ZoneCreate("resultSort Zone", 0, 0);



    /* Add in a DB_RESULTS for each index file - place to store results for a single index file */
    indexf_count = 0;
    
    for ( indexf = srch->sw->indexlist; indexf; indexf = indexf->next )
    {
        DB_RESULTS * db_results = (DB_RESULTS *) emalloc(sizeof(DB_RESULTS));
        memset( db_results, 0, sizeof(DB_RESULTS));

        db_results->results     = results;      /* parent object */
        db_results->indexf      = indexf;
        db_results->index_num   = indexf_count++;
	db_results->srch        = srch;        /* only valid during the search */


        if ( !last )
            results->db_results = db_results;  /* first one */
        else
            last->next = db_results;

        /* memory allocations after linking the db_results into the list */
        
        if ( !init_sort_propIDs( db_results, srch->sort_params, last ) )
            return results;

        last = db_results;
    }



    /* Make sure we have a query string passed in */
    if (!srch->query || !*srch->query)
        srch->sw->lasterror = NO_WORDS_IN_SEARCH;
    else
        results->query = estrdup( srch->query );
    

    return results;
}

/**************************************************************************
*  init_sort_propIDs -- load the prop ids for a single index file
*
*  Creates an array of metaEntry's that are the sort keys
*  also creates an associated array that indicates sort direction
*
***************************************************************************/

static int init_sort_propIDs( DB_RESULTS *db_results, struct swline *sort_word, DB_RESULTS *last )
{
    int cur_length = 0;    /* array size */
    struct metaEntry *m;
    struct metaEntry *rank_meta;


    /* rank sorting is the default, and is also handled special when ranking */

    rank_meta = getPropNameByName(&db_results->indexf->header, AUTOPROPERTY_RESULT_RANK);




    reset_lasterror( db_results->indexf->sw );


    /* If no sorts specified set then default to rank */

    if ( !sort_word )  /* set the default */
    {
        db_results->num_sort_props = 1;

        /* create the array */
        db_results->sort_data = (SortData *)emalloc( sizeof( SortData ) );
        memset( db_results->sort_data, 0, sizeof( SortData ) );

        if ( !rank_meta )
            progerr("Rank is not defined as an auto property - must specify sort parameters");

        db_results->sort_data[0].property = rank_meta;
        db_results->sort_data[0].direction = 1;
        db_results->sort_data[0].is_rank_sort = 1; /* flag as special -- see result_sort.c */

        return 1;
    }
        

    while ( sort_word )
    {
        char *field = sort_word->line;
        int  sortmode = -1;  /* default */

        db_results->num_sort_props++;

        /* see if there's a "asc" or "desc" modifier following */
        
        if (sort_word->next)
        {
            if (!strcasecmp(sort_word->next->line, "asc"))
            {
                sortmode = -1; /* asc sort */
                sort_word = sort_word->next;
            }
            else if (!strcasecmp(sort_word->next->line, "desc"))
            {
                sortmode = 1; /* desc sort */
                sort_word = sort_word->next;
            }
        }

        /* array big enough? */
        if ( db_results->num_sort_props > cur_length )
        {
            cur_length += 20;

            db_results->sort_data = (SortData *)erealloc( db_results->sort_data, cur_length * sizeof( SortData ) );
            memset( db_results->sort_data, 0, cur_length * sizeof( SortData ) );
        }


        m = getPropNameByName(&db_results->indexf->header, field);
        if ( !m )
        {
            set_progerr(UNKNOWN_PROPERTY_NAME_IN_SEARCH_SORT, db_results->results->sw, 
                "Property '%s' is not defined in index '%s'", field, db_results->indexf->line);
            return 0;
        }

        /* make sure the properties are compatible for sorting */
        if ( last )
           if ( !properties_compatible( last->sort_data[db_results->num_sort_props-1].property, m ) )
           {
                set_progerr(INVALID_PROPERTY_TYPE, db_results->results->sw, 
                    "Property '%s' in index '%s' is not compatible with index '%s'", field, db_results->indexf->line, last->indexf->line);
                return 0;
            }


        db_results->sort_data[db_results->num_sort_props-1].property = m;
        db_results->sort_data[db_results->num_sort_props-1].direction = sortmode;

        /* flag special case of sorting by rank */
        if ( m == rank_meta )
            db_results->sort_data[db_results->num_sort_props-1].is_rank_sort = 1;


        sort_word = sort_word->next;
    }
    return 1;
}


/****************** Utility Methods **********************************/


int SwishHits( RESULTS_OBJECT *results )
{
    if ( !results )
        return 0;       /* probably should be an error */


    return results->total_results;
}


        
SWISH *SW_ResultToSW_HANDLE( RESULT *r )
{
    return r->db_results->indexf->sw;
}

SWISH *SW_ResultsToSW_HANDLE( RESULTS_OBJECT *results )
{
    return results->sw;
}

/****************************************************************
*  Free_Results_Object - Frees all memory associated with search results
*
*   Pass in:
*       RESULTS_OBJECT
*
*****************************************************************/


void Free_Results_Object( RESULTS_OBJECT *results )
{
    DB_RESULTS *next;
    DB_RESULTS *cur;
    int         i;

    if ( !results )
        return;

    cur = results->db_results;

    while ( cur )
    {
        next = cur->next;
        freeresultlist( cur );

        freeswline( cur->parsed_words );
        freeswline( cur->removed_stopwords );

        if ( cur->sort_data )
        {
            /* free the property pointer arrays -- */
            for ( i = 0; i < cur->num_sort_props; i++ )
                if ( cur->sort_data[i].key )
                {
                    int j;
                    for ( j = 0; j < cur->result_count; j++ )
                        if ( cur->sort_data[i].key[j] &&  cur->sort_data[i].key[j] != (propEntry *)-1 )
                           efree( cur->sort_data[i].key[j] ); /** double loop! -- memzone please */

                    efree( cur->sort_data[i].key );
                }

            efree(cur->sort_data);
         }


        /* free the property string cache, if used */
        if ( cur->prop_string_cache )
        {
            int i;
            for ( i=0; i< cur->indexf->header.metaCounter; i++ )
                if ( cur->prop_string_cache[i] )
                    efree( cur->prop_string_cache[i] );

            efree( cur->prop_string_cache );
        }
        


        efree(cur);
        cur = next;
    }

    if ( results->query )
        efree( results->query );
        

    /* Free up any results */
    Mem_ZoneFree( &results->resultSearchZone );

    /* Free up sort keys */
    Mem_ZoneFree( &results->resultSortZone );

    efree( results );
}





// #define DUMP_RESULTS 1


#ifdef DUMP_RESULTS

static void dump_result_lists( RESULTS_OBJECT *results, char *message )    
{
    DB_RESULTS *db_results = results->db_results;
    int cnt = 0;
    struct swline *query;

    printf("\nDump Results: (%s)\n", message );

    while ( db_results )
    {
        RESULT *result;
        printf("\nIndex: %s\nQuery: ", db_results->indexf->line);
        for ( query = db_results->parsed_words; query; query = query->next )
            printf("%s ", query->line );

        printf("\n");
        
        
        if ( !db_results->resultlist )
        {
            printf("  no resultlist\n");
            db_results = db_results->next;
            continue;
        }

        result = db_results->resultlist->head;

        if ( !result )
        {
            printf("  resultlist, but head is null\n");
            db_results = db_results->next;
            continue;
        }

        while ( result )
        {
            printf("  Result (%2d): filenum '%d' from index file '%s'\n", ++cnt, result->filenum, db_results->indexf->line );
            result = result->next;
        }

        printf(" end of results for index\n");

        db_results = db_results->next;
    }
    printf("end of all results.\n\n");
}
#endif





/********************************************************************************
*  SwishQuery - run a simple query
*
*   Pass in:
*       SWISH * - swish handle
*       char *  - query string
*
*   Returns:
*       RESULTS_OBJECT;
*
*
********************************************************************************/

RESULTS_OBJECT *SwishQuery(SWISH *sw, char *words )
{
    SEARCH_OBJECT *srch;
    RESULTS_OBJECT *results;

    reset_lasterror( sw );

    srch = New_Search_Object( sw, words );
    if ( sw->lasterror )
        return NULL;
        
    results = SwishExecute( srch, NULL );
    Free_Search_Object( srch );
    return results;
}




/********************************************************************************
*  SwishExecute - run a query on an existing SEARCH_OBJECT
*
*   Pass in:
*       SEARCH_OBJECT * - existing search object
*       char *  - optional query string
*
*   Returns:
*       RESULTS_OBJECT -- regardless of errors CALLER MUST DESTROY
*
*   Errors:
*       Sets sw->lasterror
*
*   ToDo:
*       localize the errorstr
*
********************************************************************************/



RESULTS_OBJECT *SwishExecute(SEARCH_OBJECT *srch, char *words)
{
    RESULTS_OBJECT *results;
    DB_RESULTS     *db_results;
    SWISH          *sw;

    if ( !srch )
        progerr("Passed in NULL search object to SwishExecute");

    sw = srch->sw;

    reset_lasterror( sw );


    /* Allow words to be passed in */
    if ( words )
        SwishSetQuery( srch, words );



    /* Create the results object based on the search input object */
    results = New_Results_Object( srch );
    if ( sw->lasterror )
        return results;


    /* This returns false when no files found within the limit */
    /* or on errors such as bad property name */

    /* $$$ make sure this is not repeated once set  */

    if ( !Prepare_PropLookup( srch ) )
        return results;



    /* Fecth results for each index file */
    db_results = results->db_results;

    while ( db_results )
    {

        /* Parse the query and run the search */
        query_index( db_results );
        

        /* Any big errors? */
        /* This is ugly, but allows processing all indexes before reporting an error */
        /* one could argue if this is the correct approach or not */
       
        
        if ( sw->lasterror )
        {
            if ( sw->lasterror == QUERY_SYNTAX_ERROR )
                return results;

            if ( sw->lasterror < results->lasterror )
                results->lasterror = sw->lasterror;

            sw->lasterror = RC_OK;
        }

        db_results = db_results->next;
    }


    /* Check for errors */
    
    if ( !results->total_files )
        sw->lasterror = INDEX_FILE_IS_EMPTY;

    else if ( !results->search_words_found )
        sw->lasterror = results->lasterror ? results->lasterror : NO_WORDS_IN_SEARCH;


    if ( sw->lasterror )
        return results;

   

    /*  Sort results by rank or by properties */

    results->total_results = sortresults( results );



    /* If no results then return the last error, or any error found while processing index files */
    if (!results->total_results )
        sw->lasterror = sw->lasterror ? sw->lasterror : results->lasterror;



#ifdef DUMP_RESULTS
    dump_result_lists( results , "After sorting" );
#endif

    return results;
}



/**************************************************************************
*  limit_result_list -- removes results that are not within the limit
*
*   Notes:
*
*   If all properties were pre-sorted would be better to limit results in
*   something like getfileinfo() before doing all the work of adding the file
*   and then removing it.  Another advantage would be that the individual
*   flag arrays (one set for each index, and one for each property name)
*   could be ANDed into a single small array (bytes or vectored)
*   for each index file.
*
*
*
***************************************************************************/

static void limit_result_list( DB_RESULTS *db_results )
{
    RESULT *result;
    RESULT *next;
    RESULT *prev;
    

    /* get first result in list */
    
    if( !(result = db_results->resultlist->head) )
        return;

    prev = NULL;

    while (result)
    {
        PROP_LIMITS *prop_limits = db_results->srch->prop_limits[db_results->index_num];
        
        if ( !LimitByProperty( db_results->indexf, prop_limits, result->filenum ) )
        {
            prev = result;
            result = result->next;
            continue;
        }

        next = result->next;

        if ( !next ) /* removing last one so set the tail to the previous one */
            db_results->resultlist->tail = prev;
            

        freeresult( result );

        if ( !prev )  /* if first in list change the head pointer */
            db_results->resultlist->head = next;
        else
            prev->next = next;

        result = result->next;            
    }

}


/********************************************************************************
*  query_index -- search a single index file
*
*   Call with:
*       srch - search object
*       indexf - the current index file
*
*   Returns:
*       DB_RESULTS - A structure that contains the list of results
*                    results have been limited with -L
*                    Note: result list may be empty
*
*   Error:
*       sets sw->lasterror on error that should abort processing
*
*   Notes:
*       $$$ Probably should always return a DB_RESULTS so can report headers
*       for all index files, and can show all the stop words removed.  FIXME!
*
*
*********************************************************************************/

static void query_index( DB_RESULTS *db_results )
{
    struct swline   *searchwordlist, *tmpswl;
    RESULTS_OBJECT  *results = db_results->results;

    

    /* This is used to detect if all the index files were empty for error reporting */
    /* $$$ Can this every happen? */
    results->total_files += db_results->indexf->header.totalfiles;


    /* convert the search into a parsed list */
    /* also sets db_results->(removed_stopwords|parsed_words) */

    if ( !(searchwordlist = parse_swish_query( db_results )) )
        return;


    results->search_words_found++;  /* flag that some words were found for search so can tell difference between all stop words removed vs. no words in query */        


    /* Now do the search */

    tmpswl = searchwordlist;

    db_results->resultlist = parseterm(db_results, 0, 1, db_results->indexf, &searchwordlist);

    freeswline( tmpswl );


    /* Limit result list by -L parameter */
    if ( db_results->srch->limit_params && db_results->resultlist )
        limit_result_list( db_results );
}

/***************************************************************************
* SwishSeekResult -- seeks to the result number specified
*
*   Returns the position or a negative number on error
*
*   Position is zero based
*
*   
*
****************************************************************************/


int     SwishSeekResult(RESULTS_OBJECT *results, int pos)
{
    int    i;
    RESULT *cur_result = NULL;

    reset_lasterror( results->sw );

    if ( pos < 0 )
        pos = 0;  /* really should warn.. */
    
    if (!results)
        return (results->sw->lasterror = INVALID_RESULTS_HANDLE);

    if ( !results->db_results )
    {
        set_progerr(SWISH_LISTRESULTS_EOF, results->sw, "Attempted to SwishSeekResult before searching");
        return SWISH_LISTRESULTS_EOF;
    }



    /* Check if only one index file -> Faster SwishSeek */

    if (!results->db_results->next)
    {
        for (i = 0, cur_result = results->db_results->sortresultlist; cur_result && i < pos; i++)
            cur_result = cur_result->next;

        results->db_results->currentresult = cur_result;
        


    } else {
        /* Well, we finally have more than one index file */
        /* In this case we have no choice - We need to read the data from disk */
        /* The easy way: Let SwishNextResult do the job */

        /* Must reset the currentresult pointers first */
        /* $$$ could keep the current result seek pos number in results, and then just offset from there if greater */
        DB_RESULTS *db_results;

        for ( db_results = results->db_results; db_results; db_results = db_results->next )
            db_results->currentresult = db_results->sortresultlist;

        /* If want first one then we are done */
        if ( 0 == pos )
            return pos;
        

        for (i = 0; i < pos; i++)
            if (!(cur_result = SwishNextResult(results)))
                break;
    }

    if (!cur_result)
        return ((results->sw->lasterror = SWISH_LISTRESULTS_EOF));

    return ( results->cur_rec_number = pos );
}





RESULT *SwishNextResult(RESULTS_OBJECT *results)
{
    RESULT *res = NULL;
    RESULT *res2 = NULL;
    int     rc;
    DB_RESULTS *db_results = NULL;
    DB_RESULTS *db_results_winner = NULL;
    SWISH   *sw = results->sw;
    
    reset_lasterror( results->sw );

    /* Seems like we should error here if there are no results */
    if ( !results->db_results )
    {
        set_progerr(SWISH_LISTRESULTS_EOF, sw, "Attempted to read results before searching");
        return NULL;
    }

    

    /* Check for a unique index file */
    if (!results->db_results->next)
    {
        if ((res = results->db_results->currentresult))
        {
            /* Increase Pointer */
            results->db_results->currentresult = res->next;
        }
    }


    else    /* tape merge to find the next one from all the index files */
    
    {
        /* We have more than one index file - can't use pre-sorted index */
        /* Get the lower value */
        db_results_winner = results->db_results;  /* get the first index */
        res = db_results_winner->currentresult;   /* and current result from first index */

        /* now loop through indexes looking for the lowest one */

        for (db_results = results->db_results->next; db_results; db_results = db_results->next)
        {
            /* Any more results for this index? If not skip and move to next index */
            if (!(res2 = db_results->currentresult))
                continue;


            if (!res)  /* first one doesn't exist, so second wins */
            {
                res = res2;
                db_results_winner = db_results;
                continue;
            }

            /* Finally, compare the properties */

            rc = compare_results(&res, &res2);

            /* If first is more than second then take second */
            if (rc < 0)
            {
                res = res2;
                db_results_winner = db_results;
            }
        }

        
        /* Move current pointer to next for this index */
        if ((res = db_results_winner->currentresult))
            db_results_winner->currentresult = res->next;
    }



    if (res)
    {
        results->cur_rec_number++;
    }
    else
    {
        // it's expected to just return null on end of list.
        // sw->lasterror = SWISH_LISTRESULTS_EOF;  
    }

        
    return res;

}






/* The recursive parsing function.
** This was a headache to make but ended up being surprisingly easy. :)
** parseone tells the function to only operate on one word or term.
** parseone is needed so that with metaA=foo bar "bar" is searched
** with the default metaname.
*/

static RESULT_LIST *parseterm(DB_RESULTS *db_results, int parseone, int metaID, IndexFILE * indexf, struct swline **searchwordlist)
{
    int     rulenum;
    char   *word;
    int     lenword;
    RESULT_LIST *l_rp,
           *new_l_rp;
    int     distance = 0;

    /*
     * The andLevel is used to help keep the ranking function honest
     * when it ANDs the results of the latest search term with
     * the results so far (rp).  The idea is that if you AND three
     * words together you ultimately want the resulting rank to
     * be the average of all three individual ranks. By keeping
     * a running total of the number of terms already ANDed, the
     * next AND operation can properly scale the average-rank-so-far
     * and recompute the new average properly (see andresultlists()).
     * This implementation is a little weak in that it will not average
     * across terms that are in parenthesis. (It treats an () expression
     * as one term, and weights it as "one".)
     */
    int     andLevel = 0;       /* number of terms ANDed so far */

    word = NULL;
    lenword = 0;

    l_rp = NULL;

    rulenum = OR_RULE;
    while (*searchwordlist)
    {
        
        word = SafeStrCopy(word, (*searchwordlist)->line, &lenword);

        if (rulenum == NO_RULE)
            rulenum = DEFAULT_RULE;


        if (isunaryrule(word))  /* is it a NOT? */
        {
            *searchwordlist = (*searchwordlist)->next;
            l_rp = parseterm(db_results, 1, metaID, indexf, searchwordlist);
            l_rp = notresultlist(db_results, l_rp, indexf);

            /* Wild goose chase */
            rulenum = NO_RULE;
            continue;
        }


        /* If it's an operator, set the current rulenum, and continue */
        else if (isbooleanrule(word))
        {
            rulenum = getrulenum(word);
            /* NEAR feature */
            if (rulenum == NEAR_RULE)
            {
                distance = atol(word + strlen(NEAR_WORD));
            }
            /* end NEAR */

            *searchwordlist = (*searchwordlist)->next;
            continue;
        }


        /* Bump up the count of AND terms for this level */
        
        if ((rulenum != AND_RULE) && (rulenum != NEAR_RULE))
            andLevel = 0;       /* reset */
        else if ((rulenum == AND_RULE) || (rulenum == NEAR_RULE))
            andLevel++;



        /* Is this the start of a sub-query? */
        /* Look for a lone "(" */

        if (word[0] == '(' && '\0' == word[1])
        {

            
            /* Recurse */
            *searchwordlist = (*searchwordlist)->next;
            new_l_rp = parseterm(db_results, 0, metaID, indexf, searchwordlist);


            if (rulenum == AND_RULE)
                l_rp = andresultlists(db_results, l_rp, new_l_rp, andLevel);

            else if (rulenum == NEAR_RULE)
                l_rp = nearresultlists(db_results, l_rp, new_l_rp, andLevel, distance);

            else if (rulenum == OR_RULE)
                l_rp = orresultlists(db_results, l_rp, new_l_rp);

            else if (rulenum == PHRASE_RULE)
                l_rp = phraseresultlists(db_results, l_rp, new_l_rp, 1);

            else if (rulenum == AND_NOT_RULE)
                l_rp = notresultlists(db_results, l_rp, new_l_rp);

            if (!*searchwordlist)
                break;

            rulenum = NO_RULE;
            continue;

        }

        /* Is this the end of a sub-query? Lone ')' */

        else if (word[0] == ')' && '\0' == word[1] )
        {
            *searchwordlist = (*searchwordlist)->next;
            break;
        }


        /* Now down to checking for metanames and actual search words */


        /* Check if the next word is '=' */
        if (isMetaNameOpNext((*searchwordlist)->next))
        {
            struct metaEntry *m = getMetaNameByName(&indexf->header, word);

            /* shouldn't happen since already checked */
            if ( !m )
                progerr("Unknown metaname '%s' -- swish_words failed to find.", word );

            metaID = m->metaID;
                
            
            /* Skip both the metaName end the '=' */
            *searchwordlist = (*searchwordlist)->next->next;

            
            if ((*searchwordlist) && ((*searchwordlist)->line[0] == '('))
            {
                *searchwordlist = (*searchwordlist)->next;
                parseone = 0;
            }
            else
                parseone = 1;

            /* Now recursively process the next terms */
            
            new_l_rp = parseterm(db_results, parseone, metaID, indexf, searchwordlist);
            if (rulenum == AND_RULE)
                l_rp = andresultlists(db_results, l_rp, new_l_rp, andLevel);

            else if (rulenum == NEAR_RULE)
                l_rp = nearresultlists(db_results, l_rp, new_l_rp, andLevel, distance);

            else if (rulenum == OR_RULE)
                l_rp = orresultlists(db_results, l_rp, new_l_rp);

            else if (rulenum == PHRASE_RULE)
                l_rp = phraseresultlists(db_results, l_rp, new_l_rp, 1);

            else if (rulenum == AND_NOT_RULE)
                l_rp = notresultlists(db_results, l_rp, new_l_rp);

            if (!*searchwordlist)
                break;

            rulenum = NO_RULE;
            metaID = 1;
            continue;
        }


        /* Finally, look up a word, and merge with previous results. */

        l_rp = operate(db_results, l_rp, rulenum, word, metaID, andLevel, indexf, distance);

        if (parseone)
        {
            *searchwordlist = (*searchwordlist)->next;
            break;
        }
        rulenum = NO_RULE;

        *searchwordlist = (*searchwordlist)->next;
    }

    if (lenword)
        efree(word);

    return l_rp;
}

/* Looks up a word in the index file -
** it calls getfileinfo(), which does the real searching.
*/

static RESULT_LIST *operate(DB_RESULTS *db_results, RESULT_LIST * l_rp, int rulenum, char *wordin, int metaID, int andLevel, IndexFILE * indexf, int distance)
{
    RESULT_LIST     *new_l_rp;
    RESULT_LIST     *return_l_rp;
    char            *word;
    int             lenword;


    /* $$$ why dup the input string?? */
    word = estrdup(wordin);
    lenword = strlen(word);

    new_l_rp = return_l_rp = NULL;


    /* Lookup the word in the index */
    new_l_rp = getfileinfo(db_results, word, metaID);

    switch (rulenum)
    {
        case AND_RULE:
            return_l_rp = andresultlists(db_results, l_rp, new_l_rp, andLevel);
            break;
        
        case NEAR_RULE:
            return_l_rp = nearresultlists(db_results, l_rp, new_l_rp, andLevel, distance);
            break;

        case OR_RULE:
            return_l_rp = orresultlists(db_results, l_rp, new_l_rp);
            break;

        case NOT_RULE:
            return_l_rp = notresultlist(db_results, new_l_rp, indexf);
            break;

        case PHRASE_RULE:
            return_l_rp = phraseresultlists(db_results, l_rp, new_l_rp, 1);
            break;

        case AND_NOT_RULE:
            return_l_rp = notresultlists(db_results, l_rp, new_l_rp);
            break;
    }


    efree(word);
    return return_l_rp;
}


static RESULT_LIST *newResultsList(DB_RESULTS *db_results)
{
    RESULTS_OBJECT *results = db_results->results;
    
    RESULT_LIST *result_list = (RESULT_LIST *)Mem_ZoneAlloc(results->resultSearchZone, sizeof(RESULT_LIST));
    memset( result_list, 0, sizeof( RESULT_LIST ) );

    result_list->results = results;
    return result_list;
}

static void addResultToList(RESULT_LIST *l_r, RESULT *r)
{
    r->next = NULL;

    if(!l_r->head)
        l_r->head = r;
    if(l_r->tail)
        l_r->tail->next = r;
    l_r->tail = r;

}


/* Routine to test structure in a result */
/* Also removes posdata that do not fit with structure field */
static int test_structure(int structure, int frequency, unsigned int *posdata)
{
    int i, j;    /* i -> counter upto frequency, j -> new frequency */
    int *p,*q;   /* Use pointers to ints instead of arrays for
                 ** faster proccess */
    
    for(i = j = 0, p = q = (int*)posdata; i < frequency; i++, p++)
    {
        if(GET_STRUCTURE(*p) & structure)
        {
            if(p - q)
            {
                *q = *p;
            }
            j++;
            q++;
        }
    }
    return j;  /* return new frequency */
}



/* Finds a word and returns its corresponding file and rank information list.
** If not found, NULL is returned.
*/
/* Jose Ruiz
** New implmentation based on Hashing for direct access. Faster!!
** Also solves stars. Faster!! It can even found "and", "or"
** when looking for "an*" or "o*" if they are not stop words
*/

#define MAX_POSDATA_STACK 256

static RESULT_LIST *getfileinfo(DB_RESULTS *db_results, char *word, int metaID)
{
    int     j,
            x,
            filenum,
            frequency,
            len,
            curmetaID,
            index_structure,
            index_structfreq,
            tmpval;
    char remains[100];   // hard-coded !!!?
    char myWord[100];
    int           rLen;
    int           tLen;
    unsigned char   *q;
    RESULT_LIST *l_rp, *l_rp2;
    sw_off_t    wordID;
    int     metadata_length;
    char   *p;
    int     tfrequency = 0;
    unsigned char   *s, *buffer; 
    int     sz_buffer;
    unsigned char flag;
    unsigned int     stack_posdata[MAX_POSDATA_STACK];  /* stack buffer for posdata */
    unsigned int    *posdata;
    IndexFILE  *indexf = db_results->indexf;
    SWISH  *sw = indexf->sw;
    int     structure = db_results->srch->structure;
    unsigned char *start;
    int saved_bytes = 0;

    x = j = filenum = frequency = len = curmetaID = index_structure = index_structfreq = 0;
    metadata_length = 0;


    l_rp = l_rp2 = NULL;

    /* how would we ever get a word here with faulty wildcards, 
       since swish_words parses for them already?
     */
    
    if (*word == '*')
    {
        sw->lasterror = WILDCARD_NOT_ALLOWED_AT_WORD_START;
        return NULL;
    }


    if (*word == '?')   /* ? may not start a word, just like * may not */
    {
        sw->lasterror = WILDCARD_NOT_ALLOWED_AT_WORD_START;
        return NULL;
    }
  
 

    /* First: Look for star at the end of the word */
    if ((p = strrchr(word, '*')))
    {
        if (p != word && *(p - 1) == '\\') /* Check for an escaped * */
        {
            p = NULL;           /* If escaped it is not a wildcard */
        }
        else
        {
            /* Check if it is at the end of the word */
            if (p == (word + strlen(word) - 1))
            {
                word[strlen(word) - 1] = '\0';
                /* Remove the wildcard - p remains not NULL */
            }
            else
            {
                p = NULL;       /* Not at the end - Ignore */
            }
        }
    }

    /* Second: Look for question mark somewhere in the word */
    strcpy(remains, (char*)"");
    rLen = 0;
    tLen = strlen(word);
    // Check for first "?" in current word (not reverse)
    if ((q = strchr(word, '?')))
    {
        if (q != (unsigned char*)word && *(q - 1) == '\\') /* Check for an escaped * */
        {
            q = NULL;           /* If escaped it is not a wildcard */
        }
        else
        {
            /* Check if it is at the end of the word */
            if (q == ((unsigned char*)word + strlen(word) - 1))
            {
                strcpy(remains, q);   // including the last "?"
                rLen = strlen(remains);
                word[strlen(word) - 1] = '\0';
            }
            else
            {
                strcpy(remains, q);   // including the first "?"
                rLen = strlen(remains);
                *q = '\0';
            }
        }
    }


    DB_InitReadWords(sw, indexf->DB);
    if ((!p) && (!q))    /* No wildcard -> Direct hash search */
    {
        DB_ReadWordHash(sw, word, &wordID, indexf->DB);

        if(!wordID)
        {    
            DB_EndReadWords(sw, indexf->DB);
            // sw->lasterror = WORD_NOT_FOUND;
            return NULL;
        }
    }        

    else  /* There is a wildcard. So use the sequential approach */
    {       
        unsigned char   *resultword;

        if (*word == '*')
        {
            sw->lasterror = UNIQUE_WILDCARD_NOT_ALLOWED_IN_WORD;
            return NULL;
        }

        
        DB_ReadFirstWordInvertedIndex(sw, word, (char**)&resultword, &wordID, indexf->DB);

        if (!wordID)
        {
            DB_EndReadWords(sw, indexf->DB);
            // sw->lasterror = WORD_NOT_FOUND;
            return NULL;
        }
        else
            strcpy(myWord, resultword);   // Remember the word
            
        efree(resultword);   /* Do not need it */
    }


    /* If code is here we have found the word !! */

    do
    {
    
       // Check if this could be a match (only if "?" is present
       if (rLen)
       {
          char *pw, *ps;
          int found = 0;
          pw = &remains[0];
          ps = &myWord[strlen(word)];

          for (; *pw && *ps; pw++, ps++)
          {
            if (*pw == '?')
            {
              if (!p)
              {
                // no wildcard "*" at end, so length should exactly match
                if ((pw == &remains[strlen(remains) - 1]) && (*(ps + 1) == '\0'))
                  found = 1;
                else
                  continue;
              }
              else
              {
                // wildcard at end, so ignore length
                if (pw == &remains[strlen(remains) - 1])
                  found = 1;
                else
                  continue;

              }
            }

            if (*pw != *ps)
              break;

            if (!p)
            {
              if ((pw == &remains[strlen(remains) - 1]) && (*(ps + 1) == '\0'))
                found = 1;
            }
            else
            {
              if (pw == &remains[strlen(remains) - 1])
                found = 1;
            }

          }

          if (!found)
          {
            unsigned char   *resultword;

            /* Jump to next word */
            /* No more data for this word but we
               are in sequential search because of
               the star (p is not null) */
            /* So, go for next word */
            DB_ReadNextWordInvertedIndex(sw, word, (char**)&resultword, &wordID, indexf->DB);
            if (! wordID)
                break;          /* no more data */
            else
              strcpy(myWord, resultword);

            efree(resultword);  /* Do not need it (although might be useful for highlighting some day) */

            continue;
          }
       }


        DB_ReadWordData(sw, wordID, &buffer, &sz_buffer, &saved_bytes , indexf->DB);
        uncompress_worddata(&buffer,&sz_buffer,saved_bytes);

        s = buffer;

        // buffer structure = <tfreq><metaID><delta to next meta>

        /* Get the data of the word */
        tfrequency = uncompress2(&s); /* tfrequency - number of files with this word */

        /* Now look for a correct Metaname */
        curmetaID = uncompress2(&s);

        while (curmetaID)
        {
            metadata_length = uncompress2(&s);
            
            if (curmetaID >= metaID)
                break;

            /* If this is not the searched metaID jump onto next one */
            s += metadata_length;

            /* Check if no more meta data */
            if(s == (buffer + sz_buffer))
                break; /* exit if no more meta data */

            curmetaID = uncompress2(&s);
        }

        if (curmetaID == metaID) /* found a matching meta value */
        {
            int meta_rank = metaID * -1;  /*  store metaID in rank value until computed by getrank() */
            filenum = 0;
            start = s;   /* points to the star of data */
            do
            {
                /* Read on all items */
                uncompress_location_values(&s,&flag,&tmpval,&frequency);
                filenum += tmpval;  

                /* stack_posdata is just to avoid calling emalloc */
                /* it should be enough for most cases */
                if(frequency > MAX_POSDATA_STACK)
                    posdata = (unsigned int *)emalloc(frequency * sizeof(int));
                else
                    posdata = stack_posdata;

                /* read positions */
                uncompress_location_positions(&s,flag,frequency,posdata);

                /* test (limit by) structure and adjust frequency */
                frequency = test_structure(structure, frequency, posdata);

                /* Store metaID * -1 in rank - In this way, we can delay its computation */

                /* Store result */
                /* 2003-01 jmruiz. Check also if file is deleted */
                if(frequency && ((!indexf->header.removedfiles) || DB_CheckFileNum(sw,filenum,indexf->DB)))
                {
                    /* This is very useful if we sorted by other property */
                    if(!l_rp)
                       l_rp = newResultsList(db_results);

                    /*
		       tfrequency = number of files with this word
                       frequency = number of times this words is in this document for this metaID
                       metarank is the negative of the metaID - for use in getrank()
		    */

                    addtoresultlist(l_rp, filenum, meta_rank, tfrequency, frequency, db_results);

                    /* Copy positions */
                    memcpy((unsigned char *)l_rp->tail->posdata,(unsigned char *)posdata,frequency * sizeof(int));

                    /* Calculate rank now -- can't delay as an optimization */
                    getrank( l_rp->tail );
                }
                if(posdata != stack_posdata)
                    efree(posdata);
                    

            } while ((s - start) != metadata_length);


        }

        efree(buffer);


        if ((!p) && (!q))
            break;              /* direct access (no wild card) -> break */

        else
        {
            unsigned char   *resultword;

            /* Jump to next word */
            /* No more data for this word but we
               are in sequential search because of
               the star (p is not null) */
            /* So, go for next word */
            DB_ReadNextWordInvertedIndex(sw, word, (char**)&resultword, &wordID, indexf->DB);
            if (! wordID)
                break;          /* no more data */

            else
                strcpy(myWord, resultword); // remember the word

            efree(resultword);  /* Do not need it (although might be useful for highlighting some day) */
        }

    } while(1);   /* continue on in loop for wildcard search */



    if ((p) || (q))
    {
        /* Finally, if we are in an sequential search merge all results */
        l_rp = mergeresulthashlist(db_results, l_rp);
    }

    DB_EndReadWords(sw, indexf->DB);
    return l_rp;
}


/*
  -- Rules checking
  -- u_is...  = user rules (and, or, ...)
  -- is...    = internal rules checking
 */




/* Is a word a boolean rule?
*/

static int     isbooleanrule(char *word)
{
    if (!strcmp(word, AND_WORD) || !strncmp(word, NEAR_WORD, strlen(NEAR_WORD)) || !strcmp(word, OR_WORD) || !strcmp(word, PHRASE_WORD) || !strcmp(word, AND_NOT_WORD))
        return 1;
    else
        return 0;
}

/* Is a word a unary rule?
*/

static int     isunaryrule(char *word)
{
    if (!strcmp(word, NOT_WORD))
        return 1;
    else
        return 0;
}

/* Return the number for a rule.
*/

static int     getrulenum(char *word)
{
    if (!strcmp(word, AND_WORD))
        return AND_RULE;
    else if (!strncmp(word, NEAR_WORD, strlen(NEAR_WORD)))
        return NEAR_RULE;
    else if (!strcmp(word, OR_WORD))
        return OR_RULE;
    else if (!strcmp(word, NOT_WORD))
        return NOT_RULE;
    else if (!strcmp(word, PHRASE_WORD))
        return PHRASE_RULE;
    else if (!strcmp(word, AND_NOT_WORD))
        return AND_NOT_RULE;
    return NO_RULE;
}

// Check if new position is still valid for ALL other
// position sequences; at least one position within each
// sequence should be valid
// Definition of sequence: one or more positions, where each
//                         sequence is separated from another
//                         by means of a "0" (zero)
static int KeepPos(RESULT *r, int pos, int dist)
{
  int i;
  int pos1;
  int first;
  int found;
  int detect;

  // no earlier "nearx" for this document; so the position
  // to be checked is always a valid one, otherwise it wouldn't
  // arrive here
  if (r->bArea == 0)
    return(1);

  found = 0;
  detect = 0;
  first = 1;
  for (i = 0; i < r->frequency; i++)
  {
    pos1 = GET_POSITION(r->posdata[i]);
    if (pos1 == 0)
    {
      if (first)
      {
        found = detect;
        first = 0;
      }
      else
      {
        found = found & detect;
      }
      detect = 0;
      continue;
    }
    else
    {
      if (abs(pos1 - pos) <= dist)
        detect = 1;
    }
  }

  // Also for positions after last 0
  found = found & detect;

  if (found)
    return (1);

  return(0);
}

/* NEAR WORD feature -- proximity hits */
/* this and other NEAR WORD code contributed by Herman Knoops hk.sw@knoman.com */
//#define DUMP_NEAR_VALUES  1

// This is a special case of proximity. A sequence of single words/terms connected
// via nearX, will be checked for proximity on a certain area, e.g. wordA near50
// wordB near50 wordC will give a hit if all three words are in an area of 50 words.

// A generic nearX can be easily derived from this.

/* Takes two lists of results from searches and ANDs them together.
** On input, both result lists r1 and r2 must be sorted by filenum
** On output, the new result list remains sorted
*/
static RESULT_LIST *nearresultlists(DB_RESULTS *db_results, RESULT_LIST * l_r1, RESULT_LIST * l_r2, int andLevel, int distance)
{
    RESULT_LIST *new_results_list = NULL;
    RESULT *r1;
    RESULT *r2;
    int res = 0;
    int i, j, pos1, pos2;
    int found, found1, found2, detect1, detect2;
    int iZero = 0;
    int first1, first2;
    int *posd1 = NULL;
    int *posd2 = NULL;
    int cnt1, cnt2;
#ifdef DUMP_NEAR_VALUES
    FILE *ofd;
    int maxneed = 0;
#endif

    // Check if a valid distance was specified
    if (distance == 0)
      return(andresultlists(db_results, l_r1, l_r2, andLevel));

    /* patch provided by Mukund Srinivasan */
    if (l_r1 == NULL || l_r2 == NULL)
    {
        make_db_res_and_free(l_r1);
        make_db_res_and_free(l_r2);
        return NULL;
    }

#ifdef DUMP_NEAR_VALUES
    // Do not open earlier, otherwise file could be left open !!!
    ofd = fopen("kmlog.txt", "ab");
#endif

    if (andLevel < 1)
        andLevel = 1;

    for (r1 = l_r1->head, r2 = l_r2->head; r1 && r2;)
    {
        res = r1->filenum - r2->filenum;
        if (!res)
        {
            /*
             * Computing the new rank is interesting because
             * we want to weight each of the words that was
             * previously ANDed equally along with the new word.
             * We compute a running average using andLevel and
             * simply scale up the old average (in r1->rank)
             * and recompute a new, equally weighted average.
             */
            int     newRank = 0;
#ifdef DUMP_NEAR_VALUES
            int     maxpos;

            // Determine max combinations
            maxpos = (r1->frequency * r2->frequency);

            if (maxneed > 0)
            {
              fprintf(ofd,"  maxneed: %ld\n", maxneed);
              maxneed = 0; // reset for next to come
            }

#endif
            cnt1 = 0;
            cnt2 = 0;

#ifdef DUMP_NEAR_VALUES
            // make sure to skip the found entry if not within given proximity
            fprintf(ofd, "file %ld (andLevel %ld)\n", r1->filenum, andLevel);

            // Detects if there was already a "nearx" executed before, which
            // means there must be one or more "0" present in positions
            if (r1->bArea > 0)
              fprintf(ofd,"  bArea1: %ld\n", r1->bArea);
            // This can never happen, as long as no parenthesis/brackets
            // are supported; the complete query is parsed left to right
            // TODO: modify if priority brackets are going to be supported
            if (r2->bArea > 0)
              fprintf(ofd,"  bArea2: %ld\n", r2->bArea);

            fprintf(ofd,"  maxpos: %ld\n", maxpos);
            fprintf(ofd,"  %s: ", "term1");
            for (j = 0; j < r1->frequency; j++)
              fprintf(ofd, "  %ld:", GET_POSITION(r1->posdata[j]));
            fprintf(ofd,"\n");

            fprintf(ofd,"  %s: ", "term2");
            for (j = 0; j < r2->frequency; j++)
              fprintf(ofd, "  %ld:", GET_POSITION(r2->posdata[j]));
            fprintf(ofd,"\n");
#endif

            found1 = found2 = 0;
            detect1 = detect2 = 0;
            first1 = first2 = 1;

            for (i = 0; i < r1->frequency; i++)
            {
              pos1 = GET_POSITION(r1->posdata[i]);

              // Check if "0" is present in a posdata array. If so, there has been one or more
              // AND combinations already (each separated with "0").
              if (pos1 == 0)
              {
                if (first1 == 1)
                {
                  found1 = detect1;
                  first1 = 0;
                }
                else
                {
                  found1 = found1 & detect1;
                }
                // Reset to start checking next serie after an "0"
                detect1 = 0;
                cnt1++;

                // BUGFIX: copy also the 0 in between, otherwise more than
                // two "and" operator will gor wrong for the 3td, 4th, etc.
                if (posd1)
                  posd1 = (int *)erealloc(posd1, cnt1 * sizeof(int));
                else
                  posd1 = (int *)emalloc(cnt1 * sizeof(int));
                posd1[cnt1-1] = r1->posdata[i];

                continue;
              }

              for (j = 0; j < r2->frequency; j++)
              {
                pos2 = GET_POSITION(r2->posdata[j]);

                // Check if "0" is present in a posdata array. If so, there has been one or more
                // AND combinations already (each separated with "0").
                if (pos2 == 0)
                {
                  if (first2 == 1)
                  {
                    found2 = detect2;
                    first2 = 0;
                  }
                  else
                  {
                    found2 = found2 & detect2;
                  }
                  // Reset to start checking next serie after an "0"
                  detect2 = 0;
                  cnt2++;

                  // BUGFIX: copy also the 0 in between, otherwise more than
                  // two "and" operator will gor wrong for the 3td, 4th, etc.
                  if (posd2)
                    posd2 = (int *)erealloc(posd2, cnt2 * sizeof(int));
                  else
                    posd2 = (int *)emalloc(cnt2 * sizeof(int));
                  posd2[cnt2-1] = r2->posdata[j];

                  continue;     // skip 0
                }
// enable: maybe if parenthesis support for near is added ???
// enable ??               if ((abs(pos1 - pos2) <= distance) && (pos1 != pos2))
                if ((abs(pos1 - pos2) <= distance) && KeepPos(r1, pos2, distance))
                {
                  detect1 = detect2 = 1;

#ifdef DUMP_NEAR_VALUES
                  maxneed++;
                  fprintf(ofd, "  hit %ld: (%ld - %ld): %ld\n", i, pos1, pos2, abs(pos1 - pos2));
#endif
                  cnt1++;
                  cnt2++;
                  if (posd1)
                    posd1 = (int *)erealloc(posd1, cnt1 * sizeof(int));
                  else
                    posd1 = (int *)emalloc(cnt1 * sizeof(int));
                  if (posd2)
                    posd2 = (int *)erealloc(posd2, cnt2 * sizeof(int));
                  else
                    posd2 = (int *)emalloc(cnt2 * sizeof(int));
                  
                  posd1[cnt1-1] = r1->posdata[i];
                  posd2[cnt2-1] = r2->posdata[j];
                }
              } // for r2
            } // for r1

            // Check if this was the first serie of two or more "0" connected patterns.
            // For single patterns (no "0"), the variable "found" is already filled correctly.
            if (first1 == 1)
              found1 = detect1;
            else
              found1 = found1 & detect1;

            if (first2 == 1)
              found2 = detect2;
            else
              found2 = found2 & detect2;

            // overall result
            found = found1 & found2;


            // if there was a proximity hit then process it
            if (found)
            {
              newRank = ((r1->rank * andLevel) + r2->rank) / (andLevel + 1);

              if(!new_results_list)
                  new_results_list = newResultsList(db_results);

              addtoresultlist(new_results_list, r1->filenum, newRank, 0, cnt1 + cnt2 + 1, db_results);

              new_results_list->tail->bArea = r1->bArea + r2->bArea + 1;

              /* Storing all positions could be useful in the future  */
              /* BEWARE: an extra zero is inserted to make sure ALL words/terms of a previous near-operation */
              /*         also have a proximity to this new word/term */
              /*         Could give side-effects for people, who use the positions for highlighting !!! */ 

              CopyPositions(new_results_list->tail->posdata, 0, posd1, 0, cnt1);
              CopyPositions(new_results_list->tail->posdata, cnt1, &iZero, 0, 1);
              CopyPositions(new_results_list->tail->posdata, cnt1 + 1, posd2, 0, cnt2);
            }

            // Free if allocation has been performed
            if (posd1 != NULL)
            {
              efree(posd1);
              posd1 = NULL;
            }
            if (posd2 != NULL)
            {
              efree(posd2);
              posd2 = NULL;
            }

            r1 = r1->next;
            r2 = r2->next;
        }

        else if (res > 0)
        {
            r2 = r2->next;
        }
        else
        {
            r1 = r1->next;
        }
    } // for


#ifdef DUMP_NEAR_VALUES
    if (maxneed > 0)
      fprintf(ofd,"  maxneed: %ld\n", maxneed);

    fclose(ofd);
#endif

    return new_results_list;
}



/* Takes two lists of results from searches and ANDs them together.
** On input, both result lists r1 and r2 must be sorted by filenum
** On output, the new result list remains sorted
*/

static RESULT_LIST *andresultlists(DB_RESULTS *db_results, RESULT_LIST * l_r1, RESULT_LIST * l_r2, int andLevel)
{
    RESULT_LIST *new_results_list = NULL;
    RESULT *r1;
    RESULT *r2;
    int     res = 0;


    /* patch provided by Mukund Srinivasan */
    if (l_r1 == NULL || l_r2 == NULL)
    {
        make_db_res_and_free(l_r1);
        make_db_res_and_free(l_r2);
        return NULL;
    }

    if (andLevel < 1)
        andLevel = 1;

    for (r1 = l_r1->head, r2 = l_r2->head; r1 && r2;)
    {
        res = r1->filenum - r2->filenum;
        if (!res)
        {
            /*
             * Computing the new rank is interesting because
             * we want to weight each of the words that was
             * previously ANDed equally along with the new word.
             * We compute a running average using andLevel and
             * simply scale up the old average (in r1->rank)
             * and recompute a new, equally weighted average.
             */
            int     newRank = 0;

            newRank = ((r1->rank * andLevel) + r2->rank) / (andLevel + 1);


#ifdef DEBUG_RANK
    fprintf( stderr, "\n----\nFile num: %d  1st score: %d  2nd score: %d  andLevel: %d  newRank:  %d\n----\n", r1->filenum, r1->rank, r2->rank, andLevel, newRank );
#endif
            

            if(!new_results_list)
                new_results_list = newResultsList(db_results);

            addtoresultlist(new_results_list, r1->filenum, newRank, 0, r1->frequency + r2->frequency, db_results);


            /* Storing all positions could be useful in the future  */

            CopyPositions(new_results_list->tail->posdata, 0, r1->posdata, 0, r1->frequency);
            CopyPositions(new_results_list->tail->posdata, r1->frequency, r2->posdata, 0, r2->frequency);


            r1 = r1->next;
            r2 = r2->next;
        }

        else if (res > 0)
        {
            r2 = r2->next;
        }
        else
        {
            r1 = r1->next;
        }
    }

    return new_results_list;
}

/* Takes two lists of results from searches and ORs them together.
2001-11 jmruiz Completely rewritten. Older one was really
               slow when the lists are very long
               On input, both result lists r1 and r2 must be sorted by filenum
               On output, the new result list remains sorted

               rank is combined for matching files.  That is,
               "foo OR bar" will rank files with both higher.

*/


static RESULT_LIST *orresultlists(DB_RESULTS *db_results, RESULT_LIST * l_r1, RESULT_LIST * l_r2)
{
    int     rc;
    RESULT *r1;
    RESULT *r2;
    RESULT *rp,
           *tmp;
    RESULT_LIST *new_results_list = NULL;
    RESULTS_OBJECT *results = db_results->results;



    /* If either list is empty, just return the other */
    if (l_r1 == NULL)
        return l_r2;

    else if (l_r2 == NULL)
        return l_r1;

    /* Look for files that have both words, and add up the ranks */

    r1 = l_r1->head;
    r2 = l_r2->head;

    while(r1 && r2)
    {
        rc = r1->filenum - r2->filenum;
        if(rc < 0)
        {
            rp = r1;
            r1 = r1->next;
        }
        else if(rc > 0)
        {
            rp = r2;
            r2 = r2->next;
        }

        else /* Matching file number */
        {
            int result_size;
            
            /* Create a new RESULT - Should be a function to creete this, I'd think */

            result_size = sizeof(RESULT) + ( (r1->frequency + r2->frequency - 1) * sizeof(int) );
            rp = (RESULT *) Mem_ZoneAlloc(results->resultSearchZone, result_size );
            memset( rp, 0, result_size );

            rp->fi.filenum = rp->filenum = r1->filenum;

            rp->rank = ( r1->rank + r2->rank) * 2;  /* bump up the or terms */
	    
#ifdef DEBUG_RANK
    fprintf( stderr, "\n----\nFile num: %d  1st score: %d  2nd score: %d  newRank:  %d\n----\n", r1->filenum, r1->rank, r2->rank, rp->rank );
#endif

            rp->tfrequency = 0;
            rp->frequency = r1->frequency + r2->frequency;
            rp->db_results = r1->db_results;

            
            /* save the combined position data in the new result.  (Would freq ever be zero?) */
            if (r1->frequency)
                CopyPositions(rp->posdata, 0, r1->posdata, 0, r1->frequency);

            if (r2->frequency)
                CopyPositions(rp->posdata, r1->frequency, r2->posdata, 0, r2->frequency);

            r1 = r1->next;
            r2 = r2->next;
        }


        /* Now add the result to the output list */
        
        if(!new_results_list)
            new_results_list = newResultsList(db_results);

        addResultToList(new_results_list,rp);
    }


    /* Add the remaining results */

    tmp = r1 ? r1 : r2;

    while(tmp)
    {
        rp = tmp;
        tmp = tmp->next;
        if(!new_results_list)
            new_results_list = newResultsList(db_results);

        addResultToList(new_results_list,rp);
    }

    return new_results_list;
}


/* 2001-10 jmruiz - This code was originally at merge.c
**                  Also made it thread safe 
*/
/* These three routines are only used by notresultlist */

struct markentry
{
    struct markentry *next;
    int     num;
};

/* This marks a number as having been printed.
*/

static void    marknum(RESULTS_OBJECT *results, struct markentry **markentrylist, int num)
{
    unsigned hashval;
    struct markentry *mp;

    mp = (struct markentry *) Mem_ZoneAlloc( results->resultSearchZone, sizeof(struct markentry));

    mp->num = num;

    hashval = bignumhash(num);
    mp->next = markentrylist[hashval];
    markentrylist[hashval] = mp;
}


/* Has a number been printed?
*/

static int     ismarked(struct markentry **markentrylist, int num)
{
    unsigned hashval;
    struct markentry *mp;

    hashval = bignumhash(num);
    mp = markentrylist[hashval];

    while (mp != NULL)
    {
        if (mp->num == num)
            return 1;
        mp = mp->next;
    }
    return 0;
}

/* Initialize the marking list.
*/

static void    initmarkentrylist(struct markentry **markentrylist)
{
    int     i;

    for (i = 0; i < BIGHASHSIZE; i++)
        markentrylist[i] = NULL;
}

static void    freemarkentrylist(struct markentry **markentrylist)
{
    int     i;

    for (i = 0; i < BIGHASHSIZE; i++)
    {
        markentrylist[i] = NULL;
    }
}

/* This performs the NOT unary operation on a result list.
** NOTed files are marked with a default rank of 1000.
**
** Basically it returns all the files that have not been
** marked (GH)
*/

static RESULT_LIST *notresultlist(DB_RESULTS *db_results, RESULT_LIST * l_rp, IndexFILE * indexf)
{
    int     i,
            filenums;
    RESULT *rp;
    RESULT_LIST *new_results_list = NULL;
    struct markentry *markentrylist[BIGHASHSIZE];
    RESULTS_OBJECT *results = db_results->results;

    if(!l_rp)
        rp = NULL;
    else
        rp = l_rp->head;

    initmarkentrylist(markentrylist);
    while (rp != NULL)
    {
        marknum(results, markentrylist, rp->filenum);
        rp = rp->next;
    }

    filenums = indexf->header.totalfiles;

    for (i = 1; i <= filenums; i++)
    {
        if (!ismarked(markentrylist, i) && DB_CheckFileNum( indexf->sw, i, indexf->DB ) )
        {
            if(!new_results_list)
                new_results_list = newResultsList(db_results);

            addtoresultlist(new_results_list, i, 1000, 0, 0, db_results);
        }
    }

    freemarkentrylist(markentrylist);

    new_results_list = sortresultsbyfilenum(new_results_list);

    return new_results_list;
}

/* Phrase result routine - see distance parameter. For phrase search this
** value must be 1 (consecutive words)
**
** On input, both result lists r1 abd r2 must be sorted by filenum
** On output, the new result list remains sorted
*/
static RESULT_LIST *phraseresultlists(DB_RESULTS *db_results, RESULT_LIST * l_r1, RESULT_LIST * l_r2, int distance)
{
    int     i,
            j,
            found,
            newRank,
           *allpositions;
    int     res = 0;
    RESULT_LIST *new_results_list = NULL;
    RESULT *r1, *r2;
                


    if (l_r1 == NULL || l_r2 == NULL)
    {
        make_db_res_and_free(l_r1);
        make_db_res_and_free(l_r2);
        return NULL;
    }

    for (r1 = l_r1->head, r2 = l_r2->head; r1 && r2;)
    {
        res = r1->filenum - r2->filenum;
        if (!res)
        {
            found = 0;
            allpositions = NULL;
            for (i = 0; i < r1->frequency; i++)
            {
                for (j = 0; j < r2->frequency; j++)
                {
                    if ((GET_POSITION(r1->posdata[i]) + distance) == GET_POSITION(r2->posdata[j]))
                    {
                        found++;
                        if (allpositions)
                            allpositions = (int *) erealloc(allpositions, found * sizeof(int));

                        else
                            allpositions = (int *) emalloc(found * sizeof(int));

                        allpositions[found - 1] = r2->posdata[j];
                        break;
                    }
                }
            }
            if (found)
            {
                newRank = (r1->rank + r2->rank) / 2;

                /*
                   * Storing positions is neccesary for further
                   * operations 
                 */
                if(!new_results_list)
                    new_results_list = newResultsList(db_results);
                
                addtoresultlist(new_results_list, r1->filenum, newRank, 0, found, db_results);

                CopyPositions(new_results_list->tail->posdata, 0, allpositions, 0, found);
                efree(allpositions);
            }
            r1 = r1->next;
            r2 = r2->next;
        }
        else if (res > 0)
        {
            r2 = r2->next;
        }
        else
        {
            r1 = r1->next;
        }

    }

    return new_results_list;
}



/* Adds a file number and rank to a list of results.
*/


static void addtoresultlist(RESULT_LIST * l_rp, int filenum, int rank, int tfrequency, int frequency, DB_RESULTS *db_results)
{
    RESULT *newnode;
    int     result_size;
    RESULTS_OBJECT *results = db_results->results;

    result_size = sizeof(RESULT) + ((frequency - 1) * sizeof(int));
    newnode = (RESULT *) Mem_ZoneAlloc( results->resultSearchZone, result_size );
    memset( newnode, 0, result_size );
    newnode->fi.filenum = newnode->filenum = filenum;

    newnode->rank = rank;
    newnode->tfrequency = tfrequency;
    newnode->frequency = frequency;
    
    newnode->bArea = 0;
    newnode->pArea = NULL;
    
    newnode->db_results = db_results;

    addResultToList(l_rp, newnode);
}



/* Checks if the next word is "="
*/

int     isMetaNameOpNext(struct swline *searchWord)
{
    if (searchWord == NULL)
        return 0;

    if (!strcmp(searchWord->line, "="))
        return 1;

    return 0;
}

/* Free up a list of results that has not been assigned to a DB_RESULTS struct yet */
/* July 03 - This is called by andresultlists() and phraseresultlists() */
/*           isn't really needed because at this point in the search there's nothing  */
/*           attached to the result that needs to be freed. */

static void  make_db_res_and_free(RESULT_LIST *l_res)
{
    DB_RESULTS tmp;
    memset (&tmp,0,sizeof(DB_RESULTS));
    tmp.resultlist = l_res;
    freeresultlist(&tmp);
}



/* funtion to free all memory of a list of results */
static void    freeresultlist(DB_RESULTS *dbres)
{
    RESULT *rp;
    RESULT *tmp;

    if(dbres->resultlist)
        rp = dbres->resultlist->head;
    else
        rp = NULL;

    while (rp)
    {
        tmp = rp->next;
        freeresult(rp);
        rp = tmp;
    }
    dbres->resultlist = NULL;
    dbres->currentresult = NULL;
    dbres->sortresultlist = NULL;
}

/* funtion to free the memory of one result */
static void    freeresult(RESULT * rp)
{
    DB_RESULTS *db_results;

    if (!rp)
        return;


    freefileinfo( &rp->fi );  // may have already been freed

    
    db_results = rp->db_results;

}



/* 01/2001 Jose Ruiz */
/* Compare RESULTS using RANK */
/* This routine is used by qsort */
static int     compResultsByFileNum(const void *s1, const void *s2)
{
    return ((*(RESULT * const *) s1)->filenum - (*(RESULT * const *) s2)->filenum);
}



/* 
06/00 Jose Ruiz - Sort results by filenum
Uses an array and qsort for better performance
Used for faster "and" and "phrase" of results
*/
static RESULT_LIST *sortresultsbyfilenum(RESULT_LIST * l_rp)
{
    int     i,
            j;
    RESULT **ptmp;
    RESULT *rp;

    /* Very trivial case */
    if (!l_rp)
        return NULL;


    /* Compute results */
    for (i = 0, rp = l_rp->head; rp; rp = rp->next, i++);
    /* Another very trivial case */
    if (i == 1)
        return l_rp;
    /* Compute array size */
    ptmp = (void *) emalloc(i * sizeof(RESULT *));
    /* Build an array with the elements to compare
       and pointers to data */
    for (j = 0, rp = l_rp->head; rp; rp = rp->next)
        ptmp[j++] = rp;
    /* Sort them */
    swish_qsort(ptmp, i, sizeof(RESULT *), &compResultsByFileNum);
    /* Build the list */
    for (j = 0, rp = NULL; j < i; j++)
    {
        if (!rp)
            l_rp->head = ptmp[j];
        else
            rp->next = ptmp[j];
        rp = ptmp[j];
    }
    rp->next = NULL;
    l_rp->tail = rp;

    /* Free the memory of the array */
    efree(ptmp);

    return l_rp;
}


/* 06/00 Jose Ruiz
** returns all results in r1 that not contains r2 
**
** On input, both result lists r1 and r2 must be sorted by filenum
** On output, the new result list remains sorted
*/
static RESULT_LIST *notresultlists(DB_RESULTS *db_results, RESULT_LIST * l_r1, RESULT_LIST * l_r2)
{
    RESULT *rp, *r1, *r2;
    RESULT_LIST *new_results_list = NULL;
    int     res = 0;

    if (!l_r1)
        return NULL;
    if (l_r1 && !l_r2)
        return l_r1;

    for (r1 = l_r1->head, r2 = l_r2->head; r1 && r2;)
    {
        res = r1->filenum - r2->filenum;
        if (res < 0)
        {
            /*
               * Storing all positions could be useful
               * in the future
             */

            rp = r1;
            r1 = r1->next;
            if(!new_results_list)
                new_results_list = newResultsList(db_results);
            addResultToList(new_results_list,rp);
        }
        else if (res > 0)
        {
            r2 = r2->next;
        }
        else
        {
            r1 = r1->next;
            r2 = r2->next;
        }
    }
    /* Add remaining results */
    while (r1)
    {
        rp = r1;
        r1 = r1->next;
        if(!new_results_list)
            new_results_list = newResultsList(db_results);
        addResultToList(new_results_list,rp);
    }

    return new_results_list;
}



/* Compare two positions as stored in posdata */
/* This routine is used by qsort */
static int     icomp_posdata(const void *s1, const void *s2)
{
    return (GET_POSITION(*(unsigned int *) s1) - GET_POSITION(*(unsigned int *) s2));
}




/* Adds a file number to a hash table of results.
** If the entry's already there, add the ranks,
** else make a new entry.
**
** Jose Ruiz 04/00
** For better performance in large "or"
** keep the lists sorted by filenum
**
** Jose Ruiz 2001/11 Rewritten to get better performance
*/
static RESULT_LIST *mergeresulthashlist(DB_RESULTS *db_results, RESULT_LIST *l_r)
{
    unsigned hashval;
    RESULT *r,
           *rp,
           *tmp,
           *next,
           *start,
           *newnode = NULL;
    RESULT_LIST *new_results_list = NULL;
    int    i,
           tot_frequency,
           pos_off,
           filenum;
    RESULTS_OBJECT *results = db_results->results;

    if(!l_r)
        return NULL;

    if(!l_r->head)
        return NULL;

    /* Init hash table */
    for (i = 0; i < BIGHASHSIZE; i++)
        results->resulthashlist[i] = NULL;

    for(r = l_r->head, next = NULL; r; r =next)
    {
        next = r->next;

        tmp = NULL;
        hashval = bignumhash(r->filenum);

        rp = results->resulthashlist[hashval];

        for(tmp = NULL; rp; )
        {
            if (r->filenum <= rp->filenum)
            {
                break;
            }
            tmp = rp;
            rp = rp->next;
        }
        if (tmp)
        {
            tmp->next = r;
        }
        else
        {
            results->resulthashlist[hashval] = r;
        }
        r->next = rp;
    }

    /* Now coalesce reptitive filenums */
    for (i = 0; i < BIGHASHSIZE; i++)
    {
        rp = results->resulthashlist[i];
        for (filenum = 0, start = NULL; ; )
        {
            if(rp)
                next = rp->next;
            if(!rp || rp->filenum != filenum)
            {
                /* Start of new block, coalesce previous results */
                if(filenum)
                {
                    int result_size;
                    
                    for(tmp = start, tot_frequency = 0; tmp!=rp; tmp = tmp->next)
                    {
                        tot_frequency += tmp->frequency;                        
                    }

                    result_size = sizeof(RESULT) + ((tot_frequency - 1) * sizeof(int));
                    newnode = (RESULT *) Mem_ZoneAlloc(results->resultSearchZone, result_size );
                    memset( newnode, 0, result_size );
                    
                    newnode->fi.filenum = newnode->filenum = filenum;
                    newnode->rank = 0;
                    newnode->tfrequency = 0;
                    newnode->frequency = tot_frequency;
                    newnode->db_results = start->db_results;

                    for(tmp = start, pos_off = 0; tmp!=rp; tmp = tmp->next)
                    {
                        newnode->rank += tmp->rank;

                        if (tmp->frequency)
                        {
                            CopyPositions(newnode->posdata, pos_off, tmp->posdata, 0, tmp->frequency);
                            pos_off += tmp->frequency;
                        }

                    }
                    /* Add at the end of new_results_list */
                    if(!new_results_list)
                    {
                        new_results_list = newResultsList(db_results);
                    }
                    addResultToList(new_results_list,newnode);
                    /* Sort positions */
                    swish_qsort(newnode->posdata,newnode->frequency,sizeof(int),&icomp_posdata);
                }
                if(rp)
                    filenum = rp->filenum;
                start = rp;
            }
            if(!rp)
                break;
            rp = next;
        }
    }

    /* Sort results by filenum  and return */
    return sortresultsbyfilenum(new_results_list);
}

