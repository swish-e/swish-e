/*
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
**-------------------------------------------------------
** Added getMetaName & isMetaName 
** to support METADATA
** G. Hill 3/18/97 ghill@library.berkeley.edu
*/


#ifndef __HasSeenModule_Search
#define __HasSeenModule_Search 1

BEGIN_C_DECLS


/*
   -- module data
*/

/* -------- Search Object Structures ------------ */


/* This holds the -L parameters */

typedef struct s_LIMIT_PARAMS LIMIT_PARAMS;
typedef struct s_RESULT RESULT;
typedef struct s_SEARCH_OBJECT SEARCH_OBJECT;
typedef struct s_DB_RESULTS DB_RESULTS;



struct s_LIMIT_PARAMS
{
    LIMIT_PARAMS     *next;
    unsigned char    *propname;
    unsigned char    *lowrange;
    unsigned char    *highrange;
};



/* This defines the input parameters for a query */

typedef struct SEARCH_PARAMS
{
    char           *query;             /* Query string */
    int             PhraseDelimiter;   /* Phrase delimiter char */
    int             structure;         /* Structure for limiting to HTML tags */
    LIMIT_PARAMS   *limit_params;      /* linked list of -L limit settings */


} SEARCH_PARAMS;



/* This handles a list of results for a single index file */
/* This is probably not needed since results are always sorted might as well just have a pointer to the first result */
/* no real need to add results to the tail */

typedef struct RESULT_LIST
{
    RESULT *head;
    RESULT *tail;
    SEARCH_OBJECT *srch;
    // DB_RESULTS *db_results;  /* parent object */
}
RESULT_LIST;



/* A single result */

struct s_RESULT
{
    RESULT *next;

    int     count;              /* result Entry-Counter */
    int     filenum;            /* there's an extra four bytes we don't need */
    FileRec fi;                 /* This is used to cache the properties and the seek index */
    int     rank;
    int     frequency;
    int     tfrequency;         /* Total frequency of result */

    /* file position where this document's properties are stored */
    char  **PropSort;
    int    *iPropSort;          /* Used for presorted data */
    IndexFILE *indexf;          /* needed to get metaname info (but could use reslist)  */

    int     posdata[1];
};


/* Structure to hold all results per index */

struct s_DB_RESULTS
{
    DB_RESULTS   *next;
    SEARCH_OBJECT *srch;           

    IndexFILE   *indexf;           /* the associated index file */
    RESULT_LIST *resultlist;       /* pointer to list of results (indirectly) */
    RESULT      *sortresultlist;   /* linked list of RESULTs in sort order (actually just points to resultlist->head) */
    RESULT      *currentresult;    /* pointer to the current seek position */
    struct swline *parsed_words;   /* parsed search query */
    struct swline *removed_stopwords; /* stopwords that were removed from the query */
};


/* This is a main search object */

struct s_SEARCH_OBJECT
{
    SWISH          *sw;                 /* Parent object */
    SEARCH_PARAMS   params;             /* Search Parameters */
    DB_RESULTS     *db_results;         /* Linked list of results groupped by index file */

    int             total_results;      /* total number of results */
    int             total_files;        /* total number of files in all combined indexes */
    int             search_words_found; /* flag that some search words were found in some index after parsing -- for error message */
    int             lasterror;          /* used to save errors while processing more than one index file */
    int             bigrank;            /* Largest rank found, for scaling */
    MEM_ZONE       *resultSearchZone;   /* pool for allocating results */

    RESULT         *resulthashlist[BIGHASHSIZE];    /* Hash array for merging results */
};

SEARCH_OBJECT *New_Search_Object( SWISH *sw, char *query );
void Free_Search_Object( SEARCH_OBJECT *srch );


SEARCH_OBJECT *SwishQuery(SWISH *sw, char *words );
int SwishExecute(SEARCH_OBJECT *srch, char *words);
RESULT *SwishNextResult(SEARCH_OBJECT *srch);
int     SwishSeekResult(SEARCH_OBJECT *srch, int pos);
void set_query(SEARCH_OBJECT *srch, char *words );
int isMetaNameOpNext(struct swline *);

END_C_DECLS


#endif /* __HasSeenModule_Search */

