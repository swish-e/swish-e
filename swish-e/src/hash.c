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
**---------------------------------------------------------
** Added addStopList to support printing of common words
** G. Hill 4/7/97  ghill@library.berkeley.edu
**
** change sprintf to snprintf to avoid corruption
** SRE 11/17/99
**
** 04/00 - Jose Ruiz
** change hash for bighash in mergeresultlists for better performance
** when big searchs (a* or b* or c*)
**
*/

#include "swish.h"
#include "hash.h"
#include "search.h"
#include "mem.h"
#include "string.h"

/* Hashes a string.
*/

unsigned hash(s)
char *s;
{
unsigned hashval;
	
	for (hashval = 0; *s != '\0'; s++)
		hashval = (int)((unsigned char) *s) + 31 * hashval;
	return hashval % HASHSIZE;
}

/* Hashes a string for a larger hash table.
*/

unsigned bighash(s)
char *s;
{
unsigned hashval;
	
	for (hashval = 0; *s != '\0'; s++)
		hashval = (int)((unsigned char) *s) + 31 * hashval;
	return hashval % BIGHASHSIZE;
}

/* Hashes a int.
*/

unsigned numhash(i)
int i;
{
	return i % HASHSIZE;
}

/* Hashes a int for a larger hash table.
*/

unsigned bignumhash(i)
int i;
{
	return i % BIGHASHSIZE;
}

/* Hashes a string for a larger hash table (for search).
*/

unsigned searchhash(s)
char *s;
{
unsigned hashval;
	
	for (hashval = 0; *s != '\0'; s++)
		hashval = (int)((unsigned char) *s) + 31 * hashval;
	return hashval % SEARCHHASHSIZE;
}

/* Reads the internal list of default stopwords.
*/

void readdefaultstopwords(IndexFILE *indexf)
{
int i;
	
	for (i = 0; defaultstopwords[i] != NULL; i++)
		addstophash(indexf,defaultstopwords[i]);
}

/* Adds a stop word to the list of removed common words
*/
void addStopList(IndexFILE *indexf, char *word)
{
char* arrayWord;
	if (isstopword(indexf, word))
		return;

		/* Another BUG!!  Jose Ruiz 04/00
		The dimension of the array was not checked 
		Fixed */
	if (indexf->stopPos == indexf->stopMaxSize) {
		indexf->stopMaxSize += 100;
		if(!indexf->stopList)
			indexf->stopList = (char **)emalloc(indexf->stopMaxSize * sizeof(char *));
		else
			indexf->stopList = (char **)erealloc(indexf->stopList,indexf->stopMaxSize * sizeof(char *));
	}
	arrayWord = (char *) estrdup (word);
	indexf->stopList[indexf->stopPos++] = arrayWord;
}


/* Adds a stop word to a hash table.
*/

void addstophash(IndexFILE *indexf, char *word)
{
unsigned hashval;
struct swline *sp;
	
	if (isstopword(indexf, word))
		return;
	
	sp = (struct swline *) emalloc(sizeof(struct swline));
	sp->line = (char *) estrdup(word);
	
	hashval = hash(word);
	sp->next = indexf->hashstoplist[hashval];
	indexf->hashstoplist[hashval] = sp;
}

/* Sees if a word is a stop word by looking it up in the hash table.
*/

int isstopword(IndexFILE *indexf, char *word)
{
unsigned hashval;
struct swline *sp;
	
	hashval = hash(word);
	sp = indexf->hashstoplist[hashval];
	
	while (sp != NULL) {
		if (!strcmp(sp->line, word))
			return 1;
		sp = sp->next;
	}
	return 0;
}

/* Adds a file number to a hash table of results.
** If the entry's alrady there, add the ranks,
** else make a new entry.
*/
/* Jose Ruiz 04/00
** For better performance in large "or"
** keep the lists sorted by filename
*/
void mergeresulthashlist(sw,r)
SWISH *sw;
RESULT *r;
{
unsigned hashval;
RESULT *rp, *tmp;
int *newposition;
	
	tmp = NULL;
	hashval = bignumhash(r->filenum);
	
	rp = sw->resulthashlist[hashval];
	while (rp != NULL) {
		if (rp->filenum == r->filenum) {
			rp->rank += r->rank;
			rp->structure |= r->structure;
			if(r->frequency) {
				if (rp->frequency) {
					newposition = (int *) emalloc((rp->frequency + r->frequency) * sizeof(int));
					CopyPositions(newposition,0,r->position,0,r->frequency);
					CopyPositions(newposition,r->frequency,rp->position,0,rp->frequency);
				} else {
					newposition = (int *) emalloc(r->frequency * sizeof(int));
					CopyPositions(newposition,0,r->position,0,r->frequency);
				}
				rp->frequency += r->frequency;
				efree(rp->position);
				rp->position = newposition;
			}
			freeresult(sw,r);
			return;
		}
		else if (r->filenum < rp->filenum) break;
		tmp =rp;
		rp = rp->next;
	}
	if(!rp) {
		if(tmp) {
			tmp->next = r;
			r->next = NULL;
		} else {
			sw->resulthashlist[hashval] = r;
			r->next = NULL;
		}
	} else {
		if(tmp) {
			tmp->next = r;
			r->next = rp;
		} else {
			sw->resulthashlist[hashval] = r;
			r->next = rp;
		}
	}
}

/* Initializes the result hash list.
*/

void initresulthashlist(SWISH *sw)
{
	int i;
	
	for (i = 0; i < BIGHASHSIZE; i++)
		sw->resulthashlist[i] = NULL;
}


void freestophash(IndexFILE *indexf)
{
int i;
struct swline *sp, *tmp;
 
        for(i=0;i<HASHSIZE;i++)
                if(indexf->hashstoplist[i]) {
			sp = (struct swline *)indexf->hashstoplist[i];
			while (sp) {
				tmp = sp->next;
				efree(sp->line);
				efree(sp);
				sp = tmp;
			}
			indexf->hashstoplist[i]=NULL;
		}
}

void freeStopList(IndexFILE *indexf)
{
int i;
        for(i=0;i<indexf->stopPos;i++)
		efree(indexf->stopList[i]);
	if (indexf->stopList) efree(indexf->stopList);indexf->stopList=NULL;
	indexf->stopPos=indexf->stopMaxSize=0;
}

/* Adds a "use" word to a hash table.
*/

void addusehash(IndexFILE *indexf, char *word)
{
unsigned hashval;
struct swline *sp;
	
	if (isuseword(indexf, word))
		return;
	
	sp = (struct swline *) emalloc(sizeof(struct swline));
	sp->line = (char *) estrdup(word);
	
	hashval = hash(word);
	sp->next = indexf->hashuselist[hashval];
	indexf->hashuselist[hashval] = sp;
}

/* Sees if a word is a "use" word by looking it up in the hash table.
*/

int isuseword(IndexFILE *indexf, char *word)
{
unsigned hashval;
struct swline *sp;
	
	hashval = hash(word);
	sp = indexf->hashuselist[hashval];
	
	while (sp != NULL) {
		if (!strcmp(sp->line, word))
			return 1;
		sp = sp->next;
	}
	return 0;
}

