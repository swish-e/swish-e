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
** 2001-03-13 rasc  definable logical operators via  sw->srch_op.[and|or|nor]
**                  bugfix in parse_search_string handling...
** 2001-03-14 rasc  resultHeaderOutput  -H <n>
**
*/

#include "swish.h"
#include "search.h"
#include "index.h"
#include "string.h"
#include "file.h"
#include "list.h"
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
#include "result_sort.h"
#include "result_output.h"
#include "altavista.h"


/* 01/2001 Jose Ruiz */
/* Compare RESULTS using RANK */
/* This routine is used by qsort */
int compResultsByFileNum(const void *s1,const void *s2)
{
	return((*(RESULT* const*)s1)->filenum - (*(RESULT* const*)s2)->filenum);
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

int SwishAttach(SWISH *sw,int printflag)
{
IndexFILE *indexlist;
/* 06/00 Jose Ruiz
** Added to handle several index file headers */
IndexFILE *tmplist;

	indexlist=sw->indexlist;	
	sw->TotalWords=0;
	sw->TotalFiles=0;
	

	/* First of all . Read header default values from all index fileis */
	/* With this, we read wordchars, stripchars, ... */
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

		sw->TotalWords+=tmplist->header.totalwords;
		sw->TotalFiles+=tmplist->header.totalfiles;
		tmplist=tmplist->next;
	}
	
	/* Make lookuptables for char processing */
	return (sw->lasterror=RC_OK);
}


/*
  -- Search Swish 
  -- Check if AltaVista like search string has to be converted
  -- and call swish search...
  -- 2001-03-02 rasc
*/

int search(SWISH *sw, char *words, int structure)
{
  char *sw_srch_str;
  int  ret;


   if (sw->enableAVSearchSyntax) {	/* AltaVista like search enabled? */
	sw_srch_str = convAltaVista2SwishStr (words);
	ret = search_2 (sw, sw_srch_str, structure);
	efree (sw_srch_str);
   } else {
	ret = search_2 (sw, words, structure);
   }
   return ret;  
}



int search_2 (SWISH *sw, char *words, int structure)
{
int j,k, hassearch, metaID, indexYes, totalResults;
struct swline *searchwordlist, *tmplist2;
IndexFILE *indexlist;
int rc=0;
unsigned char PhraseDelimiter;
char  *tmpwords;
struct DB_RESULTS *db_results,*db_tmp;

			/* If not words - do nothing */
	if (!words || !*words) 
		return(sw->lasterror=NO_WORDS_IN_SEARCH);

	PhraseDelimiter = (unsigned char)sw->PhraseDelimiter;

	indexlist=sw->indexlist;
	sw->db_results=NULL;
	j=0;
	searchwordlist = NULL;
	metaID = 1;
	indexYes =0;
	totalResults=0;
	hassearch=0;

	sw->lasterror=RC_OK;
	sw->commonerror = RC_OK;

	if ((rc=initSearchResultProperties(sw))) return rc;
	if ((rc=initSortResultProperties(sw))) return rc;

	while (indexlist != NULL) {
		tmpwords = estrdup (words);   /* copy of the string  (2001-03-13 rasc) */

		if(searchwordlist)
		{
			freeswline(searchwordlist);
			searchwordlist=NULL;
		}
		if(!(searchwordlist = parse_search_string(sw,tmpwords,indexlist->header)))
		{
			indexlist=indexlist->next;
			efree (tmpwords);
			continue;
		}

		hassearch = 1;

		sw->bigrank = 0;
		
		if (!indexlist->fp) {
			efree (tmpwords);
			if (searchwordlist) freeswline(searchwordlist);   /* 2001-03-13 rasc */ 
			return sw->lasterror;
		}

		if (!indexlist->header.totalfiles) {
			indexlist = indexlist->next;
			efree (tmpwords);
			continue;
		} else {
		 indexYes = 1; /*There is a non-empty index */
		}



		searchwordlist = (struct swline *)translatechars_words_in_query(sw,indexlist,searchwordlist);
#ifdef IGNORE_STOPWORDS_IN_QUERY
		searchwordlist = (struct swline *)ignore_words_in_query(sw,indexlist,searchwordlist ,PhraseDelimiter);
#endif /* IGNORE_STOPWORDS_IN_QUERY */
		if(indexlist->header.applyStemmingRules)
			searchwordlist = stem_words_in_query(sw,indexlist,searchwordlist );
		if(indexlist->header.applySoundexRules)
			searchwordlist = soundex_words_in_query(sw,indexlist,searchwordlist );
			/* Echo index file, fixed search, stopwords */

		/* Result Header Output  (2001-03-14 rasc,  rewritten) */

		resultHeaderOut(sw,2, "# Index File: %s\n",indexlist->line);
		resultPrintHeader(sw,2, &indexlist->header,indexlist->header.savedasheader,0);
		resultHeaderOut(sw,3,"# StopWords:");
		for (k=0;k<indexlist->stopPos;k++)
			resultHeaderOut(sw,3, " %s",indexlist->stopList[k]);
		resultHeaderOut(sw,3, "\n");
		resultHeaderOut(sw,2, "# Search Words: %s\n",words);
		resultHeaderOut(sw,2, "# Parsed Words: ");
		tmplist2=searchwordlist; 
		while(tmplist2)
		{
			resultHeaderOut(sw,2, "%s ",tmplist2->line);
			tmplist2=tmplist2->next;
		}
		resultHeaderOut(sw,2, "\n");


		/* Expand phrase search: "kim harlow" becomes (kim PHRASE_WORD harlow) */
		searchwordlist = (struct swline *) expandphrase(searchwordlist,PhraseDelimiter);
		searchwordlist = (struct swline *) fixnot(searchwordlist); 

		/* Allocate memory for the result list structure */
		db_results=(struct DB_RESULTS *) emalloc(sizeof(struct DB_RESULTS));
		db_results->currentresult=NULL;
		db_results->sortresultlist=NULL;
		db_results->resultlist=NULL;
		db_results->next=NULL;

		if(searchwordlist)
		{
			tmplist2=searchwordlist;
			db_results->resultlist = (RESULT *) parseterm(sw,0,metaID,indexlist,&tmplist2);
		}

			/* add db_results to the list of results */
		if(!sw->db_results)
			sw->db_results = db_results;
		else {
			db_tmp=sw->db_results;
			while(db_tmp) {
				if(!db_tmp->next) {
					db_tmp->next=db_results;
					break;
				}
				db_tmp=db_tmp->next;
			}

		}


	  efree (tmpwords);
	  indexlist = indexlist->next;

	} /* while */

	if(!hassearch)
	{
		return(sw->lasterror=NO_WORDS_IN_SEARCH);
	}

	if(searchwordlist)
	{
		freeswline(searchwordlist);
		searchwordlist=NULL;
	}

/* 
04/00 Jose Ruiz - Sort results by rank or by properties
*/
	totalResults=sortresults(sw, structure);

	if (!totalResults && sw->commonerror) return (sw->lasterror=WORDS_TOO_COMMON);
	if (!totalResults && !indexYes) return (sw->lasterror=INDEX_FILE_IS_EMPTY);

	return totalResults;
}


/* This puts parentheses in the right places around not structures
** so the parser can do its thing correctly.
** It does it both for 'not' and '='; the '=' is used for the METADATA (GH)
*/

struct swline *fixnot(struct swline *sp)
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
		if (isMetaNameOpNext(tmpp->next)) {
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
		if (isMetaNameOpNext(tmpp)) {
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

// $$$ to be rewritten as function = smaller code (rasc)

#define ReadHeaderStr(buffer,bufferlen,len,fp)  uncompress1(len,fp); if(bufferlen<len) buffer=erealloc(buffer,(bufferlen=len+200)+1);  fread(buffer,len,1,fp);
#define ReadHeaderInt(itmp,fp) uncompress1(itmp,fp); itmp--;

void readheader(IndexFILE *indexf)
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
				makelookuptable(indexf->header.wordchars,indexf->header.wordcharslookuptable);
				break;
			case BEGINCHARSHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.beginchars = SafeStrCopy(indexf->header.beginchars,buffer,&indexf->header.lenbeginchars);
				sortstring(indexf->header.beginchars);
				makelookuptable(indexf->header.beginchars,indexf->header.begincharslookuptable);
				break;
			case ENDCHARSHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.endchars = SafeStrCopy(indexf->header.endchars,buffer,&indexf->header.lenendchars);
				sortstring(indexf->header.endchars);
				makelookuptable(indexf->header.endchars,indexf->header.endcharslookuptable);
				break;
			case IGNOREFIRSTCHARHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.ignorefirstchar = SafeStrCopy(indexf->header.ignorefirstchar,buffer,&indexf->header.lenignorefirstchar);
				sortstring(indexf->header.ignorefirstchar);
				makelookuptable(indexf->header.ignorefirstchar,indexf->header.ignorefirstcharlookuptable);
				break;
			case IGNORELASTCHARHEADER_ID:
				ReadHeaderStr(buffer,bufferlen,len,fp);
				indexf->header.ignorelastchar = SafeStrCopy(indexf->header.ignorelastchar,buffer,&indexf->header.lenignorelastchar);
				sortstring(indexf->header.ignorelastchar);
				makelookuptable(indexf->header.ignorelastchar,indexf->header.ignorelastcharlookuptable);
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

			case TRANSLATECHARTABLE_ID:
				ReadHeaderLookupTable (indexf->header.translatecharslookuptable, sizeof(indexf->header.translatecharslookuptable)/sizeof(int),fp);
				break;

			default:
				progerr("Severe index error in header");
				break;
		}
		uncompress1(id,fp);
	}
	efree(buffer);
}


/*
  -- some support functions reading header data
*/


void ReadHeaderLookupTable (int table[], int table_size, FILE *fp)
{
 int i,x;

 for (i=0; i<table_size; i++) {
   uncompress1 (x,fp);
   table[i]=x-1;
 }
}



/* Reads the offsets in the index file so word lookup is faster.
The file pointer is set by readheader */


void readoffsets(IndexFILE *indexf)
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
void readhashoffsets(IndexFILE *indexf)
{
int i;
	for (i=0;i<SEARCHHASHSIZE;i++) 
		indexf->hashoffsets[i] = readlong(indexf->fp);
		/* start of words in index file */
	indexf->wordpos=ftell(indexf->fp);
}

/* Reads the stopwords in the index file.
*/

void readstopwords(IndexFILE *indexf)
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

void readMetaNames(IndexFILE *indexf)
{
int len;
int dummy;
int wordlen,metaType,metaID;
char *word;
long sort_offset;
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
			/* Read metaID */
		uncompress1(metaID,fp);
			/* metaType was saved as metaType+1 */
		uncompress1(metaType,fp);
		metaType--;
			/* Read offset to sorted table of filenums */
		sort_offset=readlong(fp);
			/* add the meta tag */
		addMetaEntry(indexf, word, metaType, metaID, sort_offset, &dummy); 

		uncompress1(len,fp);
	}
	efree(word);
}

/* Reads the file offset table in the index file.
*/

void readfileoffsets(IndexFILE *indexf)
{
long pos, totwords;
FILE *fp=indexf->fp;
	indexf->filearray_maxsize = indexf->fileoffsetarray_maxsize = indexf->header.totalfiles;
	indexf->filearray=(struct file **)emalloc(indexf->filearray_maxsize*sizeof(struct file *));
	indexf->fileoffsetarray=(long *)emalloc(indexf->fileoffsetarray_maxsize*sizeof(long));
	indexf->filetotalwordsarray=(int *)emalloc(indexf->fileoffsetarray_maxsize*sizeof(int));
	fseek(fp,indexf->offsets[FILEOFFSETPOS], 0);
	for (indexf->filearray_cursize=0,indexf->fileoffsetarray_cursize=0,pos=1L;(pos = readlong(fp));indexf->filearray_cursize++,indexf->fileoffsetarray_cursize++) {
		if(indexf->filearray_cursize==indexf->filearray_maxsize) 
		{
			indexf->filearray=(struct file **)erealloc(indexf->filearray,(indexf->filearray_maxsize+=1000)*sizeof(struct file *));
		}
		if(indexf->fileoffsetarray_cursize==indexf->fileoffsetarray_maxsize) 
		{
			indexf->fileoffsetarray=(long *)erealloc(indexf->fileoffsetarray,(indexf->fileoffsetarray_maxsize+=1000)*sizeof(long));
			indexf->filetotalwordsarray=(int *)erealloc(indexf->filetotalwordsarray,(indexf->fileoffsetarray_maxsize+=1000)*sizeof(int));
		}
		totwords= readlong(fp);
		indexf->filearray[indexf->filearray_cursize]=NULL;
		indexf->fileoffsetarray[indexf->fileoffsetarray_cursize]=pos;
		indexf->filetotalwordsarray[indexf->fileoffsetarray_cursize]=totwords;
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

RESULT *parseterm(SWISH *sw, int parseone, int metaID, IndexFILE *indexf, struct swline **searchwordlist)
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
			rulenum = sw->srch_op.defaultrule;
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
		if ( isMetaNameOpNext((*searchwordlist)->next) ) {
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

RESULT *operate(SWISH *sw, RESULT *rp, int rulenum, char *wordin, FILE *fp, int metaID, int andLevel, IndexFILE *indexf)
{
/* int i, found; indexchars stuff removed */
RESULT *newrp, *returnrp;
char *word;
int lenword;
	word=estrdup(wordin);
	lenword=strlen(word);

	newrp = returnrp = NULL;

	if (isstopword(indexf,word) && !isrule(word)) 
	{
		if (rulenum == OR_RULE && rp != NULL)
			return rp;
		else
			sw->commonerror = 1;
	}

/*>>>>
        i=(int)((unsigned char)word[0]);
        found=isindexchar(indexf->header,i);
	
	if (!found) 
	{
		if (rulenum == AND_RULE || rulenum == PHRASE_RULE)
			return NULL;
		else if (rulenum == OR_RULE)
			return rp;
	}
indexchars stuff removed
<<<< */
	
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
RESULT *getfileinfo(SWISH *sw, char *word, IndexFILE *indexf, int metaID)
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
				rp = (RESULT *) addtoresultlist(rp, filenum, getrank(sw, frequency, tfrequency,indexf->filetotalwordsarray[filenum-1],structure,indexf->header.ignoreTotalWordCountWhenRanking), structure,frequency,position,indexf,sw);
				if (sw->opt.headerOutVerbose >= 9)
				{
					/* dump diagnostic info */
					long curFilePos;
					curFilePos = ftell(fp);	/* save */
					fi = readFileEntry(indexf, filenum);
					resultHeaderOut(sw,9, "# diag\tFILE: %s\tWORD: %s\tRANK: %d\tFREQUENCY: %d\t HASH ITEM: %d\n", fi->fi.filename, word, getrank(sw, frequency, tfrequency,indexf->filetotalwordsarray[filenum-1],structure,indexf->header.ignoreTotalWordCountWhenRanking), frequency, tries);
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
			        rp->frequency, rp->position,indexf,sw);
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
char *getfilewords(SWISH *sw, char c, IndexFILE *indexf)
{
int i,j;
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


/*
  -- Rules checking
  -- u_is...  = user rules (and, or, ...)
  -- is...    = internal rules checking
 */


/* Is a word a rule?
*/

int isrule(char *word)
{
  int i;
  static char *ops[] = {  /* internal ops... */
		AND_WORD, OR_WORD, NOT_WORD, PHRASE_WORD, AND_NOT_WORD,
		NULL };

  for (i=0; ops[i]; i++) {
     if (! strcmp (word,ops[i]) ) return 1;
  }
  return 0;
}

int u_isrule(SWISH *sw, char *word)
{
 LOGICAL_OP *op;

  op = &(sw->srch_op);
  if (!strcmp(word, op->and) || !strcmp(word, op->or) || !strcmp(word, op->not))
	return 1;
  else
	return 0;
}

int isnotrule(char *word)
{
	if (!strcmp(word,NOT_WORD) )
		return 1;
	else
		return 0;
}

int u_isnotrule(SWISH *sw, char *word)
{
	if (!strcmp(word,sw->srch_op.not) )
		return 1;
	else
		return 0;
}


/* Is a word a boolean rule?
*/

int isbooleanrule(char *word)
{
	if (!strcmp(word, AND_WORD)    || !strcmp(word, OR_WORD) ||
	    !strcmp(word, PHRASE_WORD) || !strcmp(word, AND_NOT_WORD))
		return 1;
	else
		return 0;
}

/* Is a word a unary rule?
*/

int isunaryrule(char *word)
{
	if (!strcmp(word, NOT_WORD))
		return 1;
	else
		return 0;
}

/* Return the number for a rule.
*/

int getrulenum(char *word)
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

int u_SelectDefaultRulenum(SWISH *sw, char *word)
{
	if (!strcasecmp(word, sw->srch_op.and))
		return AND_RULE;
	else if (!strcasecmp(word, sw->srch_op.or))
		return OR_RULE;
	return NO_RULE;
}








/* Takes two lists of results from searches and ANDs them together.
*/

RESULT *andresultlists(SWISH *sw, RESULT *r1, RESULT *r2, int andLevel)
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
			newnode = (RESULT *) addtoresultlist(newnode, r1->filenum, newRank, r1->structure & r2->structure, r1->frequency + r2->frequency, allpositions,r1->indexf,sw);
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

RESULT *orresultlists(SWISH *sw, RESULT *r1, RESULT *r2)
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
				rp->frequency, rp->position, rp->indexf,sw);
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

RESULT *notresultlist(SWISH *sw, RESULT *rp, IndexFILE *indexf)
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
			newp = (RESULT *) addtoresultlist(newp, i, 1000, IN_ALL,0,NULL,indexf,sw);
	}
	
	return newp;
}

RESULT *phraseresultlists(SWISH *sw, RESULT *r1, RESULT *r2, int distance)
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
				newnode = (RESULT *) addtoresultlist(newnode, r1->filenum, newRank, r1->structure & r2->structure, found, allpositions,r1->indexf,sw);
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

RESULT *addtoresultlist(RESULT *rp, int filenum, int rank, int structure, int frequency, int *position,IndexFILE *indexf,SWISH *sw)
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
	newnode->iPropSort = NULL;
	newnode->indexf = indexf;
	newnode->read = 0;
	newnode->sw = (struct SWISH *)sw;
	
	if (rp == NULL)
		rp = newnode;
	else
		rp->head->next = newnode;
	
	rp->head = newnode;
	
	return rp;
}


/* Counts the number of files that are the result
   of a search
*/

int countResults(RESULT *sp)
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
RESULT *res=NULL;
RESULT *res2=NULL;
int rc;
struct DB_RESULTS *db_results=NULL,*db_results_winner=NULL;
double  num;

	if (sw->bigrank) num = 1000.0f / (float) sw->bigrank;
	else num = 1000.0f;

		/* Check for a unique index file */
	if(!sw->db_results->next)
	{
		if((res = sw->db_results->currentresult))
		{
			/* Increase Pointer */
			sw->db_results->currentresult = res->nextsort;

			/* 02/2001 jmruiz - Read file data here */
			/* Doing it here we get better performance if maxhits specified
				and not all the results data (only read maxhits) */
			res=getproperties(res);
		}
	}
	else
	{
		/* We have more than one index file */
		/* Get the lower value */
		db_results_winner=sw->db_results;
		if((res=db_results_winner->currentresult)) 
		{
			res=getproperties(res);
			if(!res->PropSort)
				res->PropSort=getResultSortProperties(res);
		}

		for(db_results=sw->db_results->next;db_results;db_results=db_results->next)
		{
			if(!(res2=db_results->currentresult)) continue;
			else 
			{
				res2=getproperties(res2);
				if(!res2->PropSort)
					res2->PropSort=getResultSortProperties(res2);
			}
			if(!res) 
			{
				res=res2;
				db_results_winner=db_results;
				continue;
			}
			rc=(int)compResultsByNonSortedProps(&res,&res2);
			if(rc<0) 
			{
				res=res2;
				db_results_winner=db_results;
			}
		}
		if((res=db_results_winner->currentresult))
			db_results_winner->currentresult=res->nextsort;
	}
		/* Normalize rank */
	if(res) {
		res->rank = (int) ((float) res->rank * num);
		if (res->rank >= 999) res->rank = 1000;
		else if (res->rank <1) res->rank = 1;
	} 
	else
	{
		sw->lasterror=SWISH_LISTRESULTS_EOF;
	}
	return res;
	
}


/* Does an index file have a readable format?
*/

int isokindexheader(FILE *fp)
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

int isMetaNameOpNext (struct swline *searchWord)
{
	if (searchWord == NULL)
		return 0;
	if (!strcmp(searchWord->line, "=") )
		return 1;
	return 0;
}

/* funtion to free all memory of a list of results */
void freeresultlist(SWISH *sw,struct DB_RESULTS *dbres)
{
RESULT *rp;
RESULT *tmp;
	rp=dbres->resultlist;
	while(rp) {
		tmp = rp->next;
		freeresult(sw,rp);
		rp =tmp;
	}
	dbres->resultlist=NULL;
	dbres->currentresult=NULL;
	dbres->sortresultlist=NULL;
}

/* funtion to free the memory of one result */
void freeresult(SWISH *sw,RESULT *rp)
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
		if(sw->numPropertiesToSort && rp->iPropSort) {
			efree(rp->iPropSort);
		}

		efree(rp);
	}
}



/* 
06/00 Jose Ruiz - Sort results by filenum
Uses an array and qsort for better performance
Used for faster "and" and "phrase" of results
*/
RESULT *sortresultsbyfilenum(RESULT *rp)
{ 
int i, j;
RESULT **ptmp;
RESULT *rtmp;
	              /* Very trivial case */
		if(!rp) return NULL;
			/* Compute results */
		for(i=0,rtmp=rp;rtmp;rtmp = rtmp->next,i++);
	              /* Another very trivial case */
		if (i==1) return rp;
			/* Compute array size */
		ptmp=(void *)emalloc(i*sizeof(RESULT *));
			/* Build an array with the elements to compare
				 and pointers to data */
		for(j=0,rtmp=rp;rtmp;rtmp = rtmp->next) ptmp[j++]=rtmp;
			/* Sort them */
		qsort(ptmp,i,sizeof(RESULT *),&compResultsByFileNum);
			/* Build the list */
		for(j=0,rp=NULL;j<i;j++){
			if(!rp)rp=ptmp[j];
			else 
				rtmp->next=ptmp[j];
			rtmp=ptmp[j];
			
		}
		rtmp->next=NULL;
			/* Free the memory of the array */
		efree(ptmp);
		return rp;
}

RESULT *getproperties(RESULT *rp)
{
IndexFILE *indexf=rp->indexf;
SWISH *sw=(SWISH *)rp->sw;
struct file *fileInfo;
	if(rp->read) return rp;    /* Read it before */

	fileInfo = readFileEntry(indexf, rp->filenum);
	rp->read=1;
	rp->filename=estrdup(fileInfo->fi.filename);

	rp->last_modified = fileInfo->fi.mtime;

		/* Just to save some little memory */
	if(fileInfo->fi.filename==fileInfo->fi.title)
		rp->title=rp->filename;
	else
		rp->title=estrdup(fileInfo->fi.title);
	if(!fileInfo->fi.summary) 
		rp->summary=estrdup("");
	else 
		rp->summary=estrdup(fileInfo->fi.summary);
	rp->start=fileInfo->fi.start;
	rp->size=fileInfo->fi.size;
	if (sw->numPropertiesToDisplay)
		rp->Prop=getResultProperties(rp);

	return rp;
}

/* 06/00 Jose Ruiz
** returns all results in r1 that not contains r2 */
RESULT *notresultlists(SWISH *sw, RESULT *r1, RESULT *r2)
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

			newnode = (RESULT *) addtoresultlist(newnode, r1->filenum, r1->rank, r1->structure , r1->frequency, allpositions,r1->indexf,sw);
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
		newnode = (RESULT *) addtoresultlist(newnode, r1->filenum, r1->rank, r1->structure , r1->frequency, allpositions,r1->indexf,sw);
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
               		efree(tmp->fileoffsetarray);
               		efree(tmp->filetotalwordsarray);
           		tmp->fileoffsetarray=NULL;
			tmp->filetotalwordsarray=NULL;
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

/* 02/2001 Jose Ruiz */
/* Partially rewritten to consider phrase search and "and" "or" and "not" as stopwords */

struct swline *ignore_words_in_query(SWISH *sw,IndexFILE *indexf,struct swline *searchwordlist,unsigned char phrase_delimiter)
{
struct swline *pointer1, *pointer2, *pointer3;
int inphrase=0,ignore=0;
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
		if(pointer1->line[0]==phrase_delimiter) break;
		if(u_isnotrule(sw,pointer1->line) || isMetaNameOpNext(pointer2)) break;
		if(!isstopword(indexf,pointer1->line) && !u_isrule(sw,pointer1->line)) break;
		searchwordlist = pointer2; /* move the head of the list */
		resultHeaderOut(sw,1, "# Removed stopword: %s\n",pointer1->line);
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
		if(pointer1->line[0]==phrase_delimiter)
		{
			if(inphrase) inphrase=0;
			else inphrase=1;
			pointer1 = pointer1->next;
			pointer2 = pointer2->next;
		} else {
			ignore=0;
			if(!inphrase)
			{
				if((isstopword(indexf,pointer2->line) && !u_isrule(sw,pointer2->line) && !isMetaNameOpNext(pointer2->next))    /* non-rule stopwords */
				|| (u_isrule(sw,pointer1->line) &&  u_isrule(sw,pointer2->line))) /* two rules together */
					ignore=1;
			} else {
				if(isstopword(indexf,pointer2->line)) 
					ignore=1;
			}
			if(ignore)
			{
				resultHeaderOut(sw,2, "# Removed stopword: %s\n",pointer2->line);    /* keep 1st of 2 rule */
				pointer1->next = pointer2->next;
				pointer3 = pointer2->next;
				efree(pointer2->line);
				efree(pointer2);
				pointer2 = pointer3;
			} else {
				pointer1 = pointer1->next;
				pointer2 = pointer2->next;
			}
		}
	}
	return searchwordlist;
}



struct swline *stem_words_in_query(SWISH *sw,IndexFILE *indexf,struct swline *searchwordlist)
{
struct swline *tmplist;
int len,lenword;
char *word,*tmp;

	tmplist = searchwordlist;
	while (tmplist != NULL) {
		if(!isrule(tmplist->line) && !isMetaNameOpNext(tmplist->next)) 
		{
			lenword=strlen(tmplist->line)+50;
			word = emalloc(lenword+1);
			strcpy(word,tmplist->line);
			len=strlen(word)-1;
			/* apply stemming algorithm to the search term */
			len=strlen(word)-1;
			if(len && word[len]=='*') word[len]='\0';
			else len=0; /* No star */
			Stem(&word,&lenword);
			if(len) 
			{
				tmp=emalloc(strlen(word)+2);
				strcpy(tmp,word);
				strcat(tmp,"*"); 
				efree(word);
				word=tmp;
			}
			efree(tmplist->line);
			tmplist->line=word;
		}
		tmplist=tmplist->next;
	}
	return searchwordlist;
}

struct swline *soundex_words_in_query(SWISH *sw,IndexFILE *indexf,struct swline *searchwordlist)
{
struct swline *tmplist;
int len,lenword;
char *word;

	tmplist = searchwordlist;
	while (tmplist != NULL) {
		if(!isrule(tmplist->line) && !isMetaNameOpNext(tmplist->next)) 
		{
	                /* apply soundex algorithm to the search term */
			/* Need to fix word length ? */
			lenword=strlen(tmplist->line)+50;
			word = emalloc(lenword+1);
			strcpy(word,tmplist->line);
			len=strlen(word)-1;
			if(len && word[len]=='*') word[len]='\0';
			else len=0; /* No star */
			soundex(word);   /* Need to fix word length ? */
			if(len && (strlen(word)-1)<MAXWORDLEN) 
				strcat(word,"*");	 
			efree(tmplist->line);
			tmplist->line=word;
		}
		tmplist=tmplist->next;
	}
	return searchwordlist;
}


/*
  -- do TranslateCharacters on SearchWords
  -- (to match translation of search words to indexed words)
  -- Words in searchwordlist itself are changed!
  -- 2001-02-22 rasc
*/

struct swline *translatechars_words_in_query(SWISH *sw,IndexFILE *indexf,struct swline *searchwordlist)
{
  struct swline *tmplist;
  int           *tr_lookup;


	tmplist = searchwordlist;
      tr_lookup = indexf->header.translatecharslookuptable;

	while (tmplist != NULL) {
		if(!isrule(tmplist->line) && !isMetaNameOpNext(tmplist->next)) {
                  TranslateChars (tr_lookup, tmplist->line);
		}
		tmplist=tmplist->next;
	}
	return searchwordlist;
}



struct swline *parse_search_string(SWISH *sw, char *words,INDEXDATAHEADER header)
{
struct swline *searchwordlist=NULL;
int i,j;
char word[MAXWORDLEN];
unsigned char PhraseDelimiter;
unsigned char PhraseDelimiterString[2];
	PhraseDelimiter = (unsigned char)sw->PhraseDelimiter;
	PhraseDelimiterString[0] = (unsigned char)PhraseDelimiter;
	PhraseDelimiterString[1] = '\0';

	for (i = j = 0; words[i] != '\0' && words[i] != '\n'; i++) 
	{
		/* 06/00 Jose ruiz
		** Following line modified to extract words according
		** to wordchars as suggested by Bill Moseley
		*/
		if (isspace((int)((unsigned char)words[i])) || words[i] == '(' || words[i] == ')' || (words[i] == '=') || (words[i] == ((unsigned char)PhraseDelimiter)) || !((words[i]=='*') || iswordchar(header,words[i]))) /* cast to int, 2/22/00 */
		{
			if (words[i] == '=')
			{
				if (j != 0)
				{
					if (words[i-1] != '\\')
					{ 
						word[j] = '\0';
						searchwordlist = (struct swline *) addswline(searchwordlist, word);
						j = 0;
						searchwordlist = (struct swline *) addswline(searchwordlist, "=");
					}
					else
					{
						/* Needs to erase the '\' */
						j--;
						word[j] = tolower((unsigned char)words[i]);
						j++;
					}
				}
				else
				{
					searchwordlist = (struct swline *) addswline(searchwordlist, "=");
				}
			}
			else
			{
				if (j) 
				{
					word[j] = '\0';
						/* Convert chars ignored in words to spaces  */
					stripIgnoreLastChars(header,word);
					stripIgnoreFirstChars(header,word);
					if(strlen(word))
					{
						searchwordlist = (struct swline *) addswline(searchwordlist, word);
					}
					j = 0;
				}
				if (words[i] == '(') 
				{
					searchwordlist = (struct swline *) addswline(searchwordlist, "(");
				}
				if (words[i] == ')') 
				{
					searchwordlist = (struct swline *) addswline(searchwordlist, ")");
				}
				if (words[i] == (unsigned char)PhraseDelimiter) 
				{
					searchwordlist = (struct swline *) addswline(searchwordlist, PhraseDelimiterString);
				}
			}
		}
		else 
		{
			word[j] = tolower((unsigned char)words[i]);
			j++;
		}
	}
	if (j) 
	{
		word[j] = '\0';
       /* Convert chars ignored in words to spaces  */
		stripIgnoreLastChars(header,word);
		stripIgnoreFirstChars(header,word);
		if(strlen(word))
		{
			searchwordlist = (struct swline *) addswline(searchwordlist, word);
		}
	}
	return searchwordlist;
}
