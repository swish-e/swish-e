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
**
**1998-07-04  addfilter   ( R. Scherg)
**
*/

#include "swish.h"
#include "list.h"
#include "mem.h"
#include "string.h"

struct swline *addswline(rp, line)
struct swline *rp;
char *line;
{
struct swline *newnode;
	newnode = (struct swline *) emalloc(sizeof(struct swline));
	newnode->line = (char *) estrdup(line);
	newnode->next = NULL;

	if (rp == NULL)
		rp = newnode;
	else
		rp->nodep->next = newnode;
	
	rp->nodep = newnode;
	
	return rp;
}

struct swline *dupswline(rp)
struct swline *rp;
{
struct swline *tmp=NULL, *tmp2=NULL;
struct swline *newnode;
	while(rp)
	{
		newnode = (struct swline *) emalloc(sizeof(struct swline));
		newnode->line = (char *) estrdup(rp->line);
		newnode->next = NULL;
		
		if(!tmp)
			tmp = newnode;
		else
			tmp2->next=newnode;
		tmp2 = newnode;
		rp=rp->next;
	}
	return tmp;
}

IndexFILE *addindexfile(rp, line)
IndexFILE *rp;
char *line;
{
IndexFILE *newnode;
int i;
	newnode = (IndexFILE *) emalloc(sizeof(IndexFILE));
	newnode->line = (char *) estrdup(line);
	for (i=0; i<MAXCHARS; i++) newnode->offsets[i] = 0L;
	for (i=0; i<SEARCHHASHSIZE; i++) newnode->hashoffsets[i] = 0L;
	newnode->filearray = NULL;
	newnode->filearray_cursize=newnode->filearray_maxsize=0;
	newnode->fileoffsetarray = NULL;
	newnode->fileoffsetarray_cursize=newnode->fileoffsetarray_maxsize=0;
	newnode->fp=NULL;
	newnode->metaEntryList=NULL;
	newnode->Metacounter=0;
	newnode->wordpos=0;
	newnode->stopPos = 0;
	newnode->stopMaxSize = 0;
	newnode->stopList = NULL;
	for (i=0; i<HASHSIZE; i++) newnode->hashstoplist[i] = NULL;
	newnode->is_use_words_flag=0;
	for (i=0; i<HASHSIZE; i++) newnode->hashuselist[i] = NULL;
	for (i=0; i<256; i++) newnode->keywords[i] = NULL;

        newnode->locationlookup=NULL;
        newnode->structurelookup=NULL;
        newnode->structfreqlookup=NULL;
        newnode->pathlookup=NULL;

	init_header(&newnode->header);

	newnode->next = NULL;

	if (rp == NULL)
		rp = newnode;
	else
		rp->nodep->next = newnode;
	
	rp->nodep = newnode;
	
	return rp;
}

struct filter *addfilter(rp, suffix,prog,filterdir)
struct filter *rp;
char *suffix, *prog;
char *filterdir;
{
struct filter *newnode;
char *buf;
int ilen1,ilen2;

        newnode = (struct filter *) emalloc(sizeof(struct filter));
        newnode->suffix= (char *) estrdup(suffix);
	ilen1=strlen(filterdir);
	ilen2=strlen(prog);
	buf = (char *)emalloc(ilen1+ilen2+2);
	memcpy(buf,filterdir,ilen1);
		/* If filterdir exists and not ends in DIRDELIMITER 
		** (/ in Unix or \\ in WIN32) add it
		*/
	if(ilen1 && buf[ilen1-1]!=DIRDELIMITER) buf[ilen1++]=DIRDELIMITER;
	memcpy(buf+ilen1,prog,ilen2);
	buf[ilen1+ilen2]='\0';
        newnode->prog= buf;
        newnode->next = NULL;

        if (rp == NULL)
                rp = newnode;
        else
                rp->nodep->next = newnode;

        rp->nodep = newnode;

        return rp;
}

void init_header(INDEXDATAHEADER *header)
{
	header->lenwordchars=header->lenbeginchars=header->lenendchars=header->lenignorelastchar=header->lenignorefirstchar=header->lentranslatechars1=header->lentranslatechars2=header->lenbumpposchars=MAXCHARDEFINED;

	header->wordchars = (char *)emalloc(header->lenwordchars + 1);
        header->wordchars = SafeStrCopy(header->wordchars,WORDCHARS,&header->lenwordchars);
        sortstring(header->wordchars);  /* Sort chars and remove dups */
        makelookuptable(header->wordchars,header->wordcharslookuptable);

	header->beginchars = (char *)emalloc(header->lenbeginchars + 1);
        header->beginchars = SafeStrCopy(header->beginchars,BEGINCHARS,&header->lenbeginchars);
        sortstring(header->beginchars);  /* Sort chars and remove dups */
        makelookuptable(header->beginchars,header->begincharslookuptable);

	header->endchars = (char *)emalloc(header->lenendchars + 1);
        header->endchars = SafeStrCopy(header->endchars,ENDCHARS,&header->lenendchars);
        sortstring(header->endchars);  /* Sort chars and remove dups */
        makelookuptable(header->endchars,header->endcharslookuptable);

	header->ignorelastchar = (char *)emalloc(header->lenignorelastchar + 1);
        header->ignorelastchar = SafeStrCopy(header->ignorelastchar,IGNORELASTCHAR,&header->lenignorelastchar);
        sortstring(header->ignorelastchar);  /* Sort chars and remove dups */
        makelookuptable(header->ignorelastchar,header->ignorelastcharlookuptable);

	header->ignorefirstchar = (char *)emalloc(header->lenignorefirstchar + 1);
        header->ignorefirstchar = SafeStrCopy(header->ignorefirstchar,IGNOREFIRSTCHAR,&header->lenignorefirstchar);
        sortstring(header->ignorefirstchar);  /* Sort chars and remove dups */
        makelookuptable(header->ignorefirstchar,header->ignorefirstcharlookuptable);

	header->translatechars1 = (char *)emalloc(header->lentranslatechars1 + 1);
	header->translatechars1[0]='\0';

	header->translatechars2 = (char *)emalloc(header->lentranslatechars2 + 1);
	header->translatechars2[0]='\0';
	header->bumpposchars = (char *)emalloc(header->lenbumpposchars + 1);
	header->bumpposchars[0]='\0';

	header->lenindexedon=header->lensavedasheader=header->lenindexn=header->lenindexd=header->lenindexp=header->lenindexa=MAXSTRLEN;
	header->indexn = (char *)emalloc(header->lenindexn + 1);header->indexn[0]='\0';
	header->indexd = (char *)emalloc(header->lenindexd + 1);header->indexd[0]='\0';
	header->indexp = (char *)emalloc(header->lenindexp + 1);header->indexp[0]='\0';
	header->indexa = (char *)emalloc(header->lenindexa + 1);header->indexa[0]='\0';
	header->savedasheader = (char *)emalloc(header->lensavedasheader + 1);header->savedasheader[0]='\0';
	header->indexedon = (char *)emalloc(header->lenindexedon + 1);header->indexedon[0]='\0';
	header->totalfiles=header->totalwords=0;
	header->applyStemmingRules = 0;                 /* added 11/24/98 */
	header->applySoundexRules = 0;                  /* added DN 09/01/99 */
	header->applyFileInfoCompression = 0;			/* added 2000/01/01 jmruiz */
	header->ignoreTotalWordCountWhenRanking = 0;    /* added 11/24/98 */
	header->minwordlimit = MINWORDLIMIT;
        header->maxwordlimit = MAXWORDLIMIT;
}


void free_header(INDEXDATAHEADER *header)
{
	if(header->lenwordchars) efree(header->wordchars);
	if(header->lenbeginchars) efree(header->beginchars);
	if(header->lenendchars) efree(header->endchars);
	if(header->lenignorefirstchar) efree(header->ignorefirstchar);
	if(header->lenignorelastchar) efree(header->ignorelastchar);
	if(header->lenindexn) efree(header->indexn);
        if(header->lenindexa) efree(header->indexa);
        if(header->lenindexp) efree(header->indexp);
        if(header->lenindexd) efree(header->indexd);
	if(header->lenindexedon) efree(header->indexedon);		
	if(header->lensavedasheader) efree(header->savedasheader);	
	if(header->lentranslatechars1) efree(header->translatechars1);	
	if(header->lentranslatechars2) efree(header->translatechars2);
	if(header->lenbumpposchars) efree(header->bumpposchars);
}
