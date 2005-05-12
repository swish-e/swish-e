/*
**
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
    
** Mon May  9 18:19:34 CDT 2005
** added GPL



**
**
**  Sept 2002 - isolate the search and results into more separate "objects".
**              Still misses the mark.  -L stores the lookup tabels in indexf instead
**              of in a "search" object.
*/


/* #define DEBUG_RANK 1 */

#ifndef __HasSeenModule_Search
#define __HasSeenModule_Search 1

#ifdef __cplusplus
extern "C" {
#endif


/*
   -- module data
*/

/* -------- Search Object Structures ------------ */


/* This holds the -L parameters */

typedef struct s_LIMIT_PARAMS LIMIT_PARAMS;
typedef struct s_RESULT RESULT;
typedef struct s_SEARCH_OBJECT SEARCH_OBJECT;
typedef struct s_RESULTS_OBJECT RESULTS_OBJECT;
typedef struct s_DB_RESULTS DB_RESULTS;


/* These are the input parameters */

struct s_LIMIT_PARAMS
{
    LIMIT_PARAMS     *next;
    unsigned char    *propname;
    unsigned char    *lowrange;
    unsigned char    *highrange;
};

/* These are the processed parameters ready for searching */


typedef struct
{
    unsigned char   *inPropRange;  /* indexed by file number -- should be a vector to save room, but what is fastest?  int? */
    propEntry       *loPropRange;
    propEntry       *hiPropRange;
} PROP_LIMITS;



struct s_SEARCH_OBJECT
{
    SWISH          *sw;                /* Parent object */
    char           *query;             /* Query string */
    int             PhraseDelimiter;   /* Phrase delimiter char */
    int             structure;         /* Structure for limiting to HTML tags */
    struct swline  *sort_params;       /* List of sort parameter strings */

    int             limits_prepared;   /* Flag that the parameters have been prepared */
    LIMIT_PARAMS   *limit_params;      /* linked list of -L limit settings */
    PROP_LIMITS   **prop_limits;       /* flags to detect if file should be limited -L for each index, and for each metaname*/
};


    /* == Results Structures == */ 



/* A single result */

struct s_RESULT
{
    RESULT     *next;
    DB_RESULTS *db_results;     /* parent object */

//    int         count;          /* result Entry-Counter */
    int         filenum;        /* there's an extra four bytes we don't need */
    FileRec     fi;             /* This is used to cache the properties and the seek index */
    int         rank;
    int         frequency;
    int         tfrequency;     /* Total frequency of result OR result index */
                                /* during result sorting tfrequency is used as an index number */
    unsigned int         posdata[1];     /* used for phrase searches */
};




/* This handles a list of results for a single index file */
/* This is probably not needed since results are always sorted might as well just have a pointer to the first result */
/* no real need to add results to the tail */

typedef struct RESULT_LIST
{
    RESULT *head;
    RESULT *tail;
    RESULTS_OBJECT *results;
    // DB_RESULTS *db_results;  /* parent object */
}
RESULT_LIST;


typedef struct
{
    int              direction;  /* -1 for asc and 1 for desc */
    propEntry        **key;      /* pointer to an array of PropEntry's indexed by result */
    struct metaEntry *property;  /* pointer to the metaEntry for this key - need for sorting propEntry */
    int              is_rank_sort; /* flag for faster sorting by rank */
} SortData;

/* Structure to hold all results per index */

struct s_DB_RESULTS
{
    DB_RESULTS   *next;

    RESULTS_OBJECT *results;            /* parent */
    SEARCH_OBJECT  *srch;               /* make life easy (only valid during search) */


    IndexFILE   *indexf;                /* the associated index file */
    int          index_num;             /* index into params indexed by index number */

    RESULT_LIST *resultlist;            /* pointer to list of results (indirectly) */
    RESULT      *sortresultlist;        /* linked list of RESULTs in sort order (actually just points to resultlist->head) */
    RESULT      *currentresult;         /* pointer to the current seek position */

    struct swline *parsed_words;        /* parsed search query */
    struct swline *removed_stopwords;   /* stopwords that were removed from the query */

    int          num_sort_props;        /* number of sort properties */
    SortData     *sort_data;            /* an array num_sort_props of SortData */

    char         **prop_string_cache;   /* place to cache a result's string properties  $$$ I think this may be a mistake */

    int          result_count;          /* number of results in set */


};

struct s_RESULTS_OBJECT
{
    SWISH          *sw;                 /* parent */
    char           *query;              /* in case user forgot what they searched for */

    void           *ref_count_ptr;      /* for SWISH::API */

    DB_RESULTS     *db_results;         /* Linked list of results - one for each index file */

    int             cur_rec_number;     /* current record number in list */
    
    int             total_results;      /* total number of results */
    int             total_files;        /* total number of files in all combined indexes */
    int             search_words_found; /* flag that some search words were found in some index after parsing -- for error message */
    int             lasterror;          /* used to save errors while processing more than one index file */
    int             bigrank;            /* Largest rank found, for scaling */
    int             rank_scale_factor;  /* for scaling each results rank when fetching with SwishNextResult */
    MEM_ZONE       *resultSearchZone;   /* pool for allocating results */

    MEM_ZONE       *resultSortZone;     /* pool for allocating sort keys for each result */

    RESULT         *resulthashlist[BIGHASHSIZE];    /* Hash array for merging results */

};

void SwishRankScheme( SWISH *sw, int scheme );	/* set ranking scheme */

SEARCH_OBJECT *New_Search_Object( SWISH *sw, char *query );
void SwishSetStructure( SEARCH_OBJECT *srch, int structure );
void SwishPhraseDelimiter( SEARCH_OBJECT *srch, char delimiter );
void SwishSetSort( SEARCH_OBJECT *srch, char *sort );
void SwishSetQuery( SEARCH_OBJECT *srch, char *query );
void Free_Search_Object( SEARCH_OBJECT *srch );


RESULTS_OBJECT *SwishQuery(SWISH *sw, char *words );
RESULTS_OBJECT *SwishExecute(SEARCH_OBJECT *srch, char *words);

int SwishHits( RESULTS_OBJECT *results );


void Free_Results_Object( RESULTS_OBJECT *results );

RESULT *SwishNextResult(RESULTS_OBJECT *results);
int     SwishSeekResult(RESULTS_OBJECT *results, int pos);
int isMetaNameOpNext(struct swline *);

#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* __HasSeenModule_Search */

