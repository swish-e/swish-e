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
#include "stemmer.h"
#include "soundex.h"
#include "error.h"
#include "compress.h"
/* removed stuff 
#include "deflate.h"
*/
#include "metanames.h"
#include "result_sort.h"
#include "result_output.h"
#include "search_alt.h"
#include "db.h"
#include "swish_words.h"
#include "swish_qsort.h"

#include "proplimit.h"

#include "rank.h"


/*** ??? $$$ fix this ****/
#define TOTAL_WORDS_FIX 1;
/* 
  -- init structures for this module
*/

void initModule_Search (SWISH  *sw)

{
   struct MOD_Search *srch;

   srch = (struct MOD_Search *) emalloc(sizeof(struct MOD_Search));
   sw->Search = srch;

   
       /* Default variables for search */
   srch->maxhits = -1;
   srch->beginhits = 0;

   srch->currentMaxPropertiesToDisplay = 0;
   srch->numPropertiesToDisplay = 0;

   srch->db_results = NULL;

   srch->propNameToDisplay =NULL;

   srch->PhraseDelimiter=PHRASE_DELIMITER_CHAR;

   srch->bigrank = 0;

   srch->resultSearchZone = Mem_ZoneCreate("resultSearch Zone", 0, 0);

   return;
}

/* Resets memory of vars used by ResultSortt properties configuration */
void resetModule_Search (SWISH *sw)

{
 struct DB_RESULTS *tmp,*tmp2;
 struct MOD_Search *srch = sw->Search;
 int i;
 IndexFILE *tmpindexlist;

       /* Default variables for search */
   srch->maxhits = -1;
   srch->beginhits = 0;

        /* Free results from search if they exists */
   for(tmp=srch->db_results;tmp;)
   {
      freeresultlist(sw,tmp);
      tmp2=tmp->next;
      efree(tmp);
      tmp=tmp2;
   }
   Mem_ZoneReset(srch->resultSearchZone);


        /* Free display props arrays */
        /* First the common part to all the index files */
   if (srch->propNameToDisplay)
   {
      for(i=0;i<srch->numPropertiesToDisplay;i++)
         efree(srch->propNameToDisplay[i]);
      efree(srch->propNameToDisplay);
   }
   srch->propNameToDisplay=NULL;
   srch->numPropertiesToDisplay=0;
   srch->currentMaxPropertiesToDisplay=0;
                /* Now the IDs of each index file */
   for(tmpindexlist=sw->indexlist;tmpindexlist;tmpindexlist=tmpindexlist->next)
   {
      if (tmpindexlist->propIDToDisplay)
         efree(tmpindexlist->propIDToDisplay);
      tmpindexlist->propIDToDisplay=NULL;
   }
}

/* 
  -- release all wired memory for this module
*/

void freeModule_Search (SWISH *sw)

{
 struct MOD_Search *srch = sw->Search;

  /* free module data */
  Mem_ZoneFree(&srch->resultSearchZone);

  efree (srch);
  sw->Search = NULL;

  return;
}



/*
** ----------------------------------------------
** 
**  Module config code starts here
**
** ----------------------------------------------
*/


/*
 -- Config Directives
 -- Configuration directives for this Module
 -- return: 0/1 = none/config applied
*/

int configModule_Search (SWISH *sw, StringList *sl)

{
  //struct MOD_Search *srch = sw->Search;
  //char *w0    = sl->word[0];
  int  retval = 0;

  return retval;
}


/* Compare two integers */
/* This routine is used by qsort */
int     icomp(const void *s1, const void *s2)
{
    return (*(int *) s1 - *(int *) s2);
}

/* Compare two positions as stored in posdata */
/* This routine is used by qsort */
int     icomp_posdata(const void *s1, const void *s2)
{
    return (GET_POSITION(*(int *) s1) - GET_POSITION(*(int *) s2));
}

/* 01/2001 Jose Ruiz */
/* Compare RESULTS using RANK */
/* This routine is used by qsort */
int     compResultsByFileNum(const void *s1, const void *s2)
{
    return ((*(RESULT * const *) s1)->filenum - (*(RESULT * const *) s2)->filenum);
}

/* 04/00 Jose Ruiz */
/* Simple routing for comparing pointers to integers in order to
get an ascending sort with qsort */
/* Identical to previous one but use two integers per array */
int     icomp2(const void *s1, const void *s2)
{
    int     rc,
           *p1,
           *p2;

    rc = (*(int *) s1 - *(int *) s2);
    if (rc)
        return (rc);
    else
    {
        p1 = (int *) s1;
        p2 = (int *) s2;
        return (*(++p1) - *(++p2));
    }
}

int     SwishAttach(SWISH * sw, int printflag)
{
    struct MOD_Search *srch = sw->Search;
    IndexFILE *indexlist;

    IndexFILE *tmplist;
 
    indexlist = sw->indexlist;
    sw->TotalWords = 0;
    sw->TotalFiles = 0;


    /* First of all . Read header default values from all index fileis */
    /* With this, we read wordchars, stripchars, ... */
    for (tmplist = indexlist; tmplist;)
    {
        sw->commonerror = RC_OK;
        srch->bigrank = 0;

        /* Program exits in DB_Open if it fails */
        tmplist->DB = (void *)DB_Open(sw, tmplist->line);

        read_header(sw, &tmplist->header, tmplist->DB);


        sw->TotalWords += tmplist->header.totalwords;
        sw->TotalFiles += tmplist->header.totalfiles;
        tmplist = tmplist->next;
    }

    /* Make lookuptables for char processing */
    return (sw->lasterror = RC_OK);
}


/*
  -- Search Swish 
  -- Check if AltaVista/Lycos/Web.de like search string has to be converted
  -- and call swish search...
  -- 2001-03-02 rasc
*/

int     search(SWISH * sw, char *words, int structure)
{
    char   *sw_srch_str;
    int     ret;


    if (sw->SearchAlt->enableAltSearchSyntax)
    {                           /* AltaVista like search enabled? */
        sw_srch_str = convAltSearch2SwishStr(words);
        ret = search_2(sw, sw_srch_str, structure);
        efree(sw_srch_str);
    }
    else
    {
        ret = search_2(sw, words, structure);
    }
    return ret;
}


static void limit_result_list( SWISH *sw )
{
    RESULT *rtmp;
    RESULT *rp;
    RESULT *last;
    struct DB_RESULTS *db_results = sw->Search->db_results;

        
    /* Process each index */
    while ( db_results )
    {
        if(db_results->resultlist)
            rp = db_results->resultlist->head;
        else
            rp = NULL;

        last = NULL;

        while (rp)
        {
            rtmp = rp->next;
            
            if ( LimitByProperty( sw, rp->indexf, rp->filenum ) )
            {
                freeresult( sw, rp );

                if ( !last )  /* if first in list */
                    db_results->resultlist->head = rtmp;
                else
                    last->next = rtmp;
            }
            else
                last = rp;  /* move the last pointer to current one */

            rp = rtmp;                
        }
                    
        db_results = db_results->next;
        
    }
}




/********************************************************************************
*  Returns a negative number on error, zero or positive = number of results
*
********************************************************************************/

int     search_2(SWISH * sw, char *words, int structure)
{
    int     j,
            k,
            hassearch,
            indexYes,
            totalResults;
    struct swline *searchwordlist,
           *tmplist2;
    IndexFILE *indexlist;
    int     rc = 0;
    unsigned char PhraseDelimiter;
    char   *tmpwords;
    struct DB_RESULTS *db_results,
           *db_tmp;
    struct MOD_Search *srch = sw->Search;


    /* If not words - do nothing */
    if (!words || !*words)
        return (sw->lasterror = NO_WORDS_IN_SEARCH);

    PhraseDelimiter = (unsigned char) srch->PhraseDelimiter;

    indexlist = sw->indexlist;
    sw->Search->db_results = NULL;

    
    j = 0;
    searchwordlist = NULL;
    indexYes = 0;
    totalResults = 0;
    hassearch = 0;

    sw->lasterror = RC_OK;
    sw->commonerror = RC_OK;

    if ((rc = initSearchResultProperties(sw)))
        return rc;
    if ((rc = initSortResultProperties(sw)))
        return rc;


    /* This returns false when no files found within the limit */
    if ( !Prepare_PropLookup( sw ) )
        return sw->lasterror;  /* normally RC_OK (no results) */

    while (indexlist != NULL)
    {
        /* moved these inside the loop Nov 24,2001.  Need to verify */
        sw->lasterror = RC_OK;
        sw->commonerror = RC_OK;

        tmpwords = estrdup(words); /* copy of the string  (2001-03-13 rasc) */



        /* Clean up from previous loop */
        if (searchwordlist)
        {
            freeswline(searchwordlist);
            searchwordlist = NULL;
        }



        /* tokenize the query into swish words based on this current index */

        if (!(searchwordlist = tokenize_query_string(sw, tmpwords, &indexlist->header)))
        {
            indexlist = indexlist->next;
            efree(tmpwords);
            if ( sw->lasterror )
                return sw->lasterror;
            continue;
        }


        hassearch = 1;  // Flag that we found some words to search form

        srch->bigrank = 0;


        /* Is there an open index? */
        if (!indexlist->DB)
        {
            efree(tmpwords);
            if (searchwordlist)
                freeswline(searchwordlist); /* 2001-03-13 rasc */
            return sw->lasterror;
        }


        /* Any files in the index? */

        if (!indexlist->header.totalfiles)
        {
            indexlist = indexlist->next;
            efree(tmpwords);
            continue;
        }
        else
        {
            indexYes = 1;       /*There is a non-empty index */
        }


        resultHeaderOut(sw, 2, "#\n# Index File: %s\n", indexlist->line);

        searchwordlist = (struct swline *) ignore_words_in_query(sw, indexlist, searchwordlist, PhraseDelimiter);
        if ( sw->lasterror )
        {
            if (searchwordlist)
                freeswline(searchwordlist);
            return sw->lasterror;
        }


        /* Echo index file, fixed search, stopwords */

        /* Result Header Output  (2001-03-14 rasc,  rewritten) */

        resultPrintHeader(sw, 2, &indexlist->header, indexlist->header.savedasheader, 0);


        resultHeaderOut(sw, 3, "# StopWords:");
        for (k = 0; k < indexlist->header.stopPos; k++)
            resultHeaderOut(sw, 3, " %s", indexlist->header.stopList[k]);

        resultHeaderOut(sw, 3, "\n");



        printheaderbuzzwords(sw,indexlist);


        resultHeaderOut(sw, 2, "# Search Words: %s\n", words);

        tmplist2 = searchwordlist;

        if ( !tmplist2 )
            sw->commonerror++;  // Flag at the query (with this index) did no have any search words
        else
        {
            resultHeaderOut(sw, 2, "# Parsed Words: ");

            while (tmplist2)
            {
                resultHeaderOut(sw, 2, "%s ", tmplist2->line);
                tmplist2 = tmplist2->next;
            }
        
            resultHeaderOut(sw, 2, "\n");
        }


        /* Expand phrase search: "kim harlow" becomes (kim PHRASE_WORD harlow) */
        searchwordlist = (struct swline *) expandphrase(searchwordlist, PhraseDelimiter);
        searchwordlist = (struct swline *) fixmetanames(searchwordlist);
        searchwordlist = (struct swline *) fixnot1(searchwordlist);
        searchwordlist = (struct swline *) fixnot2(searchwordlist);

        /* Allocate memory for the result list structure */
        db_results = (struct DB_RESULTS *) emalloc(sizeof(struct DB_RESULTS));
        memset( db_results, 0, sizeof(struct DB_RESULTS));


        /* Now do the search */

        if (searchwordlist)
        {
            int metaID = 1;  /* Default meta ID to search */
            
            tmplist2 = searchwordlist;
            db_results->resultlist = (RESULT_LIST *) parseterm(sw, 0, metaID, indexlist, &tmplist2);
        }


        /* add db_results to the list of results */

        if ( !sw->Search->db_results)
            sw->Search->db_results = db_results;
        else
        {
            db_tmp = sw->Search->db_results;
            while (db_tmp)
            {
                if (!db_tmp->next)
                {
                    db_tmp->next = db_results;
                    break;
                }
                db_tmp = db_tmp->next;
            }

        }


        efree(tmpwords);
        indexlist = indexlist->next;

    }  /* end process each index file */


    /* Did any of the indexes have a query? */

    if (!hassearch)
        return (sw->lasterror = NO_WORDS_IN_SEARCH);



    if (searchwordlist)
    {
        freeswline(searchwordlist);
        searchwordlist = NULL;
    }


    /* Limit result list by -L parameter */
    // note -- this used to be in addtoresultlist, but that was checking every word
    // placing here means that it only checks for each file, instead of each word.

    if ( is_prop_limit_used( sw ) )
        limit_result_list( sw );



#ifdef DUMP_RESULTS
    {
        struct DB_RESULTS *db_results = sw->Search->db_results;

        while ( db_results )
        {
            RESULT *result;
            printf("Looking at index\n");
            if ( !db_results->resultlist )
            {
                printf("no resultlist\n");
                continue;
            }
            result = db_results->resultlist->head;

            if ( !result )
            {
                printf("resultlist, but head is null\n");
                continue;
            }
            while ( result )
            {
                printf("Result: filenum '%d' from index file '%s'\n", result->filenum, result->indexf->line );
                result = result->next;
            }

            printf("end of results for index\n");

            db_results = db_results->next;
        }
        printf("end of results\n");
    }
#endif

    /* 04/00 Jose Ruiz - Sort results by rank or by properties */

    // ??? Currently this does -t limits.  Wrong place, if you ask me.
    totalResults = sortresults(sw, structure);


    if (!totalResults && sw->commonerror)
        return (sw->lasterror = WORDS_TOO_COMMON);

    if (!totalResults && !indexYes)
        return (sw->lasterror = INDEX_FILE_IS_EMPTY);

    return totalResults;
}

/* 2001-09 jmruiz - Rewriting
** This puts parentheses in the right places around meta searches
** to avoid problems whith them. Basically "metaname = bla" 
** becomes "(metanames = bla)" */

struct swline *fixmetanames(struct swline *sp)
{
    int     metapar;
    struct swline *tmpp,
           *newp;

    tmpp = sp;
    newp = NULL;

    /* Fix metanames with parenthesys eg: metaname = bla => (metanames = bla) */
    while (tmpp != NULL)
    {
        if (isMetaNameOpNext(tmpp->next))
        {
            /* If it is a metaName add the name and = and skip to next */
            newp = (struct swline *) addswline(newp, "(");
            newp = (struct swline *) addswline(newp, tmpp->line);
            newp = (struct swline *) addswline(newp, "=");
            tmpp = tmpp->next;
            tmpp = tmpp->next;
            if ( !tmpp )
                return NULL;
            
            /* 06/00 Jose Ruiz
               ** Fix to consider parenthesys in the
               ** content of a MetaName */
            if (tmpp->line[0] == '(')
            {
                metapar = 1;
                newp = (struct swline *) addswline(newp, tmpp->line);
                tmpp = tmpp->next;
                while (metapar && tmpp)
                {
                    if (tmpp->line[0] == '(')
                        metapar++;
                    else if (tmpp->line[0] == ')')
                        metapar--;
                    newp = (struct swline *) addswline(newp, tmpp->line);
                    if (metapar)
                        tmpp = tmpp->next;
                }
                if (!tmpp)
                    return (newp);
            }
            else
                newp = (struct swline *) addswline(newp, tmpp->line);
            newp = (struct swline *) addswline(newp, ")");
        }
        else 
            newp = (struct swline *) addswline(newp, tmpp->line);
        /* next one */
        tmpp = tmpp->next;
    }

    freeswline(sp);
    return newp;
}

/* 2001 -09 jmruiz Rewritten 
** This optimizes some NOT operator to be faster.
**
** "word1 not word" is changed by "word1 and_not word2"
**
** In the old way the previous query was...
**    get results if word1
**    get results of word2
**    not results of word2 (If we have 100000 docs and word2 is in
**                          just 3 docs, this means read 99997
**                          results)
**    intersect both list of results
**
** The "new way"
**    get results if word1
**    get results of word2
**    intersect (and_not_rule) both lists of results
** 
*/

struct swline *fixnot1(struct swline *sp)
{
    struct swline *tmpp,
           *prev;

    if (!sp)
        return NULL;
    /* 06/00 Jose Ruiz - Check if first word is NOT_RULE */
    /* Change remaining NOT by AND_NOT_RULE */
    for (tmpp = sp, prev = NULL; tmpp; prev = tmpp, tmpp = tmpp->next)
    {
        if (tmpp->line[0] == '(')
            continue;
        else if (isnotrule(tmpp->line))
        {
            if(prev && prev->line[0]!='=' && prev->line[0]!='(')
            {
                efree(tmpp->line);
                tmpp->line = estrdup(AND_NOT_WORD);
            }
        }
    }
    return sp;
}

/* 2001 -09 jmruiz - Totally new - Fix the meta=(not ahsg) bug
** Add parentheses to avoid the way operator NOT confuse complex queries */
struct swline *fixnot2(struct swline *sp)
{
    int     openparen, found;
    struct swline *tmpp, *newp;
    char    *magic = "<__not__>";  /* magic avoids parsing the
                                   ** "not" operator twice
                                   ** and put the code in an 
                                   ** endless loop */                            

    found = 1;
    while(found)
    {
        openparen = 0;
        found = 0;

        for (tmpp = sp , newp = NULL; tmpp ; tmpp = tmpp->next)
        {
            if (isnotrule(tmpp->line))
            {
                found = 1;
                /* Add parentheses */
                newp = (struct swline *) addswline(newp, "(");
                /* Change "NOT" by magic to avoid find it in next iteration */
                newp = (struct swline *) addswline(newp, magic);
                for(tmpp = tmpp->next; tmpp; tmpp = tmpp->next)
                {
                    if ((tmpp->line)[0] == '(')
                        openparen++;
                    else if(!openparen)
                    {
                        newp = (struct swline *) addswline(newp, tmpp->line);
                        /* Add parentheses */
                        newp = (struct swline *) addswline(newp, ")");
                        break;
                    }
                    else if ((tmpp->line)[0] == ')')
                        openparen--;
                    newp = (struct swline *) addswline(newp, tmpp->line);
                }
                if(!tmpp)
                    break;
            }
            else
                newp = (struct swline *) addswline(newp, tmpp->line);
        }
        freeswline(sp);
        sp = newp;
    }

    /* remove magic and put the "real" NOT in place */
    for(tmpp = newp; tmpp ; tmpp = tmpp->next)
    {
        if(!strcmp(tmpp->line,magic))
        {
            efree(tmpp->line);
            tmpp->line = estrdup(NOT_WORD);
        }
    }

    return newp;
}

/* expandstar removed - Jose Ruiz 04/00 */

/* Expands phrase search. Berkeley University becomes Berkeley PHRASE_WORD University */
/* It also fixes the and, not or problem when they appeared inside a phrase */
struct swline *expandphrase(struct swline *sp, char delimiter)
{
    struct swline *tmp,
           *newp;
    int     inphrase;

    if (!sp)
        return NULL;
    inphrase = 0;
    newp = NULL;
    tmp = sp;
    while (tmp != NULL)
    {
        if ((tmp->line)[0] == delimiter)
        {
            if (inphrase)
            {
                inphrase = 0;
                newp = (struct swline *) addswline(newp, ")");
            }
            else
            {
                inphrase++;
                newp = (struct swline *) addswline(newp, "(");
            }
        }
        else
        {
            if (inphrase)
            {
                if (inphrase > 1)
                    newp = (struct swline *) addswline(newp, PHRASE_WORD);
                inphrase++;
                newp = (struct swline *) addswline(newp, tmp->line);
            }
            else
            {
                char *operator;

                if ( ( operator = isBooleanOperatorWord( tmp->line )) )
                    newp = (struct swline *) addswline(newp, operator);
                else
                    newp = (struct swline *) addswline(newp, tmp->line);
            }
        }
        tmp = tmp->next;
    }
    freeswline(sp);
    return newp;
}



/* Print the buzzwords */
void    printheaderbuzzwords(SWISH *sw, IndexFILE * indexf)
{
    int     hashval;
    struct swline *sp = NULL;

    resultHeaderOut(sw, 3, "# BuzzWords:");

    for (hashval = 0; hashval < HASHSIZE; hashval++)
    {
        sp = indexf->header.hashbuzzwordlist[hashval];
        while (sp != NULL)
        {
            resultHeaderOut(sw, 3, " %s", sp->line);
            sp = sp->next;
        }
    }
    resultHeaderOut(sw, 3, "\n");
}






/* The recursive parsing function.
** This was a headache to make but ended up being surprisingly easy. :)
** parseone tells the function to only operate on one word or term.
** parseone is needed so that with metaA=foo bar "bar" is searched
** with the default metaname.
*/

RESULT_LIST *parseterm(SWISH * sw, int parseone, int metaID, IndexFILE * indexf, struct swline **searchwordlist)
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

    word = NULL;
    lenword = 0;

    l_rp = NULL;

    rulenum = OR_RULE;
    while (*searchwordlist)
    {
        
        word = SafeStrCopy(word, (*searchwordlist)->line, &lenword);


        if (rulenum == NO_RULE)
            rulenum = sw->SearchAlt->srch_op.defaultrule;


        if (isunaryrule(word))  /* is it a NOT? */
        {
            *searchwordlist = (*searchwordlist)->next;
            l_rp = (RESULT_LIST *) parseterm(sw, 1, metaID, indexf, searchwordlist);
            l_rp = (RESULT_LIST *) notresultlist(sw, l_rp, indexf);

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
            new_l_rp = (RESULT_LIST *) parseterm(sw, 0, metaID, indexf, searchwordlist);


            if (rulenum == AND_RULE)
                l_rp = (RESULT_LIST *) andresultlists(sw, l_rp, new_l_rp, andLevel);

            else if (rulenum == OR_RULE)
                l_rp = (RESULT_LIST *) orresultlists(sw, l_rp, new_l_rp);

            else if (rulenum == PHRASE_RULE)
                l_rp = (RESULT_LIST *) phraseresultlists(sw, l_rp, new_l_rp, 1);

            else if (rulenum == AND_NOT_RULE)
                l_rp = (RESULT_LIST *) notresultlists(sw, l_rp, new_l_rp);

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
            
            new_l_rp = (RESULT_LIST *) parseterm(sw, parseone, metaID, indexf, searchwordlist);
            if (rulenum == AND_RULE)
                l_rp = (RESULT_LIST *) andresultlists(sw, l_rp, new_l_rp, andLevel);

            else if (rulenum == OR_RULE)
                l_rp = (RESULT_LIST *) orresultlists(sw, l_rp, new_l_rp);

            else if (rulenum == PHRASE_RULE)
                l_rp = (RESULT_LIST *) phraseresultlists(sw, l_rp, new_l_rp, 1);

            else if (rulenum == AND_NOT_RULE)
                l_rp = (RESULT_LIST *) notresultlists(sw, l_rp, new_l_rp);

            if (!*searchwordlist)
                break;

            rulenum = NO_RULE;
            metaID = 1;
            continue;
        }


        /* Finally, look up a word, and merge with previous results. */

        l_rp = (RESULT_LIST *) operate(sw, l_rp, rulenum, word, indexf->DB, metaID, andLevel, indexf);

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

RESULT_LIST *operate(SWISH * sw, RESULT_LIST * l_rp, int rulenum, char *wordin, void *DB, int metaID, int andLevel, IndexFILE * indexf)
{
    RESULT_LIST *new_l_rp,
           *return_l_rp;
    char   *word;
    int     lenword;

    word = estrdup(wordin);
    lenword = strlen(word);

    new_l_rp = return_l_rp = NULL;


    /* ??? Stopwords have already been removed at this point. */
    /*****
    if (isstopword(&indexf->header, word) && !isrule(word))
    {
        if (rulenum == OR_RULE && l_rp != NULL)
            return l_rp;
        else
            sw->commonerror = 1;
    }
    ******/


    /* ??? Ever a time when rulenum is not one of these? */

    if (rulenum == AND_RULE)
    {
        new_l_rp = (RESULT_LIST *) getfileinfo(sw, word, indexf, metaID);
        return_l_rp = (RESULT_LIST *) andresultlists(sw, l_rp, new_l_rp, andLevel);
    }


    else if (rulenum == OR_RULE)
    {
        new_l_rp = (RESULT_LIST *) getfileinfo(sw, word, indexf, metaID);
        return_l_rp = (RESULT_LIST *) orresultlists(sw, l_rp, new_l_rp);
    }


    else if (rulenum == NOT_RULE)
    {
        new_l_rp = (RESULT_LIST *) getfileinfo(sw, word, indexf, metaID);
        return_l_rp = (RESULT_LIST *) notresultlist(sw, new_l_rp, indexf);
    }


    else if (rulenum == PHRASE_RULE)
    {
        new_l_rp = (RESULT_LIST *) getfileinfo(sw, word, indexf, metaID);
        return_l_rp = (RESULT_LIST *) phraseresultlists(sw, l_rp, new_l_rp, 1);
    }


    else if (rulenum == AND_NOT_RULE)
    {
        new_l_rp = (RESULT_LIST *) getfileinfo(sw, word, indexf, metaID);
        return_l_rp = (RESULT_LIST *) notresultlists(sw, l_rp, new_l_rp);
    }


    efree(word);
    return return_l_rp;
}


static RESULT_LIST *newResultsList(SWISH *sw)
{
    RESULT_LIST *l = (RESULT_LIST *)Mem_ZoneAlloc(sw->Search->resultSearchZone, sizeof(RESULT_LIST));
    l->head = l->tail = NULL;
    l->sw = (struct SWISH *)sw;

    return l;
}

void addResultToList(RESULT_LIST *l_r, RESULT *r)
{
    r->next = NULL;

    r->reslist = l_r;

    if(!l_r->head)
        l_r->head = r;
    if(l_r->tail)
        l_r->tail->next = r;
    l_r->tail = r;

}



/* Finds a word and returns its corresponding file and rank information list.
** If not found, NULL is returned.
*/
/* Jose Ruiz
** New implmentation based on Hashing for direct access. Faster!!
** Also solves stars. Faster!! It can even found "and", "or"
** when looking for "an*" or "o*" if they are not stop words
*/
RESULT_LIST *getfileinfo(SWISH * sw, char *word, IndexFILE * indexf, int metaID)
{
    int     j,
            x,
            filenum,
            frequency,
            found,
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

        if (curmetaID == metaID)
            found = 1;
        else
            found = 0;

        // ??? why not just check curmetaID == metaID here
            
        if (found) /* found a matching meta value */
        {
            filenum = 0;
            do
            {
                /* Read on all items */
                uncompress_location_values(&s,&flag,&tmpval,&frequency);
                filenum += tmpval;  

                /* Store -1 in rank - In this way, we can delay its computation */
                /* This is very useful if we sorted by other property */
                if(!l_rp)
                    l_rp = newResultsList(sw);

                // tfrequency = number of files with this word
                // frequency = number of times this words is in this document for this metaID

                addtoresultlist(l_rp, filenum, -1, tfrequency, frequency, indexf, sw);

                // Temp fix
                {
                    struct RESULT *r1 = l_rp->tail;
                    r1->rank =  r1->rank = getrank( sw, r1->frequency, r1->tfrequency, r1->posdata, r1->indexf, r1->filenum );
                }

                    
                uncompress_location_positions(&s,flag,frequency,l_rp->tail->posdata);

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
        l_rp = mergeresulthashlist(sw, l_rp);
    }

    DB_EndReadWords(sw, indexf->DB);
    return l_rp;
}


/*
  -- Rules checking
  -- u_is...  = user rules (and, or, ...)
  -- is...    = internal rules checking
 */


/* Is a word a rule?
*/

int     isrule(char *word)
{
    int     i;
    static char *ops[] = {      /* internal ops... */
        AND_WORD, OR_WORD, NOT_WORD, PHRASE_WORD, AND_NOT_WORD,
        NULL
    };

    for (i = 0; ops[i]; i++)
    {
        if (!strcmp(word, ops[i]))
            return 1;
    }
    return 0;
}

int     u_isrule(SWISH * sw, char *word)
{
    LOGICAL_OP *op;

    op = &(sw->SearchAlt->srch_op);
    if (!strcmp(word, op->and) || !strcmp(word, op->or) || !strcmp(word, op->not))
        return 1;
    else
        return 0;
}

int     isnotrule(char *word)
{
    if (!strcmp(word, NOT_WORD))
        return 1;
    else
        return 0;
}

int     u_isnotrule(SWISH * sw, char *word)
{
    if (!strcmp(word, sw->SearchAlt->srch_op.not))
        return 1;
    else
        return 0;
}


/* Is a word a boolean rule?
*/

int     isbooleanrule(char *word)
{
    if (!strcmp(word, AND_WORD) || !strcmp(word, OR_WORD) || !strcmp(word, PHRASE_WORD) || !strcmp(word, AND_NOT_WORD))
        return 1;
    else
        return 0;
}

/* Is a word a unary rule?
*/

int     isunaryrule(char *word)
{
    if (!strcmp(word, NOT_WORD))
        return 1;
    else
        return 0;
}

/* Return the number for a rule.
*/

int     getrulenum(char *word)
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


/* 
  -- Return selected RuleNumber for default rule.
  -- defined via current Swish Search Boolean OP Settings for user.  
*/

int     u_SelectDefaultRulenum(SWISH * sw, char *word)
{
    if (!strcasecmp(word, sw->SearchAlt->srch_op.and))
        return AND_RULE;
    else if (!strcasecmp(word, sw->SearchAlt->srch_op.or))
        return OR_RULE;
    return NO_RULE;
}



static void  make_db_res_and_free(SWISH *sw,RESULT_LIST *l_res) {
   struct DB_RESULTS tmp;
   memset (&tmp,0,sizeof(struct DB_RESULTS));
   tmp.resultlist = l_res;
   freeresultlist(sw,&tmp);
}



/* Takes two lists of results from searches and ANDs them together.
** On input, both result lists r1 and r2 must be sorted by filenum
** On output, the new result list remains sorted
*/

RESULT_LIST *andresultlists(SWISH * sw, RESULT_LIST * l_r1, RESULT_LIST * l_r2, int andLevel)
{
    RESULT_LIST *new_results_list = NULL;
    RESULT *r1;
    RESULT *r2;
    int     res = 0;

    /* patch provided by Mukund Srinivasan */
    if (l_r1 == NULL || l_r2 == NULL)
    {
        make_db_res_and_free(sw,l_r1);
        make_db_res_and_free(sw,l_r2);
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

            if(r1->rank == -1)
                r1->rank = getrank( sw, r1->frequency, r1->tfrequency, r1->posdata, r1->indexf, r1->filenum );

            if(r2->rank == -1)
                r2->rank = getrank( sw, r2->frequency, r1->tfrequency, r2->posdata, r2->indexf, r2->filenum );

            newRank = ((r1->rank * andLevel) + r2->rank) / (andLevel + 1);
            

            if(!new_results_list)
                new_results_list = newResultsList(sw);

            addtoresultlist(new_results_list, r1->filenum, newRank, 0, r1->frequency + r2->frequency, r1->indexf, sw);


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


RESULT_LIST *orresultlists(SWISH * sw, RESULT_LIST * l_r1, RESULT_LIST * l_r2)
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
            /* Compute rank if not yet computed */
            if(r1->rank == -1)
                r1->rank = getrank( sw, r1->frequency, r1->tfrequency, r1->posdata, r1->indexf, r1->filenum );

            if(r2->rank == -1)
                r2->rank = getrank( sw, r2->frequency, r2->tfrequency, r2->posdata, r2->indexf, r2->filenum );


            /* Create a new RESULT - Should be a function to creeate this, I'd think */
            
            rp = (RESULT *) Mem_ZoneAlloc(sw->Search->resultSearchZone, sizeof(RESULT) + (r1->frequency + r2->frequency) * sizeof(int));
            memset( rp, 0, sizeof(RESULT));
            rp->fi.filenum = rp->filenum = r1->filenum;

            rp->rank = r1->rank + r2->rank;
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
            new_results_list = newResultsList(sw);

        addResultToList(new_results_list,rp);
    }


    /* Add the remaining results */

    tmp = r1 ? r1 : r2;

    while(tmp)
    {
        rp = tmp;
        tmp = tmp->next;
        if(!new_results_list)
            new_results_list = newResultsList(sw);

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

void    marknum(SWISH *sw, struct markentry **markentrylist, int num)
{
    unsigned hashval;
    struct markentry *mp;

    mp = (struct markentry *) Mem_ZoneAlloc(sw->Search->resultSearchZone, sizeof(struct markentry));

    mp->num = num;

    hashval = bignumhash(num);
    mp->next = markentrylist[hashval];
    markentrylist[hashval] = mp;
}


/* Has a number been printed?
*/

int     ismarked(struct markentry **markentrylist, int num)
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

void    initmarkentrylist(struct markentry **markentrylist)
{
    int     i;

    for (i = 0; i < BIGHASHSIZE; i++)
        markentrylist[i] = NULL;
}

void    freemarkentrylist(struct markentry **markentrylist)
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

RESULT_LIST *notresultlist(SWISH * sw, RESULT_LIST * l_rp, IndexFILE * indexf)
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
        marknum(sw, markentrylist, rp->filenum);
        rp = rp->next;
    }

    filenums = indexf->header.totalfiles;

    for (i = 1; i <= filenums; i++)
    {
        if (!ismarked(markentrylist, i))
        {
            if(!new_results_list)
                new_results_list = newResultsList(sw);
            addtoresultlist(new_results_list, i, 1000, 0, 0, indexf, sw);
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
RESULT_LIST *phraseresultlists(SWISH * sw, RESULT_LIST * l_r1, RESULT_LIST * l_r2, int distance)
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
                    new_results_list = newResultsList(sw);
                
                addtoresultlist(new_results_list, r1->filenum, newRank, 0, found, r1->indexf, sw);

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


void addtoresultlist(RESULT_LIST * l_rp, int filenum, int rank, int tfrequency, int frequency, IndexFILE * indexf, SWISH * sw)
{
    RESULT *newnode;


    newnode = (RESULT *) Mem_ZoneAlloc(sw->Search->resultSearchZone, sizeof(RESULT) + frequency * sizeof(int));
    memset( newnode, 0, sizeof(RESULT));
    newnode->fi.filenum = newnode->filenum = filenum;

    newnode->rank = rank;
    newnode->tfrequency = tfrequency;
    newnode->frequency = frequency;
    newnode->indexf = indexf;

    addResultToList(l_rp, newnode);
}




RESULT *SwishNext(SWISH * sw)
{
    struct MOD_Search *srch = sw->Search;
    RESULT *res = NULL;
    RESULT *res2 = NULL;
    int     rc;
    struct DB_RESULTS *db_results = NULL,
           *db_results_winner = NULL;
    int  num;

    if (srch->bigrank)
        num = 10000000 / srch->bigrank;
    else
        num = 10000;

    /* Check for a unique index file */
    if (!sw->Search->db_results->next)
    {
        if ((res = sw->Search->db_results->currentresult))
        {
            /* Increase Pointer */
            sw->Search->db_results->currentresult = res->next;
            /* If rank was delayed, compute it now */
            if(res->rank == -1)
                res->rank = getrank( sw, res->frequency, res->tfrequency, res->posdata, res->indexf, res->filenum );

        }
    }


    else    /* tape merge to find the next one from all the index files */
    
    {
        /* We have more than one index file - can't use pre-sorted index */
        /* Get the lower value */
        db_results_winner = sw->Search->db_results;

        if ((res = db_results_winner->currentresult))
        {
            /* If rank was delayed, compute it now */
            if(res->rank == -1)
                res->rank = getrank( sw, res->frequency, res->tfrequency, res->posdata, res->indexf, res->filenum );

            res->PropSort = getResultSortProperties(sw, res);
        }


        for (db_results = sw->Search->db_results->next; db_results; db_results = db_results->next)
        {
            if (!(res2 = db_results->currentresult))
                continue;
            else
            {
                            /* If rank was delayed, compute it now */
                if(res2->rank == -1)
                   res2->rank = getrank( sw, res2->frequency, res2->tfrequency, res2->posdata, res2->indexf, res2->filenum );

                res2->PropSort = getResultSortProperties(sw, res2);
            }

            if (!res)
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
        sw->lasterror = SWISH_LISTRESULTS_EOF;
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

/* funtion to free all memory of a list of results */
void    freeresultlist(SWISH * sw, struct DB_RESULTS *dbres)
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
        freeresult(sw, rp);
        rp = tmp;
    }
    dbres->resultlist = NULL;
    dbres->currentresult = NULL;
    dbres->sortresultlist = NULL;
}

/* funtion to free the memory of one result */
void    freeresult(SWISH * sw, RESULT * rp)
{
    int     i;

    if (rp)
    {
        if (sw->ResultSort->numPropertiesToSort && rp->PropSort)
        {
            for (i = 0; i < sw->ResultSort->numPropertiesToSort; i++)
                efree(rp->PropSort[i]);

/* For better performance with thousands of results this is stored in a MemZone
   in result_sort.c module
            efree(rp->PropSort);
*/

        }
/* For better performance with thousands of results this is stored in a MemZone
   in result_sort.c module

        if (sw->ResultSort->numPropertiesToSort && rp->iPropSort)
        {
            efree(rp->iPropSort);
        }
*/
/* For better performance store results in a MemZone
 
        efree(rp);
*/
    }
}



/* 
06/00 Jose Ruiz - Sort results by filenum
Uses an array and qsort for better performance
Used for faster "and" and "phrase" of results
*/
RESULT_LIST *sortresultsbyfilenum(RESULT_LIST * l_rp)
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
RESULT_LIST *notresultlists(SWISH * sw, RESULT_LIST * l_r1, RESULT_LIST * l_r2)
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
                new_results_list = newResultsList(sw);
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
            new_results_list = newResultsList(sw);
        addResultToList(new_results_list,rp);
    }

    return new_results_list;
}


/******************************************************************************
*   Remove the stop words from the tokenized query
*   rewritten Nov 24, 2001 - moseley
*   Still horrible!  Need a real parse tree.
*******************************************************************************/


struct swline *ignore_words_in_query(SWISH * sw, IndexFILE * indexf, struct swline *searchwordlist, unsigned char phrase_delimiter)
{
    struct swline  *cur_token = searchwordlist;
    struct swline  *prev_token = NULL;
    struct swline  *prev_prev_token = NULL;  // for removing two things
    int             in_phrase = 0;
    int             word_count = 0; /* number of search words found */
    int             paren_count = 0;
    struct MOD_Search *srch = sw->Search;

    srch->removed_stopwords = 0;

    

    while ( cur_token )
    {
        int remove = 0;
        char first_char = cur_token->line[0];

        if ( cur_token == searchwordlist )
        {
            prev_token = prev_prev_token = NULL;
            word_count = 0;
            paren_count = 0;
            in_phrase = 0;
        }

        while ( 1 )  // so we can use break.
        {

            /* Can't backslash here -- (because this code should really be include in swish_words.c) */
            
            if ( first_char == phrase_delimiter )
            {
                in_phrase = !in_phrase;

                if ( !in_phrase && prev_token && prev_token->line[0] == phrase_delimiter )
                    remove = 2;
                    
                break;
            }


            /* leave everything alone inside a pharse */
            
            if ( in_phrase )
            {
                if (isstopword(&indexf->header, cur_token->line))
                {
                    srch->removed_stopwords++;
                    resultHeaderOut(sw, 1, "# Removed stopword: %s\n", cur_token->line);
                    remove = 1;
                }
                else
                    word_count++;

                break;                    
            }
            

            /* Allow operators */

            if ( first_char == '=' )
                break;
            
            if ( first_char == '(' )
            {
                paren_count++;
                break;
            }

            if ( first_char == ')' )
            {
                paren_count--;

                if ( prev_token && prev_token->line[0] == '(' )
                    remove = 2;

                break;
            }
                


            /* Allow all metanames */
            
            if ( isMetaNameOpNext(cur_token->next) )
                break;



            /* Look for AND OR NOT - remove AND OR at start, and remove second of doubles */

            if ( u_isrule(sw, cur_token->line)  )
            {
                if ( prev_token )
                {
                    /* remove double tokens */
                    if ( u_isrule( sw, prev_token->line ) )
                        remove = 1;
                }
                /* allow NOT at the start */
                else if ( !u_isnotrule(sw, cur_token->line) )
                    remove = 1;

                break;
            }
                    


            /* Finally, is it a stop word? */

            if ( isstopword(&indexf->header, cur_token->line) )
            {
                srch->removed_stopwords++;
                resultHeaderOut(sw, 1, "# Removed stopword: %s\n", cur_token->line);
                remove = 1;
            }
            else
                word_count++;
            

            
            break;
        }


        /* Catch dangling metanames */
        if ( !remove && !cur_token->next && isMetaNameOpNext( cur_token ) )
            remove = 2;


        if ( remove )
        {
            struct swline *tmp = cur_token;
                
            if ( cur_token == searchwordlist )      // we are removing first token
                searchwordlist = cur_token->next;
            else
            {
                prev_token->next = cur_token->next; // remove one in the middle
                cur_token = prev_token;  // save if remove == 2
            }


            efree( tmp->line );
            efree( tmp );

            if ( remove == 2 )
            {
                tmp = cur_token;

                if ( cur_token == searchwordlist )      // we are removing first token
                    searchwordlist = cur_token->next;
                else
                    prev_prev_token->next = cur_token->next; // remove one in the middle
                
                efree( tmp->line );
                efree( tmp );
            }


            /* start at the beginning again */

            cur_token = searchwordlist;
            continue;
        }
        
        if ( prev_token )
            prev_prev_token = prev_token;
            
        prev_token = cur_token;
        cur_token = cur_token->next;
    }


    if ( in_phrase || paren_count )
        sw->lasterror = QUERY_SYNTAX_ERROR;

    else if ( !word_count )
        sw->lasterror = srch->removed_stopwords ? WORDS_TOO_COMMON : NO_WORDS_IN_SEARCH;

    return searchwordlist;

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
RESULT_LIST *mergeresulthashlist(SWISH *sw, RESULT_LIST *l_r)
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

    if(!l_r)
        return NULL;

    if(!l_r->head)
        return NULL;

    /* Init hash table */
    for (i = 0; i < BIGHASHSIZE; i++)
        sw->Search->resulthashlist[i] = NULL;

    for(r = l_r->head, next = NULL; r; r =next)
    {
        next = r->next;

        tmp = NULL;
        hashval = bignumhash(r->filenum);

        rp = sw->Search->resulthashlist[hashval];
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
            sw->Search->resulthashlist[hashval] = r;
        }
        r->next = rp;
    }

    /* Now coalesce reptitive filenums */
    for (i = 0; i < BIGHASHSIZE; i++)
    {
        rp = sw->Search->resulthashlist[i];
        for (filenum = 0, start = NULL; ; )
        {
            if(rp)
                next = rp->next;
            if(!rp || rp->filenum != filenum)
            {
                /* Start of new block, coalesce previous results */
                if(filenum)
                {
                    for(tmp = start, tot_frequency = 0; tmp!=rp; tmp = tmp->next)
                    {
                        tot_frequency += tmp->frequency;                        
                    }
                    newnode = (RESULT *) Mem_ZoneAlloc(sw->Search->resultSearchZone, sizeof(RESULT) + tot_frequency * sizeof(int));
                    memset( newnode, 0, sizeof(RESULT));
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
                        new_results_list = newResultsList(sw);
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

