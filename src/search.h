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
#define __HasSeenModule_Search       1


/*
   -- module data
*/

struct MOD_Search
{
        /* start of hits */
    int     beginhits;
        /* max. number of hits */
    int     maxhits;

        /* Properties vars */
    int     numPropertiesToDisplay;
    int     currentMaxPropertiesToDisplay;
    char  **propNameToDisplay;


    /* All Results per index file */
    struct DB_RESULTS *db_results;

    /* Hash array for merging results */
    RESULT *resulthashlist[BIGHASHSIZE];

    int bigrank;

    /* Phrase delimiter char */
    int     PhraseDelimiter;

    /* Number of stopwords removed from query */
    int     removed_stopwords; 

    MEM_ZONE *resultSearchZone;
};

void initModule_Search (SWISH *);
void freeModule_Search (SWISH *);
void resetModule_Search (SWISH *);
int  configModule_Search (SWISH *, StringList *);


int SwishAttach(SWISH *, int);
int search (SWISH *sw, char *str, int structure);
int search_2 (SWISH *sw, char *words, int structure);
RESULT *SwishNext(SWISH *);

struct swline *fixmetanames(struct swline *);
struct swline *fixnot1(struct swline *);
struct swline *fixnot2(struct swline *);
struct swline *expandphrase(struct swline *, char);

void printheaderbuzzwords(SWISH *sw, IndexFILE * indexf);

RESULT_LIST *parseterm(SWISH *, int, int, IndexFILE *, struct swline **);
RESULT_LIST *operate(SWISH *, RESULT_LIST *, int, char *, void *, int, int, IndexFILE *);
RESULT_LIST *getfileinfo(SWISH *, char *, IndexFILE *, int);

int isrule(char *);
int isnotrule(char *);
int isbooleanrule(char *);
int isunaryrule(char *);
int isMetaNameOpNext(struct swline *);
int getrulenum(char *);
int u_isnotrule(SWISH *sw, char *word);
int u_isrule(SWISH *sw, char *word);
int u_SelectDefaultRulenum(SWISH *sw, char *word);


RESULT_LIST *andresultlists(SWISH *, RESULT_LIST *, RESULT_LIST *, int);
RESULT_LIST *orresultlists(SWISH *, RESULT_LIST *, RESULT_LIST *);
RESULT_LIST *notresultlist(SWISH *, RESULT_LIST *, IndexFILE *);
RESULT_LIST *notresultlists(SWISH *, RESULT_LIST *, RESULT_LIST *);
RESULT_LIST *phraseresultlists(SWISH *, RESULT_LIST *, RESULT_LIST *, int);
void addtoresultlist(RESULT_LIST *, int, int, int, int, IndexFILE *,SWISH *);


RESULT_LIST *sortresultsbyfilenum(RESULT_LIST *);

void getrawindexline(FILE *);
int wasStemmingAppliedToIndex(FILE *);
int wasSoundexAppliedToIndex(FILE *);

void freeresultlist(SWISH *sw,struct DB_RESULTS *);
void freeresult(SWISH *,RESULT *);
struct swline *ignore_words_in_query(SWISH *,IndexFILE *, struct swline *,unsigned char);
struct swline *stem_words_in_query(SWISH *,IndexFILE *, struct swline *);
struct swline *soundex_words_in_query(SWISH *,IndexFILE *, struct swline *);
struct swline *translatechars_words_in_query(SWISH *sw,IndexFILE *indexf,struct swline *searchwordlist);
struct swline *parse_search_string(SWISH *sw, char *words,INDEXDATAHEADER header);

RESULT_LIST *mergeresulthashlist (SWISH *sw, RESULT_LIST *r);

#endif

