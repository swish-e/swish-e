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
*/

#include "swish.h"
#include "search.h"
#include "index.h"
#include "file.h"
#include "list.h"
#include "string.h"
#include "merge.h"
#include "hash.h"
#include "mem.h"
#include "docprop.h"
#include "stemmer.h"
#include "soundex.h"
#include "error.h"
#include "compress.h"
#include "deflate.h"
#include "metanames.h"

/* 04/00 Jose Ruiz */
/* Simple routing for comparing pointers to integers in order to
get an ascending sort with qsort */
int icomp(const void *s1,const void *s2)
{
	return(*(int *)s1 - *(int *)s2);
}

/* 04/00 Jose Ruiz */
/* Simple routing for comparing pointers to integers in order to
get an ascending sort with qsort */
/* Identical to previous one but use two integers per array */
int icomp2(const void *s1,const void *s2)
{
int rc,*p1,*p2;
	rc=(*(int *)s1 - *(int *)s2);
	if(rc) return(rc);
	else {
		p1=(int *)s1;
		p2=(int *)s2;
		return(*(++p1) - *(++p2));
	}
}

int SwishAttach(sw,printflag)
SWISH *sw;
int printflag;
{
IndexFILE *indexlist;
int merge;
/* 06/00 Jose Ruiz
** Added to handle several index file headers */
char *wordchars1,*beginchars1,*endchars1,*ignorelastchar1,*ignorefirstchar1,*indexn1,*indexp1,*indexa1,*indexd1;
char *wordcharsM,*begincharsM,*endcharsM,*ignorelastcharM,*ignorefirstcharM;
char *filenames;
int applyStemmingRules1,applySoundexRules1,minwordlimit1,maxwordlimit1;
IndexFILE *tmplist;

	indexlist=sw->indexlist;	
	filenames=NULL;
	merge=0;
	sw->TotalWords=0;
	sw->TotalFiles=0;
	
        wordchars1=beginchars1=endchars1=ignorelastchar1=ignorefirstchar1=indexn1=indexp1=indexa1=indexd1=NULL;
        applyStemmingRules1=applySoundexRules1=minwordlimit1=maxwordlimit1=0;

	/* First of all . Read header default values from all index fileis */
	/* With this, we read wordchars, stripchars, ... */
	/* Also merge them */
        for (tmplist=indexlist;tmplist;) {
                sw->commonerror = RC_OK;
		sw->bigrank = 0;
                if ((tmplist->fp = openIndexFILEForRead(tmplist->line)) == NULL)
		{
			return (sw->lasterror=INDEX_FILE_NOT_FOUND);
                }
                if (!isokindexheader(tmplist->fp)) {
			return (sw->lasterror=UNKNOWN_INDEX_FILE_FORMAT);
                }
		readheader(tmplist);
		readoffsets(tmplist);
		readhashoffsets(tmplist);
		readfileoffsets(tmplist);
		readstopwords(tmplist);
		readMetaNames(tmplist);
		readlocationlookuptables(tmplist);
		readpathlookuptable(tmplist);
		if(tmplist->header.applyFileInfoCompression)
			readdeflatepatterns(tmplist);
		if(merge) {
			if(strcmp(wordchars1,tmplist->header.wordchars)) {
				wordcharsM=mergestrings(wordchars1,tmplist->header.wordchars);
				sw->mergedheader.wordchars=SafeStrCopy(sw->mergedheader.wordchars,wordcharsM,&sw->mergedheader.lenwordchars);
				efree(wordcharsM);
			}
			if(strcmp(beginchars1,tmplist->header.beginchars)) {
				begincharsM=mergestrings(beginchars1,tmplist->header.beginchars);
				sw->mergedheader.beginchars=SafeStrCopy(sw->mergedheader.beginchars,begincharsM,&sw->mergedheader.lenbeginchars);
				efree(begincharsM);
			}
			if(strcmp(endchars1,tmplist->header.endchars)) {
				endcharsM=mergestrings(endchars1,tmplist->header.endchars);
				sw->mergedheader.endchars=SafeStrCopy(sw->mergedheader.endchars,endcharsM,&sw->mergedheader.lenendchars);
				efree(endcharsM);
			}
			if(strcmp(ignorelastchar1,tmplist->header.ignorelastchar)) {
				ignorelastcharM=mergestrings(ignorelastchar1,tmplist->header.ignorelastchar);
				sw->mergedheader.ignorelastchar=SafeStrCopy(sw->mergedheader.ignorelastchar,ignorelastcharM,&sw->mergedheader.lenignorelastchar);
				efree(ignorelastcharM);
			}
			if(strcmp(ignorefirstchar1,tmplist->header.ignorefirstchar)) {
				ignorefirstcharM=mergestrings(ignorefirstchar1,tmplist->header.ignorefirstchar);
				sw->mergedheader.ignorefirstchar=SafeStrCopy(sw->mergedheader.ignorefirstchar,ignorefirstcharM,&sw->mergedheader.lenignorefirstchar);
				efree(ignorefirstcharM);
			}
			sw->mergedheader.applyStemmingRules=applyStemmingRules1 && tmplist->header.applyStemmingRules;
			sw->mergedheader.applySoundexRules=applySoundexRules1 && tmplist->header.applySoundexRules;
			if(minwordlimit1<sw->mergedheader.minwordlimit) sw->mergedheader.minwordlimit=minwordlimit1;
			if(maxwordlimit1<sw->mergedheader.maxwordlimit) sw->mergedheader.maxwordlimit=maxwordlimit1;
			if(strcmp(indexp1,tmplist->header.indexp)) sw->mergedheader.indexp=SafeStrCopy(sw->mergedheader.indexp,"(several)",&sw->mergedheader.lenindexp);
			if(strcmp(indexa1,tmplist->header.indexa)) sw->mergedheader.indexa=SafeStrCopy(sw->mergedheader.indexa,"(several)",&sw->mergedheader.lenindexa);
			if(strcmp(indexd1,tmplist->header.indexd)) sw->mergedheader.indexd=SafeStrCopy(sw->mergedheader.indexd,"(several)",&sw->mergedheader.lenindexd);
			if(strcmp(indexn1,tmplist->header.indexn)) sw->mergedheader.indexn=SafeStrCopy(sw->mergedheader.indexn,"(several)",&sw->mergedheader.lenindexn);

			sw->mergedheader.savedasheader=SafeStrCopy(sw->mergedheader.savedasheader,"(several)",&sw->mergedheader.lensavedasheader);
			sw->mergedheader.indexedon=SafeStrCopy(sw->mergedheader.indexedon,"(several)",&sw->mergedheader.lenindexedon);
			efree(wordchars1);
			efree(beginchars1);
			efree(endchars1);
			efree(ignorelastchar1);
			efree(ignorefirstchar1);
			efree(indexn1);
			efree(indexd1);
			efree(indexa1);
			efree(indexp1);
		} else {    /* Merged header and header are the same if just one file is used */
			sw->mergedheader.wordchars=SafeStrCopy(sw->mergedheader.wordchars,tmplist->header.wordchars,&sw->mergedheader.lenwordchars);
			sw->mergedheader.beginchars=SafeStrCopy(sw->mergedheader.beginchars,tmplist->header.beginchars,&sw->mergedheader.lenbeginchars);
			sw->mergedheader.endchars=SafeStrCopy(sw->mergedheader.endchars,tmplist->header.endchars,&sw->mergedheader.lenendchars);
			sw->mergedheader.ignorelastchar=SafeStrCopy(sw->mergedheader.ignorelastchar,tmplist->header.ignorelastchar,&sw->mergedheader.lenignorelastchar);
			sw->mergedheader.ignorefirstchar=SafeStrCopy(sw->mergedheader.ignorefirstchar,tmplist->header.ignorefirstchar,&sw->mergedheader.lenignorefirstchar);
			sw->mergedheader.applyStemmingRules=tmplist->header.applyStemmingRules;
			sw->mergedheader.applySoundexRules=tmplist->header.applySoundexRules;
			sw->mergedheader.minwordlimit=tmplist->header.minwordlimit;
			sw->mergedheader.maxwordlimit=tmplist->header.maxwordlimit;
			sw->mergedheader.indexp=SafeStrCopy(sw->mergedheader.indexp,tmplist->header.indexp,&sw->mergedheader.lenindexp);
			sw->mergedheader.indexa=SafeStrCopy(sw->mergedheader.indexa,tmplist->header.indexa,&sw->mergedheader.lenindexa);
			sw->mergedheader.indexd=SafeStrCopy(sw->mergedheader.indexd,tmplist->header.indexd,&sw->mergedheader.lenindexd);
			sw->mergedheader.indexn=SafeStrCopy(sw->mergedheader.indexn,tmplist->header.indexn,&sw->mergedheader.lenindexn);
			sw->mergedheader.savedasheader=SafeStrCopy(sw->mergedheader.savedasheader,tmplist->header.savedasheader,&sw->mergedheader.lensavedasheader);
			sw->mergedheader.indexedon=SafeStrCopy(sw->mergedheader.indexedon,tmplist->header.indexedon,&sw->mergedheader.lenindexedon);
		}
		sw->TotalWords+=tmplist->header.totalwords;
		sw->TotalFiles+=tmplist->header.totalfiles;
		tmplist=tmplist->next;
		if(tmplist) {  /* If there are more index file we need
				 ** to preserve header values */
		
			wordchars1=estrdup(sw->mergedheader.wordchars);sortstring(wordchars1);
			beginchars1=estrdup(sw->mergedheader.beginchars);sortstring(beginchars1);
			endchars1=estrdup(sw->mergedheader.endchars);sortstring(endchars1);
			ignorelastchar1=estrdup(sw->mergedheader.ignorelastchar);sortstring(ignorelastchar1);
			ignorefirstchar1=estrdup(sw->mergedheader.ignorefirstchar);sortstring(ignorefirstchar1);
			indexn1=estrdup(sw->mergedheader.indexn);
			indexp1=estrdup(sw->mergedheader.indexp);
			indexa1=estrdup(sw->mergedheader.indexa);
			indexd1=estrdup(sw->mergedheader.indexd);
			applyStemmingRules1=sw->mergedheader.applyStemmingRules;
			applySoundexRules1=sw->mergedheader.applySoundexRules;
			minwordlimit1=sw->mergedheader.minwordlimit;
			maxwordlimit1=sw->mergedheader.maxwordlimit;
			merge=1;
		}
		if(!filenames) 
		{
			filenames=estrdup(sw->mergedheader.savedasheader);
		} else {
			filenames=erealloc(filenames,strlen(filenames)+strlen(sw->mergedheader.savedasheader)+2);
			sprintf(filenames,"%s %s",filenames,sw->mergedheader.savedasheader);
		}
	}
	if (printflag) printheader(&sw->mergedheader, stdout, filenames, sw->TotalWords, sw->TotalFiles, 0);
	efree(filenames);
	
	/* Make lookuptables for char processing */
	makelookuptable(sw->mergedheader.wordchars,sw->mergedheader.wordcharslookuptable);
	makelookuptable(sw->mergedheader.beginchars,sw->mergedheader.begincharslookuptable);
	makelookuptable(sw->mergedheader.endchars,sw->mergedheader.endcharslookuptable);
	makelookuptable(sw->mergedheader.ignorefirstchar,sw->mergedheader.ignorefirstcharlookuptable);
	makelookuptable(sw->mergedheader.ignorelastchar,sw->mergedheader.ignorelastcharlookuptable);
	return (sw->lasterror=RC_OK);
}


int search(sw, words, structure,extended_info)
SWISH *sw;
char *words;
int structure;
int extended_info;
{
int i, j, k, metaID, indexYes, totalResults;
char word[MAXWORDLEN];
RESULT *tmpresultlist,*tmpresultlist2;
struct swline *tmplist, *tmplist2;
IndexFILE *indexlist;
int rc=0;
int PhraseDelimiter;
unsigned char PhraseDelimiterString[2];
	PhraseDelimiter = sw->PhraseDelimiter;
	PhraseDelimiterString[0] = (unsigned char)PhraseDelimiter;
	PhraseDelimiterString[1] = '\0';

	indexlist=sw->indexlist;
	sw->sortresultlist=NULL;
	j=0;
	sw->searchwordlist = NULL;
	metaID = 1;
	indexYes =0;
	totalResults=0;

	sw->lasterror=RC_OK;
	sw->commonerror = RC_OK;

	if (words[0] == '\0')  return (sw->lasterror=NO_WORDS_IN_SEARCH);

	for (i = j = 0; words[i] != '\0' && words[i] != '\n'; i++) 
	{
		/* 06/00 Jose ruiz
		** Following line modified to extract words according
		** to wordchars as suggested by Bill Moseley
		*/
		if (isspace((int)((unsigned char)words[i])) || words[i] == '(' || words[i] == ')' || (words[i] == '=') || (words[i] == ((unsigned char)PhraseDelimiter)) || !((words[i]=='*') || iswordchar(sw->mergedheader,words[i]))) /* cast to int, 2/22/00 */
		{
			if (words[i] == '=')
			{
				if (j != 0)
				{
					if (words[i-1] != '\\')
					{ 
						word[j] = '\0';
						sw->searchwordlist = (struct swline *) addswline(sw->searchwordlist, word);
						j = 0;
						sw->searchwordlist = (struct swline *) addswline(sw->searchwordlist, "=");
					}
					else
					{
						/* Needs to erase the '\' */
						j--;
						word[j] = tolower(words[i]);
						j++;
					}
				}
				else
				{
					sw->searchwordlist = (struct swline *) addswline(sw->searchwordlist, "=");
				}
			}
			else
			{
				if (j) 
				{
					word[j] = '\0';
                               /* Convert chars ignored in words to spaces  */
                                        stripIgnoreLastChars(sw->mergedheader,word);
                                        stripIgnoreFirstChars(sw->mergedheader,word);
					if(strlen(word))
					{
						sw->searchwordlist = (struct swline *) addswline(sw->searchwordlist, word);
					}
					j = 0;
				}
				if (words[i] == '(') 
				{
					sw->searchwordlist = (struct swline *) addswline(sw->searchwordlist, "(");
				}
				if (words[i] == ')') 
				{
					sw->searchwordlist = (struct swline *) addswline(sw->searchwordlist, ")");
				}
				if (words[i] == (unsigned char)PhraseDelimiter) 
				{
					sw->searchwordlist = (struct swline *) addswline(sw->searchwordlist, PhraseDelimiterString);
				}
			}
		}
		else 
		{
			word[j] = tolower(words[i]);
			j++;
		}
	}
	if (j) 
	{
		word[j] = '\0';
       /* Convert chars ignored in words to spaces  */
                stripIgnoreLastChars(sw->mergedheader,word);
                stripIgnoreFirstChars(sw->mergedheader,word);
		if(strlen(word))
		{
			sw->searchwordlist = (struct swline *) addswline(sw->searchwordlist, word);
		}
	}
	
	if (!sw->searchwordlist)  return (sw->lasterror=NO_WORDS_IN_SEARCH);

	while (indexlist != NULL) {
		sw->bigrank = 0;
		
		if (!indexlist->fp)
			return sw->lasterror;

		if (!indexlist->header.totalfiles) {
			indexlist = indexlist->next;
			continue;
		}
		else
		{ indexYes = 1; /*There is a non-empty index */ }
		if ((rc=initSearchResultProperties(sw))) return rc;
		if (isSortProp(sw))
			if ((rc=initSortResultProperties(sw))) return rc;

		tmpresultlist=NULL;
		tmplist=NULL;

		/* make a copy of sw->searchwordlist */
		tmplist = dupswline(sw->searchwordlist);
#ifdef IGNORE_STOPWORDS_IN_QUERY
		tmplist = ignore_words_in_query(sw,indexlist,tmplist);
#endif /* IGNORE_STOPWORDS_IN_QUERY */
			/* Echo index file, fixed search, stopwords */
		if (extended_info)
		{
			printf("# Index File: %s\n",indexlist->line);
			printheader(&indexlist->header,stdout,indexlist->header.savedasheader, indexlist->header.totalwords, indexlist->header.totalfiles,0);
			printf("# StopWords:");
			for (k=0;k<indexlist->stopPos;k++)
				printf(" %s",indexlist->stopList[k]);
			printf("\n");
			printf("# Parsed Search Words:");
			tmplist2=tmplist; 
			while(tmplist2)
			{
				printf(" %s",tmplist2->line);
				tmplist2=tmplist2->next;
			}
			printf("\n");
		}
		/* Expand phrase search: "kim harlow" becomes (kim PHRASE_WORD harlow) */
		tmplist = (struct swline *) expandphrase(tmplist,PhraseDelimiter);
		tmplist = (struct swline *) fixnot(tmplist); 

		if(tmplist)
		{
			tmplist2=tmplist;
			tmpresultlist = (RESULT *) parseterm(sw,0,metaID,indexlist,&tmplist2);

			/* 04/00 Jose Ruiz-Get properties first before sorting. 
			** In this way we can sort results by metaName
			*/
			if(tmpresultlist) tmpresultlist = (RESULT *) getproperties(sw,indexlist,tmpresultlist);
		}
		if(tmplist) freeswline(tmplist);

		if(!sw->resultlist)
			sw->resultlist = tmpresultlist;
		else {
			tmpresultlist2=sw->resultlist;
			while(tmpresultlist2) {
				if(!tmpresultlist2->next) {
					tmpresultlist2->next=tmpresultlist;
					break;
				}
				tmpresultlist2=tmpresultlist2->next;
			}

		}
                indexlist = indexlist->next;
	}

/* 
04/00 Jose Ruiz - Sort results by rank or by properties
*/
	if (isSortProp(sw)) 
		sw->sortresultlist = (RESULT *) sortresultsbyproperty(sw, structure);
	else
		sw->sortresultlist = (RESULT *) sortresultsbyrank(sw, structure);
	if (!sw->sortresultlist) {
		if (!sw->commonerror) totalResults=0;
	} else {
			/* Point current result at the begining */
		sw->currentresult = sw->sortresultlist;
		totalResults = countResults(sw->sortresultlist);
	}
		
	if (!totalResults && sw->commonerror) return (sw->lasterror=WORDS_TOO_COMMON);
	if (!totalResults && !indexYes) return (sw->lasterror=INDEX_FILE_IS_EMPTY);

	return totalResults;
}


/* This puts parentheses in the right places around not structures
** so the parser can do its thing correctly.
** It does it both for 'not' and '='; the '=' is used for the METADATA (GH)
*/

struct swline *fixnot(sp)
struct swline *sp;
{
int openparen, hasnot;
int openMeta, hasMeta;
int isfirstnot=0,metapar;
struct swline *tmpp, *newp;
	if(!sp) return NULL;
	/* 06/00 Jose Ruiz - Check if first word is NOT_RULE */
	/* Change remaining NOT by AND_NOT_RULE */
	for(tmpp = sp;tmpp;tmpp=tmpp->next) {
		if (tmpp->line[0]=='(') continue;
		else if ( isnotrule(tmpp->line) ) {
			isfirstnot=1;
		} else break;
	}
	for(tmpp = sp;tmpp;tmpp=tmpp->next) {
		if ( isnotrule(tmpp->line)) {
			if(!isfirstnot) {
				efree(tmpp->line);
				tmpp->line=estrdup(AND_NOT_WORD);
			}else isfirstnot=0;
		}
	}

	tmpp = sp;
	newp = NULL;
	
	openparen = 0;
	openMeta = 0;
	hasMeta = 0;
	hasnot = 0;
	while (tmpp != NULL) {
		if ( ((tmpp->line)[0] == '(') && hasnot)
			openparen++;
		else if ( ((tmpp->line)[0] == '(') && hasMeta) 
			openMeta++;
		else if ( ((tmpp->line)[0] == ')') && hasnot)
			openparen--;
		else if ( ((tmpp->line)[0] == ')') && hasMeta)
			openMeta--;
		if (isMetaName(tmpp->next)) {
			/* If it is a metaName add the name and = and skip to next */
			hasMeta = 1;
			newp = (struct swline *) addswline(newp, "(");
			newp = (struct swline *) addswline(newp, tmpp->line);
			newp = (struct swline *) addswline(newp, "=");
			tmpp = tmpp->next;
			tmpp = tmpp->next;
			continue;
		}
		if ( isnotrule(tmpp->line) ) {
			hasnot = 1;
			newp = (struct swline *) addswline(newp, "("); 
		}
		else if (hasnot && !openparen) {
			hasnot = 0;
			newp = (struct swline *) addswline(newp, tmpp->line);
			newp = (struct swline *) addswline(newp, ")");
			tmpp = tmpp->next;
			continue;
		}
		else if (hasMeta && !openMeta) {
			hasMeta = 0;
				/* 06/00 Jose Ruiz
				** Fix to consider parenthesys in the
				** content of a MetaName */
			if (tmpp->line[0] == '(') {
				metapar=1;
				newp = (struct swline *) addswline(newp, tmpp->line);
				tmpp=tmpp->next;
				while (metapar && tmpp) {
					if (tmpp->line[0]=='(') metapar++;
					else if (tmpp->line[0]==')') metapar--;
					newp = (struct swline *) addswline(newp, tmpp->line);
					if(metapar) tmpp=tmpp->next;
				}
				if(!tmpp) return(newp);
			} else
				newp = (struct swline *) addswline(newp, tmpp->line);
			newp = (struct swline *) addswline(newp, ")");
			tmpp = tmpp->next;
			continue;
		}
		newp = (struct swline *) addswline(newp, tmpp->line);
		if (isMetaName(tmpp)) {
			hasMeta = 1;
			newp = (struct swline *) addswline(newp, "(");
		}
		tmpp = tmpp->next;
	}
	
	freeswline(sp);
	return newp;
}

/* expandstar removed - Jose Ruiz 04/00 */

/* Expands phrase search. Berkeley University becomes Berkeley PHRASE_WORD University */
/* It also fixes the and, not or problem when they appeared inside a phrase */
struct swline *expandphrase( struct swline *sp, char delimiter)
{
struct swline *tmp,*newp;
int inphrase;
	if(!sp) return NULL;
	inphrase = 0;
	newp = NULL;
	tmp = sp;
	while(tmp != NULL) {
		if((tmp->line)[0]==delimiter) {
			if (inphrase) 
			{
				inphrase = 0;
				newp = (struct swline *) addswline(newp,")");
			}
			else
			{
				inphrase++;
				newp = (struct swline *) addswline(newp,"(");
			}
		}
		else
		{
			if (inphrase)
			{
				if(inphrase > 1) 
				newp = (struct swline *) addswline(newp,PHRASE_WORD);
				inphrase++;
				newp = (struct swline *) addswline(newp,tmp->line);
			} else {
				/* Fix for and, not or when not in a phrase */
				if(!strcasecmp(tmp->line,_AND_WORD))
					newp = (struct swline *) addswline(newp,AND_WORD);
				else if(!strcasecmp(tmp->line,_OR_WORD))
					newp = (struct swline *) addswline(newp,OR_WORD);
				else if(!strcasecmp(tmp->line,_NOT_WORD))
					newp = (struct swline *) addswline(newp,NOT_WORD);
				else
					newp = (struct swline *) addswline(newp,tmp->line);
			}
		}
		tmp = tmp->next;
	}
	freeswline(sp);
	return newp;
}

/*  getmatchword removed. Obsolete. Jose Ruiz 04/00 */

/* Reads and prints the header of an index file.
** Also reads the information in the header (wordchars, beginchars, etc)
*/

#define ReadHeaderStr(buffer,bufferlen,len,fp)  uncompress1(len,fp); if(bufferlen<len) buffer=erealloc(buffer,(bufferlen=len+200)+1);  fread(buffer,len,1,fp);

#define ReadHeaderInt(itmp,fp) uncompress1(itmp,fp); itmp--;

void readheader(indexf)
IndexFILE *indexf;
{
long swish_magic;
int id,len,itmp;
int bufferlen;
char *buffer;
FILE *fp=indexf->fp;
	buffer=emalloc((bufferlen=MAXSTRLEN)+1);
	swish_magic=readlong(fp);
	uncompress1(id,fp);
	while (id) {
		switch(id) {
			case INDEXHEADER_ID:
			case INDEXVERSION_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				break;
			case MERGED_ID:
			case DOCPROPENHEADER_ID:
				ReadHeaderInt(itmp,fp);
				break;
			case WORDCHARSHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.wordchars = SafeStrCopy(indexf->header.wordchars,buffer,&indexf->header.lenwordchars);
				sortstring(indexf->header.wordchars);
				break;
			case BEGINCHARSHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.beginchars = SafeStrCopy(indexf->header.beginchars,buffer,&indexf->header.lenbeginchars);
				sortstring(indexf->header.beginchars);
				break;
			case ENDCHARSHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.endchars = SafeStrCopy(indexf->header.endchars,buffer,&indexf->header.lenendchars);
				sortstring(indexf->header.endchars);
				break;
			case IGNOREFIRSTCHARHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.ignorefirstchar = SafeStrCopy(indexf->header.ignorefirstchar,buffer,&indexf->header.lenignorefirstchar);
				sortstring(indexf->header.ignorefirstchar);
				break;
			case IGNORELASTCHARHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.ignorelastchar = SafeStrCopy(indexf->header.ignorelastchar,buffer,&indexf->header.lenignorelastchar);
				sortstring(indexf->header.ignorelastchar);
				break;
			case STEMMINGHEADER_ID:
				ReadHeaderInt(itmp,fp);
				indexf->header.applyStemmingRules = itmp;
				break;
			case SOUNDEXHEADER_ID:
				ReadHeaderInt(itmp,fp);
				indexf->header.applySoundexRules = itmp;
				break;
			case IGNORETOTALWORDCOUNTWHENRANKING_ID:
				ReadHeaderInt(itmp,fp);
				indexf->header.ignoreTotalWordCountWhenRanking = itmp;
				break;
			case MINWORDLIMHEADER_ID:
				ReadHeaderInt(itmp,fp);
				indexf->header.minwordlimit = itmp;
				break;
			case MAXWORDLIMHEADER_ID:
				ReadHeaderInt(itmp,fp);
				indexf->header.maxwordlimit = itmp;
				break;
			case SAVEDASHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.savedasheader = SafeStrCopy(indexf->header.savedasheader,buffer,&indexf->header.lensavedasheader);
				break;
			case NAMEHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.indexn = SafeStrCopy(indexf->header.indexn,buffer,&indexf->header.lenindexn);
				break;
			case DESCRIPTIONHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.indexd = SafeStrCopy(indexf->header.indexd,buffer,&indexf->header.lenindexd);
				break;
			case POINTERHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.indexp = SafeStrCopy(indexf->header.indexp,buffer,&indexf->header.lenindexp);
				break;
			case MAINTAINEDBYHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.indexa = SafeStrCopy(indexf->header.indexa,buffer,&indexf->header.lenindexa);
				break;
			case INDEXEDONHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.indexedon = SafeStrCopy(indexf->header.indexedon,buffer,&indexf->header.lenindexedon);
				break;
			case COUNTSHEADER_ID:
				ReadHeaderInt(itmp,fp);
				indexf->header.totalwords=itmp;
				ReadHeaderInt(itmp,fp);
				indexf->header.totalfiles=itmp;
				break;
			case FILEINFOCOMPRESSION_ID:
				ReadHeaderInt(itmp,fp);
				indexf->header.applyFileInfoCompression = itmp;
				break;
			default:
				progerr("Severe index error in header\n.\n");
				break;
		}
		uncompress1(id,fp);
	}
	efree(buffer);
}

/* Reads the offsets in the index file so word lookup is faster.
The file pointer is set by readheader */


void readoffsets(indexf)
IndexFILE *indexf;
{
int i;
	for (i=0;i<MAXCHARS;i++) 
	{
		indexf->offsets[i] = readlong(indexf->fp);
	}
}

/*Jose Ruiz 04/00
Reads the hash index
The file pointer is set by readoffsets */
void readhashoffsets(indexf)
IndexFILE *indexf;
{
int i;
	for (i=0;i<SEARCHHASHSIZE;i++) 
		indexf->hashoffsets[i] = readlong(indexf->fp);
		/* start of words in index file */
	indexf->wordpos=ftell(indexf->fp);
}

/* Reads the stopwords in the index file.
*/

void readstopwords(indexf)
IndexFILE *indexf;
{
int len;
int lenword=0;
char *word=NULL;
FILE *fp=indexf->fp;
	
	word = (char *) emalloc((lenword=MAXWORDLEN) + 1);
	fseek(fp, indexf->offsets[STOPWORDPOS], 0);
	uncompress1(len,fp);
	while (len) {
		if(len>=lenword) {
			lenword*=len + 200;
			word = (char *) erealloc(word,lenword+1);
		}
		fread(word,len,1,fp);
		word[len]='\0';
		addStopList(indexf,word);
		addstophash(indexf,word);
		uncompress1(len,fp);
	}
	efree(word);
}

/* Reads the metaNames from the index
*/

void readMetaNames(indexf)
IndexFILE *indexf;
{
int len;
int dummy;
int wordlen,metaType;
char *word;
FILE *fp=indexf->fp;

	wordlen = MAXWORDLEN;
	word=(char *)emalloc(MAXWORDLEN +1);
	
	fseek(fp, indexf->offsets[METANAMEPOS], 0);
	uncompress1(len,fp);
	while ( len )
	{
		if(len>=wordlen) {
			wordlen=len+200;
			word = (char *) erealloc(word,wordlen+1);
		}
		fread(word,len,1,fp);
		word[len]='\0';
			/* It was saved as Style+1 */
		uncompress1(metaType,fp);
		metaType--;
			/* add the meta tag */
		addMetaEntry(indexf, word, metaType, &dummy); 

		uncompress1(len,fp);
	}
	efree(word);
}

/* Reads the file offset table in the index file.
*/

void readfileoffsets(indexf)
IndexFILE *indexf;
{
long pos, totwords;
struct file *filep;
FILEOFFSET *fileo;
FILE *fp=indexf->fp;
	indexf->filearray=(struct file **)emalloc((indexf->filearray_maxsize=BIGHASHSIZE)*sizeof(struct file *));
	indexf->fileoffsetarray=(FILEOFFSET **)emalloc((indexf->fileoffsetarray_maxsize=BIGHASHSIZE)*sizeof(FILEOFFSET *));
	fseek(fp,indexf->offsets[FILEOFFSETPOS], 0);
	for (indexf->filearray_cursize=0,indexf->fileoffsetarray_cursize=0,pos=1L;(pos = readlong(fp));indexf->filearray_cursize++,indexf->fileoffsetarray_cursize++) {
		if(indexf->filearray_cursize==indexf->filearray_maxsize) 
		{
			indexf->filearray=(struct file **)erealloc(indexf->filearray,(indexf->filearray_maxsize+=1000)*sizeof(struct file *));
		}
		if(indexf->fileoffsetarray_cursize==indexf->fileoffsetarray_maxsize) 
		{
			indexf->fileoffsetarray=(FILEOFFSET **)erealloc(indexf->fileoffsetarray,(indexf->fileoffsetarray_maxsize+=1000)*sizeof(FILEOFFSET *));
		}
		totwords= readlong(fp);
		filep=(struct file *) emalloc(sizeof(struct file));
		filep->fi.filename=NULL;
		filep->fi.title=NULL;
		filep->fi.summary=NULL;
		filep->fi.start=0;
		filep->fi.size=0;
		filep->docProperties=NULL;
		filep->read=0;
		indexf->filearray[indexf->filearray_cursize]=filep;
		fileo=(FILEOFFSET *) emalloc(sizeof(FILEOFFSET));
		fileo->filelong=pos;
		fileo->ftotalwords=totwords;
		indexf->fileoffsetarray[indexf->fileoffsetarray_cursize]=fileo;
	}
}

/* Read the lookuptables for structure, frequency */
void readlocationlookuptables(IndexFILE *indexf)
{
FILE *fp=indexf->fp;
int i,n,tmp;
	fseek(fp,indexf->offsets[LOCATIONLOOKUPTABLEPOS], 0);
	uncompress1(n,fp);
	if(!n)  /* No words in file !!! */
	{
		indexf->structurelookup=NULL;
		uncompress1(n,fp);   /* just to maintain file pointer */
		indexf->structfreqlookup=NULL;
		return;
	}
	indexf->structurelookup=(struct int_lookup_st *)emalloc(sizeof(struct int_lookup_st)+sizeof(struct int_st *)*(n-1));
	indexf->structurelookup->n_entries=n;
	for(i=0;i<n;i++)
	{
		indexf->structurelookup->all_entries[i]=(struct int_st *)emalloc(sizeof(struct int_st));
		uncompress1(tmp,fp);
		indexf->structurelookup->all_entries[i]->val[0]=tmp-1;
	}
	uncompress1(n,fp);
	indexf->structfreqlookup=(struct int_lookup_st *)emalloc(sizeof(struct int_lookup_st)+sizeof(struct int_st *)*(n-1));
	indexf->structfreqlookup->n_entries=n;
	for(i=0;i<n;i++)
	{
		indexf->structfreqlookup->all_entries[i]=(struct int_st *)emalloc(sizeof(struct int_st)+sizeof(int));
		uncompress1(tmp,fp);
		indexf->structfreqlookup->all_entries[i]->val[0]=tmp-1;
		uncompress1(tmp,fp);
		indexf->structfreqlookup->all_entries[i]->val[1]=tmp-1;
	}
}

/* Read the lookuptable for paths/urls */
void readpathlookuptable(IndexFILE *indexf)
{
FILE *fp=indexf->fp;
int i,n,len;
char *tmp;
	fseek(fp,indexf->offsets[PATHLOOKUPTABLEPOS], 0);
	uncompress1(n,fp);
	indexf->pathlookup=(struct char_lookup_st *)emalloc(sizeof(struct char_lookup_st)+sizeof(struct char_st *)*(n-1));
	indexf->pathlookup->n_entries=n;
	for(i=0;i<n;i++)
	{
		indexf->pathlookup->all_entries[i]=(struct char_st *)emalloc(sizeof(struct char_st));
		uncompress1(len,fp);
		tmp=emalloc(len);
		fread(tmp,len,1,fp);
		indexf->pathlookup->all_entries[i]->val=tmp;
	}
}

/* The recursive parsing function.
** This was a headache to make but ended up being surprisingly easy. :)
** parseone tells the function to only operate on one word or term.
*/

RESULT *parseterm(sw, parseone, metaID, indexf, searchwordlist)
SWISH *sw;
int parseone;
int metaID;
IndexFILE *indexf;
struct swline **searchwordlist;
{
int rulenum;
char *word;
int lenword;
RESULT *rp, *newrp;
FILE *fp=indexf->fp;
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
	int andLevel = 0;	/* number of terms ANDed so far */

	word = NULL;
	lenword = 0;

	rp = NULL;
	
	rulenum = OR_RULE;
	while (*searchwordlist) {
		word = SafeStrCopy(word, (*searchwordlist)->line,&lenword);
		
		if (rulenum == NO_RULE)
			rulenum = DEFAULT_RULE;
		if (isunaryrule(word)) {
			*searchwordlist = (*searchwordlist)->next;
			rp = (RESULT *) parseterm(sw, 1, metaID,indexf, searchwordlist);
			rp = (RESULT *) notresultlist(sw, rp, indexf);
			/* Wild goose chase */
			rulenum = NO_RULE;
			continue;
		}
		else if (isbooleanrule(word)) {
			rulenum = getrulenum(word);
			*searchwordlist = (*searchwordlist)->next;
			continue;
		}
		
		if (rulenum != AND_RULE)
			andLevel = 0;	/* reset */
		else if (rulenum == AND_RULE)
			andLevel++;
		
		if (word[0] == '(') {
			
			*searchwordlist = (*searchwordlist)->next;
			newrp = (RESULT *) parseterm(sw, 0, metaID, indexf,searchwordlist);
			
			if (rulenum == AND_RULE)
				rp = (RESULT *)
				andresultlists(sw, rp, newrp, andLevel);
			else if (rulenum == OR_RULE)
				rp = (RESULT *)
				orresultlists(sw, rp, newrp);
			else if (rulenum == PHRASE_RULE)
				rp = (RESULT *)
				phraseresultlists(sw, rp, newrp,1);
			else if (rulenum == AND_NOT_RULE)
				rp = (RESULT *)
				notresultlists(sw, rp, newrp);

			if (! *searchwordlist)
				break;
			
			rulenum = NO_RULE;
			continue;
			
		}
		else if (word[0] == ')') {
			*searchwordlist = (*searchwordlist)->next;
			break;
		}
		
		/* Check if the next word is '=' */
		if ( isMetaName((*searchwordlist)->next) ) {
			metaID = getMetaNameID(indexf, word);
			if (metaID == 1) {
				sw->lasterror=UNKNOWN_METANAME;
				return NULL;
			}
			/* Skip both the metaName end the '=' */
			*searchwordlist = (*searchwordlist)->next->next;
			if((*searchwordlist) && ((*searchwordlist)->line[0]=='('))
			{
				*searchwordlist = (*searchwordlist)->next;
				parseone=0;
			} else 
				parseone=1;
			newrp = (RESULT *) parseterm(sw, parseone, metaID, indexf,searchwordlist);
			if (rulenum == AND_RULE)
				rp = (RESULT *) andresultlists(sw, rp, newrp, andLevel);
			else if (rulenum == OR_RULE)
				rp = (RESULT *) orresultlists(sw, rp, newrp);
			else if (rulenum == PHRASE_RULE)
				rp = (RESULT *) phraseresultlists(sw, rp, newrp,1);
			else if (rulenum == AND_NOT_RULE)
				rp = (RESULT *)notresultlists(sw, rp, newrp);
			
			if (! *searchwordlist)
				break;
			
			rulenum = NO_RULE;
			metaID = 1;
			continue;
		}
	
		rp = (RESULT *) operate(sw, rp, rulenum, word, fp, metaID, andLevel,indexf);
		
		if (parseone) {
			*searchwordlist = (*searchwordlist)->next;
			break;
		}
		rulenum = NO_RULE;
		
		*searchwordlist = (*searchwordlist)->next;
	}
	
	if(lenword) efree(word);
	return rp;
}

/* Looks up a word in the index file -
** it calls getfileinfo(), which does the real searching.
*/

RESULT *operate(sw, rp, rulenum, wordin, fp, metaID, andLevel, indexf)
SWISH *sw;
RESULT *rp;
int rulenum;
char *wordin;
FILE *fp;
int metaID;
int andLevel;
IndexFILE *indexf;
{
int i, found;
RESULT *newrp, *returnrp;
char *tmp;
char *word;
int lenword;
	word=estrdup(wordin);
	lenword=strlen(word);

	newrp = returnrp = NULL;

	if (indexf->header.applyStemmingRules)
	{
		/* apply stemming algorithm to the search term */
		i=strlen(word)-1;
		if(i && word[i]=='*') word[i]='\0';
		else i=0; /* No star */
		Stem(&word,&lenword);
		if(i) 
		{
			tmp=emalloc(strlen(word)+2);
			strcpy(tmp,word);
			strcat(tmp,"*"); 
			efree(word);
			word=tmp;
		}
	}
        if (indexf->header.applySoundexRules)
        {
                /* apply soundex algorithm to the search term */
		i=strlen(word)-1;
		if(i && word[i]=='*') word[i]='\0';
		else i=0; /* No star */
                soundex(word);   /* Need to fix word length ? */
		if(i && (strlen(word)-1)<MAXWORDLEN) strcat(word,"*"); 
        }

	if (isstopword(indexf,word) && !isrule(word)) 
	{
		if (rulenum == OR_RULE && rp != NULL)
			return rp;
		else
			sw->commonerror = 1;
	}

        i=(int)((unsigned char)word[0]);
        found=isindexchar(indexf->header,i);
	
	if (!found) 
	{
		if (rulenum == AND_RULE || rulenum == PHRASE_RULE)
			return NULL;
		else if (rulenum == OR_RULE)
			return rp;
	}
	
	if (rulenum == AND_RULE) {
		newrp = (RESULT *) getfileinfo(sw, word, indexf, metaID);
		returnrp = (RESULT *) andresultlists(sw, rp, newrp, andLevel);
	} else if (rulenum == OR_RULE) {
		newrp = (RESULT *) getfileinfo(sw, word, indexf, metaID);
		returnrp = (RESULT *) orresultlists(sw, rp, newrp);
	} else if (rulenum == NOT_RULE) {
		newrp = (RESULT *) getfileinfo(sw, word, indexf, metaID);
		returnrp = (RESULT *) notresultlist(sw, newrp, indexf);
	} else if (rulenum == PHRASE_RULE) {
		newrp = (RESULT *) getfileinfo(sw, word, indexf, metaID);
		returnrp = (RESULT *) phraseresultlists(sw, rp, newrp, 1);
	} else if (rulenum == AND_NOT_RULE) {
		newrp = (RESULT *) getfileinfo(sw, word, indexf, metaID);
		returnrp = (RESULT *) notresultlists(sw, rp, newrp);
	}
	efree(word);
	return returnrp;
}

/* Finds a word and returns its corresponding file and rank information list.
** If not found, NULL is returned.
*/
/* Jose Ruiz
** New implmentation based on Hashing for direct access. Faster!!
** Also solves stars. Faster!! It can even found "and", "or"
** when looking for "an*" or "o*" if they are not stop words
*/
RESULT *getfileinfo(sw, word, indexf, metaID)
SWISH *sw;
char *word;
IndexFILE *indexf;
int metaID;
{
int i, j, x, filenum, structure, frequency,  *position, tries, found, len, curmetaID, index_structure, index_structfreq;
int filewordlen = 0;
char *fileword = NULL;
RESULT *rp, *rp2, *tmp;
int res, wordlen;
unsigned hashval;
long offset,dataoffset=0L,nextword=0L,nextposmetaname;
char *p;
struct file *fi=NULL;
int tfrequency=0;
FILE *fp=indexf->fp;

	x=j=filenum=structure=frequency=tries=len=curmetaID=index_structure=index_structfreq=0;
	position=NULL;
	nextposmetaname=0L;

        fileword = (char *) emalloc((filewordlen=MAXWORDLEN) + 1);

	rp = rp2 = NULL;
		/* First: Look for star */
	if(!(p=strchr(word,'*'))) {
			/* If there is not a star use the hash approach ... */
		res = 1;
		tries = 0;
			/* Get hash file offset */
		hashval = searchhash(word);
		if(!(offset=indexf->hashoffsets[hashval])) {
			efree(fileword);
			sw->lasterror=WORD_NOT_FOUND;
			return(NULL); 
		}
			/* Search for word */
		while (res) {
			/* tries is just to see how hash works and store hash tries */
			tries++;
			/* Position in file */
			fseek(fp,offset,SEEK_SET);
			/* Get word */
			uncompress1(wordlen,fp);
			if(wordlen > filewordlen) {
				filewordlen = wordlen + 100;
				fileword = (char *) erealloc(fileword,filewordlen + 1);
			}
			fread(fileword,1,wordlen,fp);
			fileword[wordlen]='\0';
			offset = readlong(fp);  /* Next hash */
			dataoffset = readlong(fp);  /* Offset to Word data */
			if(!(res=strcmp(word,fileword))) break;  /* Found !! */
			else if (!offset) {
				efree(fileword);
				sw->lasterror=WORD_NOT_FOUND;
				return NULL; /* No more entries if NULL*/
			}
		}
	}
	else 
	{	/* There is a star. So use the sequential approach */
		if(p == word) 
		{
			efree(fileword);
			sw->lasterror=UNIQUE_WILDCARD_NOT_ALLOWED_IN_WORD;
			return NULL;
		}
		len = p - word;
		
	        i=(int)((unsigned char)word[0]);

		if(!indexf->offsets[i])  {
			efree(fileword);
			sw->lasterror=WORD_NOT_FOUND;
			return NULL;
		}
		found=1;
		fseek(fp, indexf->offsets[i], 0);

		/* Look for first occurrence */
	        uncompress1(wordlen,fp); 
		if(wordlen > filewordlen) {
			filewordlen = wordlen + 100;
			fileword = (char *) erealloc(fileword,filewordlen + 1);
		}
	        while (wordlen) {
			fread(fileword,1,wordlen,fp);
			fileword[wordlen]='\0';
			readlong(fp);    /* jump hash offset */
			dataoffset=readlong(fp);    /* Get offset to word's data*/
			if(!(res=strncmp(word,fileword,len)))  /*Found!!*/
			{
				nextword=ftell(fp);  /* preserve next word pos */
				break;
			}
			if (res < 0) {
				efree(fileword);
				sw->lasterror=WORD_NOT_FOUND;
				return NULL;  /* Not found */
			}
				/* Go to next value */
	        	uncompress1(wordlen,fp);  /* Next word */
			if (!wordlen) {
				efree(fileword);
				sw->lasterror=WORD_NOT_FOUND;
				return NULL;  /* not found */
			}
			if(wordlen > filewordlen) {
				filewordlen = wordlen + 100;
				fileword = (char *) erealloc(fileword,filewordlen + 1);
			}
		}
	}
		/* If code is here we have found the word !! */
	do {
		/* Get the data */
		fseek(fp,dataoffset,SEEK_SET);
		uncompress1(tfrequency,fp);  /* tfrequency */
		/* Now look for a correct Metaname */
		uncompress1(curmetaID,fp); 
		while(curmetaID) {
			nextposmetaname=readlong(fp);
			if(curmetaID>=metaID) break;
			fseek(fp,nextposmetaname,0);
			uncompress1(curmetaID,fp); 
		}
		if(curmetaID==metaID) found=1;
		else found=0;
		if(found) {
			do {   /* Read on all items */
				uncompress1(filenum,fp);
				uncompress1(index_structfreq,fp);
				frequency=indexf->structfreqlookup->all_entries[index_structfreq-1]->val[0];
				index_structure=indexf->structfreqlookup->all_entries[index_structfreq-1]->val[1];
				structure=indexf->structurelookup->all_entries[index_structure-1]->val[0];
				position=(int *)emalloc(frequency*sizeof(int));
				for(j=0;j<frequency;j++) {
					uncompress1(x,fp);
					position[j] = x;
				}
				rp = (RESULT *) addtoresultlist(rp, filenum, getrank(sw, frequency, tfrequency,indexf->fileoffsetarray[filenum-1]->ftotalwords,structure), structure,frequency,position,indexf);
				if (sw->verbose == 4)
				{
					/* dump diagnostic info */
					long curFilePos;
					curFilePos = ftell(fp);	/* save */
					fi = readFileEntry(indexf, filenum);
					printf("# diag\tFILE: %s\tWORD: %s\tRANK: %d\tFREQUENCY: %d\t HASH ITEM: %d\n", fi->fi.filename, word, getrank(sw, frequency, tfrequency,indexf->fileoffsetarray[filenum-1]->ftotalwords,structure), frequency, tries);
					fseek(fp, curFilePos, 0); /* restore */
				}
			} while(ftell(fp)!=nextposmetaname);
		}
		if(!p) break;   /* direct access -> break */
		else {
			/* Jump to next word */
			/* No more data for this word but we
			are in sequential search because of
			the star (p is not null) */
			/* So, go for next word */
			fseek(fp,nextword,SEEK_SET);
	        	uncompress1(wordlen,fp);
			if (!wordlen) 
				break; /* no more data */
			
			if(wordlen > filewordlen) {
				filewordlen = wordlen + 100;
				fileword = (char *) erealloc(fileword,filewordlen + 1);
			}
			fread(fileword,1,wordlen,fp);
			fileword[wordlen] = '\0';
			res=strncmp(word,fileword,len);
			if (res) break;  /* No more data */
			readlong(fp);    /* jump hash offset */
			dataoffset=readlong(fp);    /* Get data offset */
			nextword=ftell(fp);
		}
	} while(1);
	if (p) {
			/* Finally, if we are in an sequential search
			merge all results */
		initresulthashlist(sw);
		rp2 = NULL;
	 	while (rp != NULL) {
			tmp = rp->next;
	                mergeresulthashlist(sw,rp);
	        	rp=tmp;
	  	}
		for (i = 0; i < BIGHASHSIZE; i++) {
			rp = sw->resulthashlist[i];
	                while (rp != NULL) {
				rp2 = (RESULT *) addtoresultlist(rp2,
	                	rp->filenum, rp->rank, rp->structure,
			        rp->frequency, rp->position,indexf);
				tmp = rp->next;
				/* Do not free position in freeresult
				It was added to rp2 !! */
				rp->position = NULL;
				freeresult(sw,rp);
				rp = tmp;
			}
		}
		rp =rp2;
	}
	efree(fileword);
	return rp;
}

/* 11/00 Function to read all words starting with a character */
char *getfilewords(sw, c, indexf)
SWISH *sw;
IndexFILE *indexf;
char c;
{
int i,j,found;
int wordlen;
char *buffer;
int bufferpos,bufferlen;
FILE *fp=indexf->fp;
	if(!c) 
		return "";
		/* Check if already read */
	j=(int)((unsigned char)c);
	if(indexf->keywords[j]) 
		return(indexf->keywords[j]);

        i=(int)((unsigned char)c);
	if(!indexf->offsets[i])  {
		sw->lasterror=WORD_NOT_FOUND;
		return "";
	}
	fseek(fp, indexf->offsets[i], 0);

	bufferlen=MAXSTRLEN*10;
	bufferpos=0;
	buffer=emalloc(bufferlen+1);
	buffer[0]='\0';
	
		/* Look for occurrences */
	uncompress1(wordlen,fp);
	while (wordlen) {
		if((bufferpos+wordlen+1) > bufferlen) {
			bufferlen += MAXSTRLEN + wordlen + 1;
			buffer = (char *) erealloc(buffer,bufferlen + 1);
		}
		fread(buffer+bufferpos,1,wordlen,fp);
		if(c!=buffer[bufferpos])
		{
			buffer[bufferpos]='\0';
			break;
		}
		buffer[bufferpos+wordlen]='\0';
		bufferpos+=wordlen+1;
		readlong(fp);    /* jump hash offset */
		readlong(fp);    /* jump offset to word's data */
		uncompress1(wordlen,fp);
	}
	indexf->keywords[j]=buffer;
	return(indexf->keywords[j]);
}

/* Is a word a rule?
*/

int isrule(word)
char *word;
{
	if (!strcmp(word, AND_WORD) || !strcmp(word, OR_WORD) || !strcmp(word, NOT_WORD) || !strcmp(word, PHRASE_WORD) || !strcmp(word, AND_NOT_WORD))
		return 1;
	else
		return 0;
}

int isnotrule(word)
char *word;
{
	if (!strcmp(word,NOT_WORD) )
		return 1;
	else
		return 0;
}


/* Is a word a boolean rule?
*/

int isbooleanrule(word)
char *word;
{
	if (!strcmp(word, AND_WORD) || !strcmp(word, OR_WORD) || !strcmp(word, PHRASE_WORD) || !strcmp(word, AND_NOT_WORD))
		return 1;
	else
		return 0;
}

/* Is a word a unary rule?
*/

int isunaryrule(word)
char *word;
{
	if (!strcmp(word, NOT_WORD))
		return 1;
	else
		return 0;
}

/* Return the number for a rule.
*/

int getrulenum(word)
char *word;
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
*/

RESULT *andresultlists(sw, r1, r2, andLevel)
SWISH *sw;
RESULT *r1;
RESULT *r2;
int andLevel;
{
RESULT *tmpnode, *newnode, *r1b, *r2b;
int res=0;
	
	if (r1 == NULL || r2 == NULL)
		return NULL;
	
	newnode = NULL;
	if (andLevel < 1)
		andLevel = 1;
	/* Jose Ruiz 06/00
	** Sort r1 and r2 by filenum for better performance */
	r1=sortresultsbyfilenum(r1);
	r2=sortresultsbyfilenum(r2);
	/* Jose Ruiz 04/00 -> Preserve r1 and r2 for further proccesing */
	r1b = r1;
	r2b = r2;
	
	for(;r1 && r2;) {
		res=r1->filenum - r2->filenum;
		if(!res) {
			/*
			 * Computing the new rank is interesting because
			 * we want to weight each of the words that was
			 * previously ANDed equally along with the new word.
			 * We compute a running average using andLevel and
			 * simply scale up the old average (in r1->rank)
			 * and recompute a new, equally weighted average.
			 */
			int newRank=0;
			int *allpositions;
			newRank = ((r1->rank * andLevel) + r2->rank) / (andLevel+1);
			/*
			* Storing all positions could be useful
			* in the future
			*/
			allpositions=(int *)emalloc((r1->frequency+r2->frequency)*sizeof(int));
			CopyPositions(allpositions,0,r1->position,0,r1->frequency);
			CopyPositions(allpositions,r1->frequency,r2->position,0,r2->frequency);
			newnode = (RESULT *) addtoresultlist(newnode, r1->filenum, newRank, r1->structure & r2->structure, r1->frequency + r2->frequency, allpositions,r1->indexf);
			r1 = r1->next;
			r2 = r2->next;
		} else if(res>0) {
			r2 = r2->next;
		} else {
			r1 = r1->next;
		}
	}
			/* Jose Ruiz 04/00 Free memory no longer needed */
	while (r1b) {
		tmpnode = r1b->next;
		freeresult(sw,r1b);
		r1b = tmpnode;
	}
	while (r2b) {
		tmpnode = r2b->next;
		freeresult(sw,r2b);
		r2b = tmpnode;
	}
	return newnode;
}

/* Takes two lists of results from searches and ORs them together.
*/

RESULT *orresultlists(sw, r1, r2)
SWISH *sw;
RESULT *r1;
RESULT *r2;
{
int i;
RESULT *rp, *tmp;
RESULT *newnode=NULL;

	if (r1 == NULL)
		return r2;
	else if (r2 == NULL)
		return r1;
	
	initresulthashlist(sw);
	while (r1 != NULL) {
		tmp = r1->next;  /* Save pointer now because memory can be
				 ** freed in mergeresulthashlist */
		mergeresulthashlist(sw,r1);
		r1=tmp;
	}
	while (r2 != NULL) {
		tmp = r2->next;
		mergeresulthashlist(sw,r2);
		r2=tmp;
	}
	for (i = 0; i < BIGHASHSIZE; i++) {
		rp = sw->resulthashlist[i];
		while (rp != NULL) {
			newnode = (RESULT *) addtoresultlist(newnode,
				rp->filenum, rp->rank, rp->structure,
				rp->frequency, rp->position, rp->indexf);
			tmp = rp->next;
				/* Do not free position in freeresult 
				It was added to newnode !! */
			rp->position = NULL;
			freeresult(sw,rp);
			rp = tmp;
		}
	}
	return newnode;
}

/* This performs the NOT unary operation on a result list.
** NOTed files are marked with a default rank of 1000.
**
** Basically it returns all the files that have not been
** marked (GH)
*/

RESULT *notresultlist(sw, rp, indexf)
SWISH *sw;
RESULT *rp;
IndexFILE *indexf;
{
int i, filenums;
RESULT *newp;
	
	newp = NULL;
	initmarkentrylist();
	while (rp != NULL) {
		marknum(rp->filenum);
		rp = rp->next;
	}
	
	filenums = indexf->header.totalfiles;
	
	for (i = 1; i <= filenums; i++) {

		if (!ismarked(i))
			newp = (RESULT *) addtoresultlist(newp, i, 1000, IN_ALL,0,NULL,indexf);
	}
	
	return newp;
}

RESULT *phraseresultlists(sw, r1, r2, distance)
SWISH *sw;
RESULT *r1;
RESULT *r2;
int distance;
{
RESULT *tmpnode, *newnode, *r1b, *r2b;
int i, j, found, newRank, *allpositions;
int res=0;

	if (r1 == NULL || r2 == NULL)
		return NULL;
	
	newnode = NULL;
	
	r1=sortresultsbyfilenum(r1);
	r2=sortresultsbyfilenum(r2);
	r1b = r1;
	r2b = r2;
	for (;r1 && r2;) {
		res=r1->filenum - r2->filenum;
		if(!res){
			found = 0;
			allpositions = NULL;
			for(i=0;i<r1->frequency;i++)
			{
				for(j=0;j<r2->frequency;j++)
				{
					if((r1->position[i] + distance) == r2->position[j]) {
						found++;
						if (allpositions) allpositions = (int *) erealloc(allpositions,found*sizeof(int));
						else allpositions = (int *) emalloc(found*sizeof(int));
						allpositions[found-1] = r2->position[j];
						break;
					}
				}
			}
			if (found) {
				/* To do: Compute newrank */
				newRank = (r1->rank + r2->rank) / 2;
				/*
				* Storing positions is neccesary for further
				* operations
				*/
				newnode = (RESULT *) addtoresultlist(newnode, r1->filenum, newRank, r1->structure & r2->structure, found, allpositions,r1->indexf);
			}
			r1 = r1->next;
			r2 = r2->next;
		} else if(res>0) {
			r2 = r2->next;
		} else {
			r1 = r1->next;
		}
			
	}
		/* free unused memory */
	while (r1b) {
		tmpnode = r1b->next;
		freeresult(sw,r1b);
		r1b = tmpnode;
	}
	while (r2b) {
		tmpnode = r2b->next;
		freeresult(sw,r2b);
		r2b = tmpnode;
	}
	return newnode;
}

/* Adds a file number and rank to a list of results.
*/

RESULT *addtoresultlist(rp, filenum, rank, structure, frequency, position,indexf)
RESULT *rp;
int filenum;
int rank;
int structure;
int frequency;
int *position;
IndexFILE *indexf;
{
RESULT *newnode;
	newnode = (RESULT *) emalloc(sizeof(RESULT));
	newnode->filenum = filenum;
	newnode->filename = NULL;
	newnode->title = NULL;
	newnode->summary = NULL;
	newnode->start = 0;
	newnode->size = 0;
	newnode->rank = rank;
	newnode->structure = structure;
	newnode->frequency = frequency;
	if (frequency && position)  newnode->position = position;
	else newnode->position = NULL;
	newnode->next = NULL;
	newnode->Prop = NULL;
	newnode->PropSort = NULL;
	newnode->indexf = indexf;
	
	if (rp == NULL)
		rp = newnode;
	else
		rp->head->next = newnode;
	
	rp->head = newnode;
	
	return rp;
}

/* Adds the results of a search, sorts them by rank.
*/

/* Jose Ruiz 04/00
** Complete rewrite
** Sort was made before calling this function !! -> FASTER!!
** This one just reverses order
*/
RESULT *addsortresult(sw, sphead, r)
SWISH *sw;
RESULT *sphead;
RESULT *r;
{
	if (r->rank > sw->bigrank)
		sw->bigrank = r->rank;
	if (sphead == NULL) {
		r->nextsort = NULL;
	}
	else {
		r->nextsort = sphead;
	}
	return r;
}

/* Counts the number of files that are the result
   of a search
*/

int countResults(sp)
RESULT *sp;
{
	int tot = 0;
	
	while (sp) {
		tot++;
		sp = sp->nextsort;
	}
	return(tot);
}


RESULT *SwishNext(SWISH *sw)
{
RESULT *tmp;
double  num;
	tmp = sw->currentresult;
		/* Increase Pointer */

        if (sw->bigrank) num = 1000.0f / (float) sw->bigrank;
        else num = 1000.0f;
	if(tmp) {
		tmp->rank = (int) ((float) tmp->rank * num);
		if (tmp->rank >= 999) tmp->rank = 1000;
		else if (tmp->rank <1) tmp->rank = 1;
		sw->currentresult = tmp->nextsort;
	}
	if(!tmp)sw->lasterror=SWISH_LISTRESULTS_EOF;
	return tmp;
}


/* Prints the final results of a search.
*/

void printsortedresults(sw, extended_info)
SWISH *sw;
int extended_info;
{
RESULT *sp;
int resultmaxhits;
	resultmaxhits=sw->maxhits;

	while ((sp=SwishNext(sw))) 
	{
		if (!sw->beginhits) {
			if (sw->maxhits) 
			{
				if (sw->useCustomOutputDelimiter)
				{
					if(extended_info)
						printf("%d%s%s%s%s%s%s%s%s%s%s%s%d%s%d",
						    sp->rank,           sw->customOutputDelimiter,
						    sp->indexf->line,   sw->customOutputDelimiter,
						    sp->filename,       sw->customOutputDelimiter,
						    sp->ISOTime,        sw->customOutputDelimiter,
						    sp->title,          sw->customOutputDelimiter,
						    sp->summary,        sw->customOutputDelimiter,
						    sp->start,          sw->customOutputDelimiter,
						    sp->size);
					else
						printf("%d%s%s%s%s%s%d",
						    sp->rank,           sw->customOutputDelimiter,
						    sp->filename,       sw->customOutputDelimiter,
						    sp->title,          sw->customOutputDelimiter,
						    sp->size);
				}
				else
				{
					if(extended_info)
						printf("%d %s %s \"%s\" \"%s\" \"%s\" %d %d",
						    sp->rank,
						    sp->indexf->line,
						    sp->filename,
						    sp->ISOTime,
						    sp->title,
						    sp->summary,
						    sp->start,
						    sp->size);
					else
						printf("%d %s \"%s\" %d",
						    sp->rank,
						    sp->filename,
						    sp->title,
						    sp->size);
				}
				printSearchResultProperties(sw, sp->Prop);
				printf("\n");
				if (resultmaxhits > 0) resultmaxhits--; /* Modified DN 08/29/99  */
			}
		}
		if(sw->beginhits) sw->beginhits--;
		sp = sp->nextsort;
	}
}

/* Does an index file have a readable format?
*/

int isokindexheader(fp)
FILE *fp;
{
long swish_magic;
	fseek(fp, 0, 0);
	swish_magic=readlong(fp);
	if(swish_magic!=SWISH_MAGIC) {
		fseek(fp, 0, 0);
		return 0;
	}
	fseek(fp, 0, 0);
	return 1;
}


/* Checks if the next word is "="
*/

int isMetaName (searchWord)
struct swline* searchWord;
{
	if (searchWord == NULL)
		return 0;
	if (!strcmp(searchWord->line, "=") )
		return 1;
	return 0;
}

/* funtion to free all memory of a list of results */
void freeresultlist(sw)
SWISH *sw;
{
RESULT *rp;
RESULT *tmp;
	rp=sw->resultlist;
	while(rp) {
		tmp = rp->next;
		freeresult(sw,rp);
		rp =tmp;
	}
	sw->resultlist=NULL;
}

/* funtion to free the memory of one result */
void freeresult(sw,rp)
SWISH *sw;
RESULT *rp;
{
int i;
	if(rp) 
	{
		if(rp->position) efree(rp->position);
		if(rp->title && rp->title!=rp->filename) efree(rp->title); 
		if(rp->filename) efree(rp->filename); 
		if(rp->summary) efree(rp->summary); 
		if(sw->numPropertiesToDisplay && rp->Prop) {
			for(i=0;i<sw->numPropertiesToDisplay;i++) 
				efree(rp->Prop[i]);
			efree(rp->Prop);
		}
		if(sw->numPropertiesToSort && rp->PropSort) {
			for(i=0;i<sw->numPropertiesToSort;i++) 
				efree(rp->PropSort[i]);
			efree(rp->PropSort);
		}
		efree(rp);
	}
}

/* 
04/00 Jose Ruiz - Sort results by rank
Uses an array and qsort for better performance
*/
RESULT *sortresultsbyrank(sw, structure)
SWISH *sw;
int structure;
{ 
int i, j;
unsigned char *ptmp,*ptmp2;
int *pi;
RESULT *pv;
RESULT *rtmp;
RESULT *sortresultlist;
RESULT *rp;
		rp=sw->resultlist;
	              /* Very trivial case */
		if(!rp) return NULL;
			/* Compute results */
		for(i=0,rtmp=rp;rtmp;rtmp = rtmp->next) 
			if (rtmp->structure & structure) i++;
	              /* Another very trivial case */
		if (!i) return NULL;
			/* Compute array wide */
		sortresultlist = NULL;
		j=sizeof(int)+sizeof(void *);
			/* Compute array size */
		ptmp=(void *)emalloc(j*i);
			/* Build an array with the elements to compare
				 and pointers to data */
		for(ptmp2=ptmp,rtmp=rp;rtmp;rtmp = rtmp->next) 
			if (rtmp->structure & structure) {
				pi=(int *)ptmp2;
				pi[0] = rtmp->rank;
				ptmp2+=sizeof(int);
				memcpy((char *)ptmp2,(char *)&rtmp,sizeof(RESULT *));
				ptmp2+=sizeof(void *);
			}
			/* Sort them */
		qsort(ptmp,i,j,&icomp);
			/* Build the list */
		for(j=0,ptmp2=ptmp;j<i;j++){
				pi=(int *)ptmp2;
				ptmp2+=sizeof(int);
				memcpy((char *)&pv,(char*)ptmp2,sizeof(RESULT *));
				ptmp2+=sizeof(void *);
				sortresultlist = (RESULT *) addsortresult(sw, sortresultlist, pv);
		}
			/* Free the memory od the array */
		efree(ptmp);
		return sortresultlist;
}

/* 
06/00 Jose Ruiz - Sort results by filenum
Uses an array and qsort for better performance
Used for faster "and" and "phrase" of results
*/
RESULT *sortresultsbyfilenum(rp)
RESULT *rp;
{ 
int i, j;
unsigned char *ptmp,*ptmp2;
int *pi;
RESULT *pv;
RESULT *rtmp;
	              /* Very trivial case */
		if(!rp) return NULL;
			/* Compute results */
		for(i=0,rtmp=rp;rtmp;rtmp = rtmp->next,i++);
	              /* Another very trivial case */
		if (i==1) return rp;
			/* Compute array wide */
		j=sizeof(int)+sizeof(void *);
			/* Compute array size */
		ptmp=(void *)emalloc(j*i);
			/* Build an array with the elements to compare
				 and pointers to data */
		for(ptmp2=ptmp,rtmp=rp;rtmp;rtmp = rtmp->next) {
			pi=(int *)ptmp2;
			pi[0] = rtmp->filenum;
			ptmp2+=sizeof(int);
			memcpy((char *)ptmp2,(char *)&rtmp,sizeof(RESULT *));
			ptmp2+=sizeof(void *);
		}
			/* Sort them */
		qsort(ptmp,i,j,&icomp);
			/* Build the list */
		for(j=0,rp=NULL,ptmp2=ptmp;j<i;j++){
			pi=(int *)ptmp2;
			ptmp2+=sizeof(int);
			memcpy((char *)&pv,(char*)ptmp2,sizeof(RESULT *));
			ptmp2+=sizeof(void *);
			if(!rp)rp=pv;
			else 
				rtmp->next=pv;
			rtmp=pv;
			
		}
		rtmp->next=NULL;
			/* Free the memory of the array */
		efree(ptmp);
		return rp;
}

RESULT *getproperties(sw,indexf,rp)
SWISH *sw;
IndexFILE *indexf;
RESULT *rp;
{
RESULT *tmp;
struct file *fileInfo;

	tmp = rp;
	while(tmp) {
		fileInfo = readFileEntry(indexf, tmp->filenum);
		tmp->filename=estrdup(fileInfo->fi.filename);

  		strftime(tmp->ISOTime,sizeof(tmp->ISOTime),"%Y/%m/%d %H:%M:%S",(struct tm *)localtime((time_t *)&fileInfo->fi.mtime));

			/* Just to save some little memory */
		if(fileInfo->fi.filename==fileInfo->fi.title)
			tmp->title=tmp->filename;
		else
			tmp->title=estrdup(fileInfo->fi.title);
		if(!fileInfo->fi.summary) 
			tmp->summary=estrdup("");
		else 
			tmp->summary=estrdup(fileInfo->fi.summary);
		tmp->start=fileInfo->fi.start;
		tmp->size=fileInfo->fi.size;
		if (sw->numPropertiesToDisplay)
			tmp->Prop=getResultProperties(sw,indexf,fileInfo->docProperties);
		if (sw->numPropertiesToSort)
			tmp->PropSort=getResultSortProperties(sw,indexf,fileInfo->docProperties);
		tmp = tmp->next;
	}
	return rp;
}

/* 06/00 Jose Ruiz
** returns all results in r1 that not contains r2 */
RESULT *notresultlists(sw, r1, r2)
SWISH *sw;
RESULT *r1;
RESULT *r2;
{
RESULT *tmpnode, *newnode, *r1b, *r2b;
int res=0;
int *allpositions;

	if (!r1 ) return NULL;
	if (r1 && !r2 ) return r1;
	
	newnode = NULL;
	r1=sortresultsbyfilenum(r1);
	r2=sortresultsbyfilenum(r2);
	/* Jose Ruiz 04/00 -> Preserve r1 and r2 for further proccesing */
	r1b = r1;
	r2b = r2;
	
	for(;r1 && r2;) {
		res=r1->filenum - r2->filenum;
		if(res<0) {
                        /*
                        * Storing all positions could be useful
                        * in the future
                        */
			allpositions=(int *)emalloc((r1->frequency)*sizeof(int));
			CopyPositions(allpositions,0,r1->position,0,r1->frequency);

			newnode = (RESULT *) addtoresultlist(newnode, r1->filenum, r1->rank, r1->structure , r1->frequency, allpositions,r1->indexf);
			r1 = r1->next;
		} else if(res>0) {
			r2 = r2->next;
		} else {
			r1 = r1->next;
			r2 = r2->next;
		}
	}
		/* Add remaining results */
	for(;r1;r1=r1->next) {
		allpositions=(int *)emalloc((r1->frequency)*sizeof(int));
		CopyPositions(allpositions,0,r1->position,0,r1->frequency);
		newnode = (RESULT *) addtoresultlist(newnode, r1->filenum, r1->rank, r1->structure , r1->frequency, allpositions,r1->indexf);
	}
			/* Free memory no longer needed */
	while (r1b) {
		tmpnode = r1b->next;
		freeresult(sw,r1b);
		r1b = tmpnode;
	}
	while (r2b) {
		tmpnode = r2b->next;
		freeresult(sw,r2b);
		r2b = tmpnode;
	}
	return newnode;
}

void freefileoffsets(SWISH *sw)
{
int i;
IndexFILE *tmp=sw->indexlist;
	while(tmp)
	{
        	if(tmp->filearray) {
                	for(i=0;i<tmp->filearray_cursize;i++) 
			{
				if(tmp->filearray[i])
					freefileinfo(tmp->filearray[i]);
			}
              		efree(tmp->filearray);
               		tmp->filearray=NULL;
               		tmp->filearray_maxsize=tmp->filearray_cursize=0;
        	}
	        if(tmp->fileoffsetarray) {
       		        for(i=0;i<tmp->fileoffsetarray_cursize;i++) 
			{
				if(tmp->fileoffsetarray[i])
					efree(tmp->fileoffsetarray[i]);
			}
                	efree(tmp->fileoffsetarray);
               		tmp->fileoffsetarray=NULL;
                        tmp->fileoffsetarray_maxsize=tmp->fileoffsetarray_cursize=0;
		}
		tmp=tmp->next;
        }
}

void freefileinfo(struct file *f)
{
	if(f->fi.title && f->fi.title!=f->fi.filename) efree(f->fi.title);
	if(f->fi.filename) efree(f->fi.filename);
	if(f->fi.summary) efree(f->fi.summary);
	if(f->docProperties) freeDocProperties(&f->docProperties);
	efree(f);
}

struct swline *ignore_words_in_query(sw,indexf,searchwordlist)
SWISH *sw;
IndexFILE *indexf;
struct swline *searchwordlist;
{
struct swline *pointer1, *pointer2, *pointer3;
	/* Added JM 1/10/98. */
	/* completely re-written 2/25/00 - SRE - "ted and steve" --> "and steve" if "ted" is stopword --> no matches! */

	/* walk the list, looking for rules & stopwords to splice out */
	/* remove a rule ONLY if it's the first thing on the line */
	/*   (as when exposed by removing stopword that comes before it) */

	/* loop on FIRST word: quit when neither stopword nor rule (except NOT rule) or metaname (last one as suggested by Adrian Mugnolo) */
	pointer1 = searchwordlist;
	while (pointer1 != NULL) {
		pointer2 = pointer1->next;
			/* 05/00 Jose Ruiz
			** NOT rule is legal at begininig */
		if(isnotrule(pointer1->line) || isMetaName(pointer2)) break;
		if(!isstopword(indexf,pointer1->line) && !isrule(pointer1->line)) break;
		searchwordlist = pointer2; /* move the head of the list */
		printf("# Removed stopword: %s\n",pointer1->line);
			 /* Free line also !! Jose Ruiz 04/00 */
		efree(pointer1->line);
		efree(pointer1); /* toss the first point */
			 /* Free line also !! Jose Ruiz 04/00 */
		pointer1 = pointer2; /* reset for the loop */
	}
	if (!pointer1) {
		sw->lasterror=WORDS_TOO_COMMON;
		return NULL;
	}

	/* loop on REMAINING words: ditch stopwords but keep rules (unless two rules in a row?) */
	pointer2 = pointer1->next;
	while (pointer2 != NULL) {
		/* Added Patch from Adrian Mugnolo */
		if((isstopword(indexf,pointer2->line) && !isrule(pointer2->line) && !isMetaName(pointer2->next))    /* non-rule stopwords */
		|| (    isrule(pointer1->line) &&  isrule(pointer2->line))) { /* two rules together */
			printf("# Removed stopword: %s\n",pointer2->line);    /* keep 1st of 2 rule */
			pointer1->next = pointer2->next;
			pointer3 = pointer2->next;
				/* Jose Ruiz 04/00
				** Fix memory problem
				*/
			efree(pointer2->line);
			efree(pointer2);
			pointer2 = pointer3;
		} else {
			pointer1 = pointer1->next;
			pointer2 = pointer2->next;
		}
		/* Jose Ruiz 04/00
		** Removed!! If pointer2 was previously freed 
		** we must not reassign it contents here
		** pointer2 = pointer2->next;
		*/
	}
	return searchwordlist;
}
