/*
$Id$
**
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**
** This program and library is free software; you can redistribute it and/or
** modify it under the terms of the GNU (Library) General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU (Library) General Public License for more details.
**
** You should have received a copy of the GNU (Library) General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**-----------------------------------------------------------------
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
#include "string.h"
#include "search.h"
#include "index.h"
#include "file.h"
#include "list.h"
#include "merge.h"
#include "hash.h"
#include "docprop.h"
#include "error.h"
#include "compress.h"
#include "metanames.h"
#include "result_sort.h"
#include "search_alt.h"
#include "db.h"
#include "swish_words.h"
#include "swish_qsort.h"

#include "proplimit.h"

#include "rank.h"


/* ------ static fucntions ----------- */
static DB_RESULTS * query_index( SEARCH_OBJECT *srch, IndexFILE *indexf );
static int isbooleanrule(char *);
static int isunaryrule(char *);
static int getrulenum(char *);
static RESULT_LIST *parseterm(SEARCH_OBJECT *srch, int parseone, int metaID, IndexFILE * indexf, struct swline **searchwordlist);
static RESULT_LIST *operate(SEARCH_OBJECT *srch, RESULT_LIST * l_rp, int rulenum, char *wordin, void *DB, int metaID, int andLevel, IndexFILE * indexf);
static RESULT_LIST *getfileinfo(SEARCH_OBJECT *srch, char *word, IndexFILE * indexf, int metaID);

static RESULT_LIST *andresultlists(SEARCH_OBJECT *srch, RESULT_LIST *, RESULT_LIST *, int);
static RESULT_LIST *orresultlists(SEARCH_OBJECT *srch, RESULT_LIST *, RESULT_LIST *);
static RESULT_LIST *notresultlist(SEARCH_OBJECT *srch, RESULT_LIST *, IndexFILE *);
static RESULT_LIST *notresultlists(SEARCH_OBJECT *srch, RESULT_LIST *, RESULT_LIST *);
static RESULT_LIST *phraseresultlists(SEARCH_OBJECT *srch, RESULT_LIST *, RESULT_LIST *, int);
static void addtoresultlist(RESULT_LIST * l_rp, int filenum, int rank, int tfrequency, int frequency, IndexFILE * indexf, SEARCH_OBJECT * srch);
static void freeresultlist(DB_RESULTS *);
static void freeresult(RESULT *);
static RESULT_LIST *mergeresulthashlist (SEARCH_OBJECT *srch, RESULT_LIST *r);
static RESULT_LIST *sortresultsbyfilenum(RESULT_LIST *r);
static void  make_db_res_and_free(RESULT_LIST *l_res);


/* Create a new search object */

SEARCH_OBJECT *New_Search_Object( SWISH *sw, char *query )
{
    SEARCH_OBJECT *srch;

    srch = (SEARCH_OBJECT *) emalloc(sizeof(SEARCH_OBJECT));
    memset( srch, 0, sizeof(SEARCH_OBJECT) );

    srch->sw = sw;  /* parent object */

    srch->params.PhraseDelimiter = PHRASE_DELIMITER_CHAR;
    srch->params.structure = IN_FILE;

    if ( query )
        set_query( srch, query );
      
    srch->resultSearchZone = Mem_ZoneCreate("resultSearch Zone", 0, 0);
    return srch;
}


void set_query(SEARCH_OBJECT *srch, char *words )
{
    
    if ( srch->params.query )
        efree( srch->params.query );

    srch->params.query = words ? estrdup( words ) : NULL;
}

static void free_db_results( SEARCH_OBJECT *srch )
{
    DB_RESULTS *next;
    DB_RESULTS *cur = srch->db_results;

    while ( cur )
    {
        next = cur->next;
        freeresultlist( cur );

        freeswline( cur->parsed_words );
        freeswline( cur->removed_stopwords );


        efree(cur);
        cur = next;
    }

    srch->db_results = NULL;
}



void Free_Search_Object( SEARCH_OBJECT *srch )
{
    if ( !srch )
        return;

    /* Free up any query parameters */

    if ( srch->params.query )
        efree( srch->params.query );

    if ( srch->params.limit_params )
        ClearLimitParameter( srch->params.limit_params );



    /* Free results from search if they exists */
    free_db_results( srch );


    Mem_ZoneFree(&srch->resultSearchZone);
    efree (srch);
}


/************************************************************
* New_db_results -- one for each index file searched
*
*************************************************************/


DB_RESULTS *New_db_results( SEARCH_OBJECT *srch, IndexFILE *indexf )
{
    DB_RESULTS * db_results = (DB_RESULTS *) emalloc(sizeof(DB_RESULTS));
    memset( db_results, 0, sizeof(DB_RESULTS));

    db_results->indexf = indexf;
    db_results->srch = srch;

    return db_results;
}


// #define DUMP_RESULTS 1


#ifdef DUMP_RESULTS

static void dump_result_lists( SEARCH_OBJECT *srch, char *message )    
{
    DB_RESULTS *db_results = srch->db_results;
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
********************************************************************************/

SEARCH_OBJECT *SwishQuery(SWISH *sw, char *words )
{
    SEARCH_OBJECT *srch = New_Search_Object( sw, words );
    
    SwishExecute( srch, NULL );
    return srch;
}




/********************************************************************************
*  SwishExecute - run a query on an existing SEARCH_OBJECT
*
*   Pass in:
*       SEARCH_OBJECT * - existing search object
*       char *  - optional query string
*
*   Notes:
*       would probably be better to return a separate "results" object
*       as then would not worry about calling this again on the same object (and needing to clean up first)
*
********************************************************************************/



int SwishExecute(SEARCH_OBJECT *srch, char *words)
{
    IndexFILE *indexf;
    int     rc = 0;
    SWISH   *sw;

    if ( !srch )
        progerr("Passed in NULL search object to SwishExecute");

    sw = srch->sw;


    /* just in case calling with same object */
    free_db_results( srch );


    /* Allow words to be passed in */
    if ( words )
        set_query( srch, words );


    /* If not words - do nothing */
    if (!srch->params.query || !*srch->params.query)
        return (sw->lasterror = NO_WORDS_IN_SEARCH);



    if ((rc = initSortResultProperties(srch->sw)))
        return rc;




    /* This returns false when no files found within the limit */
    
    if ( !Prepare_PropLookup( srch ) )
        return sw->lasterror;  /* normally RC_OK (no results), but could be an error */



    /* Fecth results for each index file */

    for ( indexf = sw->indexlist; indexf; indexf = indexf->next )
    {

        DB_RESULTS *cur_results = query_index( srch, indexf );

        /* add cur_results to the end of the list of results */

        if ( !srch->db_results)
            srch->db_results = cur_results;
        else
        {
            DB_RESULTS *db_tmp = srch->db_results;
            while (db_tmp)
            {
                if (!db_tmp->next)
                {
                    db_tmp->next = cur_results;
                    break;
                }
                db_tmp = db_tmp->next;
            }
        }
        

        /* Any big errors? */
        /* This is ugly, but allows processing all indexes before reporting an error */
        /* one could argue if this is the correct approach or not */
        
        if ( sw->lasterror )
        {
            if ( sw->lasterror == QUERY_SYNTAX_ERROR )
                return sw->lasterror;

            if ( sw->lasterror > srch->lasterror )
                srch->lasterror = sw->lasterror;

            sw->lasterror = RC_OK;
        }
    }

    /* Were all the index files empty - shouldn't happen? */

    if (!srch->total_files)
        return (sw->lasterror = INDEX_FILE_IS_EMPTY);
        


    /* Did any of the indexes have a parsed query? */

    if (!srch->search_words_found)
        return (sw->lasterror = NO_WORDS_IN_SEARCH);


    /* catch any other general errors */
    if ( sw->lasterror )
        return sw->lasterror;

   

    /* 04/00 Jose Ruiz - Sort results by rank or by properties */

    srch->total_results = sortresults(srch);



    /* If no results then return the last error, or any error found while processing index files */
    if (!srch->total_results )
        return ( sw->lasterror = sw->lasterror ? sw->lasterror : srch->lasterror );



#ifdef DUMP_RESULTS
    dump_result_lists( srch, "After sorting" );
#endif

    return srch->total_results;
}


/**************************************************************************
*  limit_result_list -- removes results that are not within the limit
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
        if ( !LimitByProperty( result->indexf, result->filenum ) )
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

static DB_RESULTS * query_index( SEARCH_OBJECT *srch, IndexFILE *indexf )
{
    struct swline *searchwordlist, *tmpswl;
    DB_RESULTS *db_results;

    
    /* Allocate memory for the result list structure for the current index file */
    db_results = New_db_results( srch, indexf );



    /* This is used to detect if all the index files were empty for error reporting */
    /* $$$ Can this every happen? */
    srch->total_files += indexf->header.totalfiles;


    /* convert the search into a parsed list */
    /* also sets db_results->(removed_stopwords|parsed_words) */

    if ( !(searchwordlist = parse_swish_query( db_results )) )
        return db_results;


    srch->search_words_found++;  /* flag that some words were found for search so can tell difference between all stop words removed vs. no words in query */        


    /* Now do the search */

    tmpswl = searchwordlist;

    db_results->resultlist = parseterm(srch, 0, 1, indexf, &searchwordlist);

    freeswline( tmpswl );


    /* Limit result list by -L parameter */
    if ( srch->params.limit_params && db_results->resultlist )
        limit_result_list( db_results );


    return db_results;
}




/* The recursive parsing function.
** This was a headache to make but ended up being surprisingly easy. :)
** parseone tells the function to only operate on one word or term.
** parseone is needed so that with metaA=foo bar "bar" is searched
** with the default metaname.
*/

static RESULT_LIST *parseterm(SEARCH_OBJECT *srch, int parseone, int metaID, IndexFILE * indexf, struct swline **searchwordlist)
{
    int     rulenum;
    char   *word;
    int     lenword;
    RESULT_LIST *l_rp,
           *new_l_rp;

    /*
     * The andLevel is used to help keep the ranking function honest
     * when it ANDs the results of the latest search term with
     * the results so far (rp).  The idea is that if you AND three
     * words together you ultimately want to resulting rank to
     * be the average of all three individual work ranks. By keeping
     * a running total of the number of terms already ANDed, the
     * next AND operation can properly scale the average-rank-so-far
     * and recompute the new average properly (see andresultlists()).
     * This implementation is a little weak in that it will not average
     * across terms that are in parenthesis. (It treats an () expression
     * as one term, and weights it as "one".)
     */
    int     andLevel = 0;       /* number of terms ANDed so far */

    /* $$$ this should be in the search object */
    LOGICAL_OP *srch_op = &srch->sw->SearchAlt->srch_op;

    word = NULL;
    lenword = 0;

    l_rp = NULL;

    rulenum = OR_RULE;
    while (*searchwordlist)
    {
        
        word = SafeStrCopy(word, (*searchwordlist)->line, &lenword);


        if (rulenum == NO_RULE)
            rulenum = srch_op->defaultrule;


        if (isunaryrule(word))  /* is it a NOT? */
        {
            *searchwordlist = (*searchwordlist)->next;
            l_rp = parseterm(srch, 1, metaID, indexf, searchwordlist);
            l_rp = notresultlist(srch, l_rp, indexf);

            /* Wild goose chase */
            rulenum = NO_RULE;
            continue;
        }


        /* If it's an operator, set the current rulenum, and continue */
        else if (isbooleanrule(word))
        {
            rulenum = getrulenum(word);
            *searchwordlist = (*searchwordlist)->next;
            continue;
        }


        /* Bump up the count of AND terms for this level */
        
        if (rulenum != AND_RULE)
            andLevel = 0;       /* reset */
        else if (rulenum == AND_RULE)
            andLevel++;



        /* Is this the start of a sub-query? */

        if (word[0] == '(')
        {

            
            /* Recurse */
            *searchwordlist = (*searchwordlist)->next;
            new_l_rp = (RESULT_LIST *) parseterm(srch, 0, metaID, indexf, searchwordlist);


            if (rulenum == AND_RULE)
                l_rp = (RESULT_LIST *) andresultlists(srch, l_rp, new_l_rp, andLevel);

            else if (rulenum == OR_RULE)
                l_rp = (RESULT_LIST *) orresultlists(srch, l_rp, new_l_rp);

            else if (rulenum == PHRASE_RULE)
                l_rp = (RESULT_LIST *) phraseresultlists(srch, l_rp, new_l_rp, 1);

            else if (rulenum == AND_NOT_RULE)
                l_rp = (RESULT_LIST *) notresultlists(srch, l_rp, new_l_rp);

            if (!*searchwordlist)
                break;

            rulenum = NO_RULE;
            continue;

        }
        else if (word[0] == ')')
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
            
            new_l_rp = (RESULT_LIST *) parseterm(srch, parseone, metaID, indexf, searchwordlist);
            if (rulenum == AND_RULE)
                l_rp = (RESULT_LIST *) andresultlists(srch, l_rp, new_l_rp, andLevel);

            else if (rulenum == OR_RULE)
                l_rp = (RESULT_LIST *) orresultlists(srch, l_rp, new_l_rp);

            else if (rulenum == PHRASE_RULE)
                l_rp = (RESULT_LIST *) phraseresultlists(srch, l_rp, new_l_rp, 1);

            else if (rulenum == AND_NOT_RULE)
                l_rp = (RESULT_LIST *) notresultlists(srch, l_rp, new_l_rp);

            if (!*searchwordlist)
                break;

            rulenum = NO_RULE;
            metaID = 1;
            continue;
        }


        /* Finally, look up a word, and merge with previous results. */

        l_rp = (RESULT_LIST *) operate(srch, l_rp, rulenum, word, indexf->DB, metaID, andLevel, indexf);

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

static RESULT_LIST *operate(SEARCH_OBJECT *srch, RESULT_LIST * l_rp, int rulenum, char *wordin, void *DB, int metaID, int andLevel, IndexFILE * indexf)
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
    new_l_rp = getfileinfo(srch, word, indexf, metaID);

    switch (rulenum)
    {
        case AND_RULE:
            return_l_rp = andresultlists(srch, l_rp, new_l_rp, andLevel);
            break;


        case OR_RULE:
            return_l_rp = orresultlists(srch, l_rp, new_l_rp);
            break;

        case NOT_RULE:
            return_l_rp = notresultlist(srch, new_l_rp, indexf);
            break;

        case PHRASE_RULE:
            return_l_rp = phraseresultlists(srch, l_rp, new_l_rp, 1);
            break;

        case AND_NOT_RULE:
            return_l_rp = notresultlists(srch, l_rp, new_l_rp);
            break;
    }


    efree(word);
    return return_l_rp;
}


static RESULT_LIST *newResultsList(SEARCH_OBJECT *srch)
{
    RESULT_LIST *result_list = (RESULT_LIST *)Mem_ZoneAlloc(srch->resultSearchZone, sizeof(RESULT_LIST));
    memset( result_list, 0, sizeof( RESULT_LIST ) );

    result_list->srch = srch;
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
static int test_structure(int structure, int frequency, int *posdata)
{
    int i, j;    /* i -> counter upto frequency, j -> new frequency */
    int *p,*q;   /* Use pointers to ints instead of arrays for
                 ** faster proccess */
    
    for(i = j = 0, p = q = posdata; i < frequency; i++, p++)
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

static RESULT_LIST *getfileinfo(SEARCH_OBJECT *srch, char *word, IndexFILE * indexf, int metaID)
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
    RESULT_LIST *l_rp, *l_rp2;
    long    wordID;
    unsigned long  nextposmetaname;
    char   *p;
    int     tfrequency = 0;
    unsigned char   *s, *buffer; 
    int     sz_buffer;
    unsigned char flag;
    int     stack_posdata[MAX_POSDATA_STACK];  /* stack buffer for posdata */
    int    *posdata;
    SWISH  *sw = srch->sw;
    int     structure = srch->params.structure;

    x = j = filenum = frequency = len = curmetaID = index_structure = index_structfreq = 0;
    nextposmetaname = 0L;


    l_rp = l_rp2 = NULL;


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




    DB_InitReadWords(sw, indexf->DB);
    if (!p)    /* No wildcard -> Direct hash search */
    {
        DB_ReadWordHash(sw, word, &wordID, indexf->DB);

        if(!wordID)
        {    
            DB_EndReadWords(sw, indexf->DB);
            sw->lasterror = WORD_NOT_FOUND;
            return NULL;
        }
    }        

    else  /* There is a star. So use the sequential approach */
    {       
        char   *resultword;

        if (*word == '*')
        {
            sw->lasterror = UNIQUE_WILDCARD_NOT_ALLOWED_IN_WORD;
            return NULL;
        }

        
        DB_ReadFirstWordInvertedIndex(sw, word, &resultword, &wordID, indexf->DB);

        if (!wordID)
        {
            DB_EndReadWords(sw, indexf->DB);
            sw->lasterror = WORD_NOT_FOUND;
            return NULL;
        }
        efree(resultword);   /* Do not need it */
    }


    /* If code is here we have found the word !! */

    do
    {
        DB_ReadWordData(sw, wordID, &buffer, &sz_buffer, indexf->DB);
        s = buffer;

        // buffer structure = <tfreq><metaID><delta to next meta>

        /* Get the data of the word */
        tfrequency = uncompress2(&s); /* tfrequency - number of files with this word */


        /* Now look for a correct Metaname */
        curmetaID = uncompress2(&s);

        while (curmetaID)
        {
            nextposmetaname = UNPACKLONG2(s);
            s += sizeof(long);

            
            if (curmetaID >= metaID)
                break;

            s = buffer + nextposmetaname;
            if(nextposmetaname == (unsigned long)sz_buffer)
                break; // if no more meta data

            curmetaID = uncompress2(&s);
        }

        if (curmetaID == metaID) /* found a matching meta value */
        {
            filenum = 0;
            do
            {
                /* Read on all items */
                uncompress_location_values(&s,&flag,&tmpval,&frequency);
                filenum += tmpval;  

                /* stack_posdata is just to avoid calling emalloc */
                /* it should be enough for most cases */
                if(frequency > MAX_POSDATA_STACK)
                    posdata = (int *)emalloc(frequency * sizeof(int));
                else
                    posdata = stack_posdata;

                /* read positions */
                uncompress_location_positions(&s,flag,frequency,posdata);

                /* test structure and adjust frequency */
                frequency = test_structure(structure, frequency, posdata);

                /* Store -1 in rank - In this way, we can delay its computation */
                /* This stuff has been removed */

                /* Store result */
                if(frequency)
                {
                    /* This is very useful if we sorted by other property */
                    if(!l_rp)
                       l_rp = newResultsList(srch);

                    // tfrequency = number of files with this word
                    // frequency = number of times this words is in this document for this metaID

                    addtoresultlist(l_rp, filenum, -1, tfrequency, frequency, indexf, srch);

                    /* Copy positions */
                    memcpy((unsigned char *)l_rp->tail->posdata,(unsigned char *)posdata,frequency * sizeof(int));

                    // Temp fix
                    {
                        RESULT *r1 = l_rp->tail;
                        r1->rank = getrank( sw, r1->frequency, r1->tfrequency, r1->posdata, r1->indexf, r1->filenum );
                    }
                }
                if(posdata != stack_posdata)
                    efree(posdata);
                    

            } while ((unsigned long)(s - buffer) != nextposmetaname);


        }

        efree(buffer);


        if (!p)
            break;              /* direct access (no wild card) -> break */

        else
        {
            char   *resultword;

            /* Jump to next word */
            /* No more data for this word but we
               are in sequential search because of
               the star (p is not null) */
            /* So, go for next word */
            DB_ReadNextWordInvertedIndex(sw, word, &resultword, &wordID, indexf->DB);
            if (! wordID)
                break;          /* no more data */

            efree(resultword);  /* Do not need it (although might be useful for highlighting some day) */
        }

    } while(1);   /* continue on in loop for wildcard search */



    if (p)
    {
        /* Finally, if we are in an sequential search merge all results */
        l_rp = mergeresulthashlist(srch, l_rp);
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
    if (!strcmp(word, AND_WORD) || !strcmp(word, OR_WORD) || !strcmp(word, PHRASE_WORD) || !strcmp(word, AND_NOT_WORD))
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






/* Takes two lists of results from searches and ANDs them together.
** On input, both result lists r1 and r2 must be sorted by filenum
** On output, the new result list remains sorted
*/

static RESULT_LIST *andresultlists(SEARCH_OBJECT * srch, RESULT_LIST * l_r1, RESULT_LIST * l_r2, int andLevel)
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
            SWISH  *sw = srch->sw;


            if(r1->rank == -1)
                r1->rank = getrank( sw, r1->frequency, r1->tfrequency, r1->posdata, r1->indexf, r1->filenum );

            if(r2->rank == -1)
                r2->rank = getrank( sw, r2->frequency, r1->tfrequency, r2->posdata, r2->indexf, r2->filenum );

            newRank = ((r1->rank * andLevel) + r2->rank) / (andLevel + 1);
            

            if(!new_results_list)
                new_results_list = newResultsList(srch);

            addtoresultlist(new_results_list, r1->filenum, newRank, 0, r1->frequency + r2->frequency, r1->indexf, srch);


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


static RESULT_LIST *orresultlists(SEARCH_OBJECT * srch, RESULT_LIST * l_r1, RESULT_LIST * l_r2)
{
    int     rc;
    RESULT *r1;
    RESULT *r2;
    RESULT *rp,
           *tmp;
    RESULT_LIST *new_results_list = NULL;


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
            SWISH  *sw = srch->sw;
            
            /* Compute rank if not yet computed */
            if(r1->rank == -1)
                r1->rank = getrank( sw, r1->frequency, r1->tfrequency, r1->posdata, r1->indexf, r1->filenum );

            if(r2->rank == -1)
                r2->rank = getrank( sw, r2->frequency, r2->tfrequency, r2->posdata, r2->indexf, r2->filenum );


            /* Create a new RESULT - Should be a function to creeate this, I'd think */

            result_size = sizeof(RESULT) + ( (r1->frequency + r2->frequency - 1) * sizeof(int) );
            rp = (RESULT *) Mem_ZoneAlloc(srch->resultSearchZone, result_size );
            memset( rp, 0, result_size );


            rp->fi.filenum = rp->filenum = r1->filenum;

            rp->rank = ( r1->rank + r2->rank) * 2;  // bump up the or terms
            rp->tfrequency = 0;
            rp->frequency = r1->frequency + r2->frequency;
            rp->indexf = r1->indexf;

            
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
            new_results_list = newResultsList(srch);

        addResultToList(new_results_list,rp);
    }


    /* Add the remaining results */

    tmp = r1 ? r1 : r2;

    while(tmp)
    {
        rp = tmp;
        tmp = tmp->next;
        if(!new_results_list)
            new_results_list = newResultsList(srch);

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

static void    marknum(SEARCH_OBJECT *srch, struct markentry **markentrylist, int num)
{
    unsigned hashval;
    struct markentry *mp;

    mp = (struct markentry *) Mem_ZoneAlloc(srch->resultSearchZone, sizeof(struct markentry));

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

static RESULT_LIST *notresultlist(SEARCH_OBJECT *srch, RESULT_LIST * l_rp, IndexFILE * indexf)
{
    int     i,
            filenums;
    RESULT *rp;
    RESULT_LIST *new_results_list = NULL;
    struct markentry *markentrylist[BIGHASHSIZE];

    if(!l_rp)
        rp = NULL;
    else
        rp = l_rp->head;

    initmarkentrylist(markentrylist);
    while (rp != NULL)
    {
        marknum(srch, markentrylist, rp->filenum);
        rp = rp->next;
    }

    filenums = indexf->header.totalfiles;

    for (i = 1; i <= filenums; i++)
    {
        if (!ismarked(markentrylist, i))
        {
            if(!new_results_list)
                new_results_list = newResultsList(srch);

            addtoresultlist(new_results_list, i, 1000, 0, 0, indexf, srch);
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
static RESULT_LIST *phraseresultlists(SEARCH_OBJECT * srch, RESULT_LIST * l_r1, RESULT_LIST * l_r2, int distance)
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
        return NULL;

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
                SWISH  *sw = srch->sw;
                
                /* Compute newrank */
                if(r1->rank == -1)
                    r1->rank = getrank( sw, r1->frequency, r1->tfrequency, r1->posdata, r1->indexf, r1->filenum );
                if(r2->rank == -1)
                    r2->rank = getrank( sw, r2->frequency, r1->tfrequency, r2->posdata, r2->indexf, r2->filenum );

                newRank = (r1->rank + r2->rank) / 2;
                /*
                   * Storing positions is neccesary for further
                   * operations 
                 */
                if(!new_results_list)
                    new_results_list = newResultsList(srch);
                
                addtoresultlist(new_results_list, r1->filenum, newRank, 0, found, r1->indexf, srch);

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


static void addtoresultlist(RESULT_LIST * l_rp, int filenum, int rank, int tfrequency, int frequency, IndexFILE * indexf, SEARCH_OBJECT * srch)
{
    RESULT *newnode;
    int     result_size;

    result_size = sizeof(RESULT) + ((frequency - 1) * sizeof(int));
    newnode = (RESULT *) Mem_ZoneAlloc( srch->resultSearchZone, result_size );
    memset( newnode, 0, result_size );
    
    newnode->fi.filenum = newnode->filenum = filenum;

    newnode->rank = rank;
    newnode->tfrequency = tfrequency;
    newnode->frequency = frequency;
    newnode->indexf = indexf;

    addResultToList(l_rp, newnode);
}

/***************************************************************************
* SwishSeekResult -- seeks to the result number specified
*
*   Returns the position or a negative number on error
*
*   
*
****************************************************************************/


int     SwishSeekResult(SEARCH_OBJECT *srch, int pos)
{
    int    i;
    RESULT *cur_result = NULL;

    if (!srch)
        return (srch->sw->lasterror = INVALID_SWISH_HANDLE);

    if ( !srch->db_results )
    {
        set_progerr(SWISH_LISTRESULTS_EOF, srch->sw, "Attempted to SwishSeekResult before searching");
        return SWISH_LISTRESULTS_EOF;
    }



    /* Check if only one index file -> Faster SwishSeek */

    if (!srch->db_results->next)
    {
        for (i = 0, cur_result = srch->db_results->sortresultlist; cur_result && i < pos; i++)
            cur_result = cur_result->next;

        srch->db_results->currentresult = cur_result;
        


    } else {
        /* Well, we finally have more than one index file */
        /* In this case we have no choice - We need to read the data from disk */
        /* The easy way: Let SwishNext do the job */

        /* Must reset the currentresult pointers first */
        /* $$$ could keep the current result seek pos number in srch, and then just offset from there if greater */
        DB_RESULTS *db_results;

        for ( db_results = srch->db_results; db_results; db_results = db_results->next )
            db_results->currentresult = db_results->sortresultlist;

        /* If want first one then we are done */
        if ( 0 == pos )
            return pos;
        

        for (i = 0; i < pos; i++)
            if (!(cur_result = SwishNextResult(srch)))
                break;
    }

    if (!cur_result)
        return ((srch->sw->lasterror = SWISH_LISTRESULTS_EOF));

    return pos;
}





RESULT *SwishNextResult(SEARCH_OBJECT * srch)
{
    RESULT *res = NULL;
    RESULT *res2 = NULL;
    int     rc;
    DB_RESULTS *db_results = NULL;
    DB_RESULTS *db_results_winner = NULL;
    int  num;
    SWISH   *sw = srch->sw;
    

    if (srch->bigrank)
        num = 10000000 / srch->bigrank;
    else
        num = 10000;


    /* Seems like we should error here if there are no results */
    if ( !srch->db_results )
    {
        set_progerr(SWISH_LISTRESULTS_EOF, sw, "Attempted to read results before searching");
        return NULL;
    }

    

    /* Check for a unique index file */
    if (!srch->db_results->next)
    {
        if ((res = srch->db_results->currentresult))
        {
            /* Increase Pointer */
            srch->db_results->currentresult = res->next;
            
            /* If rank was delayed, compute it now */
            if(res->rank == -1)
                res->rank = getrank( sw, res->frequency, res->tfrequency, res->posdata, res->indexf, res->filenum );
        }
    }


    else    /* tape merge to find the next one from all the index files */
    
    {
        /* We have more than one index file - can't use pre-sorted index */
        /* Get the lower value */
        db_results_winner = srch->db_results;

        if ((res = db_results_winner->currentresult))
        {
            /* If rank was delayed, compute it now */
            if(res->rank == -1)
                res->rank = getrank( sw, res->frequency, res->tfrequency, res->posdata, res->indexf, res->filenum );

            if ( !res->PropSort )
                res->PropSort = getResultSortProperties(sw, res);
        }


        for (db_results = srch->db_results->next; db_results; db_results = db_results->next)
        {
            /* Any more results for this index? */
            if (!(res2 = db_results->currentresult))
                continue;

            /* If rank was delayed, compute it now */

            if(res2->rank == -1)
               res2->rank = getrank( sw, res2->frequency, res2->tfrequency, res2->posdata, res2->indexf, res2->filenum );


            /* Load the sort properties for this results */

            if ( !res2->PropSort )
                res2->PropSort = getResultSortProperties(sw, res2);

                
            /* Compare the properties */
            
            if (!res)  /* first one doesn't exist, so second wins */
            {
                res = res2;
                db_results_winner = db_results;
                continue;
            }

            rc = (int) compResultsByNonSortedProps(&res, &res2);
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



    /* Normalize rank */
    if (res)
    {
        res->rank = (int) (res->rank * num)/10000;
        if (res->rank >= 999)
            res->rank = 1000;
        else if (res->rank < 1)
            res->rank = 1;
    }
    else
    {
        // it's expected to just return null on end of list.
        // sw->lasterror = SWISH_LISTRESULTS_EOF;  
    }

        
    return res;

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
    int     i;
    SWISH  *sw = rp->indexf->sw;

    if (!rp)
        return;

    freefileinfo( &rp->fi );  // may have already been freed
        
    if (sw->ResultSort->numPropertiesToSort && rp->PropSort)
        for (i = 0; i < sw->ResultSort->numPropertiesToSort; i++)
            efree(rp->PropSort[i]);
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
static RESULT_LIST *notresultlists(SEARCH_OBJECT * srch, RESULT_LIST * l_r1, RESULT_LIST * l_r2)
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
                new_results_list = newResultsList(srch);
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
            new_results_list = newResultsList(srch);
        addResultToList(new_results_list,rp);
    }

    return new_results_list;
}



/* Compare two positions as stored in posdata */
/* This routine is used by qsort */
static int     icomp_posdata(const void *s1, const void *s2)
{
    return (GET_POSITION(*(int *) s1) - GET_POSITION(*(int *) s2));
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
static RESULT_LIST *mergeresulthashlist(SEARCH_OBJECT *srch, RESULT_LIST *l_r)
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
    SWISH  *sw = srch->sw;

    if(!l_r)
        return NULL;

    if(!l_r->head)
        return NULL;

    /* Init hash table */
    for (i = 0; i < BIGHASHSIZE; i++)
        srch->resulthashlist[i] = NULL;

    for(r = l_r->head, next = NULL; r; r =next)
    {
        next = r->next;

        tmp = NULL;
        hashval = bignumhash(r->filenum);

        rp = srch->resulthashlist[hashval];

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
            srch->resulthashlist[hashval] = r;
        }
        r->next = rp;
    }

    /* Now coalesce reptitive filenums */
    for (i = 0; i < BIGHASHSIZE; i++)
    {
        rp = srch->resulthashlist[i];
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
                    newnode = (RESULT *) Mem_ZoneAlloc(srch->resultSearchZone, result_size );
                    memset( newnode, 0, result_size );
                    
                    newnode->fi.filenum = newnode->filenum = filenum;
                    newnode->rank = 0;
                    newnode->tfrequency = 0;
                    newnode->frequency = tot_frequency;
                    newnode->indexf = start->indexf;

                    for(tmp = start, pos_off = 0; tmp!=rp; tmp = tmp->next)
                    {
                        /* Compute rank if not yet computed */
                        if(tmp->rank == -1)
                            tmp->rank = getrank( sw, tmp->frequency, tmp->tfrequency, tmp->posdata, tmp->indexf, tmp->filenum );

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
                        new_results_list = newResultsList(srch);
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

