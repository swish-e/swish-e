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
** 1998-07-04  addfilter   ( R. Scherg)
** 2001-02-28  rasc  -- addfilter removed here
**
*/

#include "swish.h"
#include "list.h"
#include "mem.h"
#include "metanames.h"
#include "string.h"


struct swline *addswline(struct swline *rp, char *line)
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

struct swline *dupswline(struct swline *rp)
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

void addindexfile(SWISH *sw, char *line)
{
    IndexFILE *head = sw->indexlist;
    IndexFILE *indexf = (IndexFILE *) emalloc(sizeof(IndexFILE));

	memset( indexf, 0, sizeof(IndexFILE) );

	indexf->sw = sw;  /* save parent object */
	indexf->line = estrdup(line);
	init_header(&indexf->header);
	indexf->next = NULL;


    /* Add default meta names -- these will be replaced if reading from an index file */
	add_default_metanames(indexf);

	/* Add index to end of list */

	if ( head == NULL )  /* first entry? */
		sw->indexlist = head = indexf;
	else
		head->nodep->next = indexf;  /* point the previous last one to the new last one */
	
	head->nodep = indexf;  /* set the last pointer */
}


void freeswline(struct swline *tmplist)
{
struct swline *tmplist2;

	while (tmplist) {
		tmplist2 = tmplist->next;
		efree(tmplist->line);
		efree(tmplist);
		tmplist = tmplist2;
	}
}


void freeindexfile(IndexFILE *tmplist)
{
IndexFILE *tmplist2;

	while (tmplist) {
		tmplist2 = tmplist->next;
		efree(tmplist->line);
		efree(tmplist);
		tmplist = tmplist2;
	}
}


void init_header(INDEXDATAHEADER *header)
{

	header->lenwordchars=header->lenbeginchars=header->lenendchars=header->lenignorelastchar=header->lenignorefirstchar=header->lenbumpposchars=MAXCHARDEFINED;

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


	header->bumpposchars = (char *)emalloc(header->lenbumpposchars + 1);
	header->bumpposchars[0]='\0';

	header->lenindexedon=header->lensavedasheader=header->lenindexn=header->lenindexd=header->lenindexp=header->lenindexa=MAXSTRLEN;
	header->indexn = (char *)emalloc(header->lenindexn + 1);header->indexn[0]='\0';
	header->indexd = (char *)emalloc(header->lenindexd + 1);header->indexd[0]='\0';
	header->indexp = (char *)emalloc(header->lenindexp + 1);header->indexp[0]='\0';
	header->indexa = (char *)emalloc(header->lenindexa + 1);header->indexa[0]='\0';
	header->savedasheader = (char *)emalloc(header->lensavedasheader + 1);header->savedasheader[0]='\0';
	header->indexedon = (char *)emalloc(header->lenindexedon + 1);header->indexedon[0]='\0';

	header->ignoreTotalWordCountWhenRanking = 1;
	header->minwordlimit = MINWORDLIMIT;
	header->maxwordlimit = MAXWORDLIMIT;

	makelookuptable("",header->bumpposcharslookuptable); 

    BuildTranslateChars(header->translatecharslookuptable,(unsigned char *)"",(unsigned char *)"");


    /* this is to ignore numbers */
    header->numberchars_used_flag = 0;  /* not used by default*/
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
	if(header->lenbumpposchars) efree(header->bumpposchars);


    /* ??? temporary until metas and props are seperated */
    if ( header->propIDX_to_metaID )
        efree( header->propIDX_to_metaID );

    if ( header->metaID_to_PropIDX )
        efree( header->metaID_to_PropIDX );

#ifndef USE_BTREE
    if ( header->TotalWordsPerFile )
        efree( header->TotalWordsPerFile );
#endif

}


/* 2001/04Jose Ruiz
Splits a swline struct by char c */
void splitswline(struct swline *rp, int c)
{
struct swline *temp;
char *p;
	for(p=rp->line;(p=strrchr(rp->line,c));)
	{
		*p='\0';
		p++;
		if(*p)
		{
			temp=(struct swline *)emalloc(sizeof(struct swline));
			temp->next=rp->next;
			temp->line=(char *)estrdup(p);
			rp->next=temp;
		}
	}
}

