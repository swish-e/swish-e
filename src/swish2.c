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
*/

#define GLOBAL_VARS

#include "swish.h"

#include "error.h"
#include "list.h"
#include "search.h"
#include "index.h"
#include "string.h"
#include "file.h"
#include "http.h"
#include "merge.h"
#include "docprop.h"
#include "mem.h"
#include "hash.h"

SWISH *SwishNew()
{
SWISH *sw;
int i;
	sw=emalloc(sizeof(SWISH));
	sw->followsymlinks = 0;
	sw->TotalWords = 0;
	sw->TotalFiles = 0;
	sw->filenum = 0;
	sw->useCustomOutputDelimiter = 0;           /* added 11/24/98 */
	sw->entryArray = NULL;
        sw->dirlist = NULL;
        sw->indexlist = NULL;
        sw->replacelist = NULL;
        sw->maxhits = -1;
        sw->beginhits = 0;
        sw->lasterror = RC_OK;
        sw->verbose = VERBOSE;
        sw->indexComments = 1;
        sw->nocontentslist = NULL;
	sw->DefaultDocType=NODOCTYPE;
        sw->indexcontents = NULL;
        sw->storedescription = NULL;
        sw->suffixlist = NULL;
	sw->resultlist=NULL;
	sw->searchwordlist=NULL;
	sw->sortresultlist=NULL;
	sw->currentresult=NULL;
        sw->applyautomaticmetanames = 0;
	sw->len_compression_buffer = MAXSTRLEN;  /* For example */
	sw->compression_buffer=(unsigned char *)emalloc(sw->len_compression_buffer);
	init_header(&sw->mergedheader);

	sw->lenfilterdir=sw->lentmpdir=sw->lenspiderdirectory=MAXSTRLEN;
	sw->filterdir = (char *)emalloc(sw->lenfilterdir + 1);sw->filterdir[0]='\0';
	sw->filterlist=NULL;
	sw->tmpdir = (char *)emalloc(sw->lentmpdir + 1);sw->tmpdir[0]='\0';
	sw->spiderdirectory = (char *)emalloc(sw->lenspiderdirectory + 1);sw->spiderdirectory[0]='\0';
	
	sw->lencustomOutputDelimiter=5;
	sw->customOutputDelimiter=(char *)emalloc(sw->lencustomOutputDelimiter+1); sw->customOutputDelimiter[0]='\0';
		/* Properties */
	sw->numPropertiesToDisplay=sw->currentMaxPropertiesToDisplay=sw->numPropertiesToSort=sw->currentMaxPropertiesToSort=0;
	sw->propNameToDisplay=NULL;
	sw->propNameToSort=NULL;
	sw->propModeToSort=NULL;


	sw->opt.extendedformat = NULL;
	sw->opt.extendedheader = 0;


		/* File system parameters */
	sw->pathconlist=sw->dirconlist=sw->fileconlist=sw->titconlist=sw->fileislist=NULL;
	for(i=0;i<BIGHASHSIZE;i++) sw->inode_hash[i]=NULL;

	sw->equivalentservers=NULL;
	for(i=0;i<BIGHASHSIZE;i++) sw->url_hash[i]=NULL;

		/* Init  hash tables */
	for (i=0; i<SEARCHHASHSIZE; i++) sw->hashentries[i] = NULL;
		/* Swap flag and temp files*/
	sw->swap_flag=SWAP_DEFAULT;
		/* By default do not translate to ascii7 */
	sw->applyAscii7=0;
	sw->fp_loc_write=sw->fp_loc_read=sw->fp_file_write=sw->fp_file_read=NULL;
	if(sw->tmpdir && sw->tmpdir[0] && isdirectory(sw->tmpdir))
	{
		sw->swap_file_name=tempnam(sw->tmpdir,"swfi");
		sw->swap_location_name=tempnam(sw->tmpdir,"swlo");
	} else {
		sw->swap_file_name=tempnam(NULL,"swfi");
		sw->swap_location_name=tempnam(NULL,"swlo");
	}

		/* Load Default Values */
	SwishDefaults(sw);
		/* Make rest of lookup tables */
	makeallstringlookuptables(sw);
	return(sw);
}

void SwishDefaults(SWISH *sw)
{
	init_header(&sw->mergedheader);

        /* Initialize tmpdir */
        sw->tmpdir = SafeStrCopy(sw->tmpdir,TMPDIR,&sw->lentmpdir);

        /* Initialize spider directory */
        sw->spiderdirectory = SafeStrCopy(sw->spiderdirectory,SPIDERDIRECTORY,&sw->lenspiderdirectory);
	sw->plimit=PLIMIT;
	sw->flimit=FLIMIT;
	sw->PhraseDelimiter=PHRASE_DELIMITER_CHAR;
		/* MetaNames indexing options (default values from config.h)*/
	sw->ReqMetaName=REQMETANAME;
	sw->OkNoMeta=OKNOMETA;
		/* CONVERTHTMLENTITIES indexing default option */
	sw->ConvertHTMLEntities=CONVERTHTMLENTITIES;
		/* Init Entities hash table to NULL */
	sw->EntitiesHashTable=NULL;
		/* http system parameters */
	sw->maxdepth=5;
	sw->delay=60;
}

/* Free memory for search results and parameters (properties ...) */
void SwishResetSearch(SWISH *sw)
{
		/* Default variables for search */
        sw->maxhits = -1;
        sw->beginhits = 0;
		/* Free results from previous search if they exists */
	if(sw->resultlist) freeresultlist(sw);sw->resultlist=NULL;
	if(sw->searchwordlist) freeswline(sw->searchwordlist); sw->searchwordlist=NULL;
		/* Free props arrays */
	FreeOutputPropertiesVars(sw);
}

void SwishClose(SWISH *sw)
{
IndexFILE *tmpindexlist;
int i;

if(sw) {
		/* Free search results and imput parameters */
		SwishResetSearch(sw);

		free_header(&sw->mergedheader);
                if(sw->lencustomOutputDelimiter)efree(sw->customOutputDelimiter);
		if(sw->lenspiderdirectory) efree(sw->spiderdirectory);		
		if(sw->lentmpdir) efree(sw->tmpdir);		
		if(sw->lenfilterdir) efree(sw->filterdir);		

                        /* Free file structures */
		freefileoffsets(sw);
			/* Free MetaNames and close files */
		tmpindexlist=sw->indexlist;
		while(tmpindexlist)
		{
			if(tmpindexlist->metaCounter)
			{
				for(i=0;i<tmpindexlist->metaCounter;i++)
				{
                			efree(tmpindexlist->metaEntryArray[i]->metaName);
                			efree(tmpindexlist->metaEntryArray[i]);
				}
                		efree(tmpindexlist->metaEntryArray);
			}
			if (tmpindexlist->fp) 
				fclose(tmpindexlist->fp);
                        /* Free stopwords structures */
			freestophash(tmpindexlist);
			freeStopList(tmpindexlist);
			free_header(&tmpindexlist->header);
			/* Free compression lookup tables */
			if(tmpindexlist->locationlookup)
			{
				for(i=0;i<tmpindexlist->locationlookup->n_entries;i++)
					efree(tmpindexlist->locationlookup->all_entries[i]);
				efree(tmpindexlist->locationlookup);
			}	
			if(tmpindexlist->structurelookup)
			{
				for(i=0;i<tmpindexlist->structurelookup->n_entries;i++)
					efree(tmpindexlist->structurelookup->all_entries[i]);
				efree(tmpindexlist->structurelookup);
			}	
			if(tmpindexlist->structfreqlookup)
			{
				for(i=0;i<tmpindexlist->structfreqlookup->n_entries;i++)
					efree(tmpindexlist->structfreqlookup->all_entries[i]);
				efree(tmpindexlist->structfreqlookup);
			}
			if(tmpindexlist->pathlookup)
			{
				for(i=0;i<tmpindexlist->pathlookup->n_entries;i++)
				{
					efree(tmpindexlist->pathlookup->all_entries[i]->val);
					efree(tmpindexlist->pathlookup->all_entries[i]);
				}
				efree(tmpindexlist->pathlookup);
			}	
			if(tmpindexlist->header.applyFileInfoCompression && tmpindexlist->n_dict_entries)
			{
				for(i=0;i<tmpindexlist->n_dict_entries;i++)
					efree(tmpindexlist->dict[i]);
			}
			for(i=0;i<256;i++)
				if(tmpindexlist->keywords[i])
					efree(tmpindexlist->keywords[i]);
			tmpindexlist=tmpindexlist->next;
                }
		freeindexfile(sw->indexlist);
		
		/* Free fs parameters */
		if (sw->pathconlist) freeswline(sw->pathconlist);
		if (sw->dirconlist) freeswline(sw->dirconlist);
		if (sw->fileconlist) freeswline(sw->fileconlist);
		if (sw->titconlist) freeswline(sw->titconlist);
		if (sw->fileislist) freeswline(sw->fileislist);

		/* Free compression info */	
		/* Free compression buffer */	
		efree(sw->compression_buffer);
		

		/* Free SWISH struct */
		efree(sw);
	}
}


SWISH *SwishOpen(char *indexfiles)
{
StringList *sl=NULL;
SWISH *sw;
int i;
	if(indexfiles && indexfiles[0]) {
		sw = SwishNew();
		sl=parse_line(indexfiles);
		for(i=0;i<sl->n;i++)
			sw->indexlist = (IndexFILE *) 
				addindexfile(sw->indexlist, sl->word[i]);
		if((i=SwishAttach(sw,0))<0) {
			SwishClose(sw);
			sw=NULL;
		}
	} else sw=NULL;
	if (sl)	freeStringList(sl);
	return sw;
}

int SwishSearch(SWISH *sw, char *words, int structure, char *props, char *sort)
{
StringList *slprops=NULL;
StringList *slsort=NULL;
int i,sortmode;
char *field;
	if(!sw) return INVALID_SWISH_HANDLE;
		/* If previous search - reset its values (results, props ) */
	SwishResetSearch(sw);
	if(props && props[0]) {
                        slprops=parse_line(props);
                        for(i=0;i<slprops->n;i++)
                                addSearchResultDisplayProperty(sw,slprops->word[i]);
        }

	if(sort && sort[0]) {
                slsort=parse_line(sort);
                for(i=0;i<slsort->n;) {
			sortmode=1;  /* Default mode is ascending */
			field=slsort->word[i++];
			if(i<slsort->n) {
				if(!strcasecmp(slsort->word[i],"asc")) {
					sortmode=-1;  /* Ascending */
					i++;
				} else if(!strcasecmp(slsort->word[i],"desc")) {
					sortmode=1;  /* Ascending */
					i++;
				}
			}
                        addSearchResultSortProperty(sw,field,sortmode);
		}
        }
	i=0;
	i=search(sw,words,structure);   /* search with no eco */
        if(slsort) freeStringList(slsort);
        if(slprops) freeStringList(slprops);
	return i;
}

/* Gets the current time in seconds since the epoch.
*/

long getthetime()
{
long thetime;
time_t tp;
        thetime = (long) time(&tp);
        return thetime;
}

int SwishSeek(SWISH *sw,int pos)
{
int i;
RESULT *sp;
	if(!sw) return INVALID_SWISH_HANDLE;
	sw->currentresult=0;
        for (i=0,sp=SwishNext(sw);sp && i<pos;i++)
        {
		sp = sp->nextsort;
	}
	if(!sp) return(sw->lasterror);
	else return pos;
}

int SwishError(SWISH *sw)
{
	if(!sw) return INVALID_SWISH_HANDLE;
	return(sw->lasterror);
}
	
char *SwishErrorString(int errornumber)
{
	return(getErrorString(errornumber));
}

char *SwishHeaderParameter(IndexFILE *indexf,char *parameter_name)
{
	if(!strcasecmp(parameter_name,WORDCHARSPARAMNAME)) 
		return indexf->header.wordchars;
	else if(!strcasecmp(parameter_name,BEGINCHARSPARAMNAME)) 
		return indexf->header.beginchars;
	else if(!strcasecmp(parameter_name,BEGINCHARSPARAMNAME)) 
		return indexf->header.beginchars;
	else if(!strcasecmp(parameter_name,ENDCHARSPARAMNAME)) 
		return indexf->header.endchars;
	else if(!strcasecmp(parameter_name,IGNOREFIRSTCHARPARAMNAME)) 
		return indexf->header.ignorefirstchar;
	else if(!strcasecmp(parameter_name,IGNORELASTCHARPARAMNAME)) 
		return indexf->header.ignorelastchar;
	else if(!strcasecmp(parameter_name,INDEXEDONPARAMNAME)) 
		return indexf->header.indexedon;
	else if(!strcasecmp(parameter_name,DESCRIPTIONPARAMNAME)) 
		return indexf->header.indexd;
	else if(!strcasecmp(parameter_name,POINTERPARAMNAME)) 
		return indexf->header.indexp;
	else if(!strcasecmp(parameter_name,MAINTAINEDBYPARAMNAME)) 
		return indexf->header.indexa;
	else if(!strcasecmp(parameter_name,STEMMINGPARAMNAME)) {
		if(indexf->header.applyStemmingRules)return "1";
		else return "0";
	} else if(!strcasecmp(parameter_name,SOUNDEXPARAMNAME)) {
		if(indexf->header.applySoundexRules)return "1";
		else return "0";
	} else
		return "";
}

char **SwishStopWords(SWISH *sw, char * filename, int *numstops)
{
IndexFILE *indexf;
	indexf=sw->indexlist;
	while (indexf)
	{
		if (!strcasecmp(indexf->line,filename))
		{
			*numstops=indexf->stopPos;
			return indexf->stopList;
		}
	}
	*numstops=0;
	return NULL;
}

char *SwishWords(SWISH *sw, char * filename, char c)
{
IndexFILE *indexf;
        indexf=sw->indexlist;
        while (indexf)
        {
                if (!strcasecmp(indexf->line,filename))
                {
                        return getfilewords(sw,c,indexf);
                }
        }
        return "";
}


