/*
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
*/

#define GLOBAL_VARS

#include "swish.h"

#include "string.h"
#include "error.h"
#include "list.h"
#include "search.h"
#include "index.h"
#include "file.h"
#include "http.h"
#include "merge.h"
#include "docprop.h"
#include "mem.h"
#include "hash.h"
#include "entities.h"
#include "filter.h"
#include "result_output.h"
#include "search_alt.h"
#include "result_output.h"
#include "result_sort.h"
#include "db.h"

/* 
  -- init swish structure 
*/

SWISH *SwishNew()
{
SWISH *sw;
int i;
    sw=emalloc(sizeof(SWISH));

    initModule_Filter (sw);
    initModule_ResultOutput (sw);
    initModule_SearchAlt (sw);
    initModule_ResultSort (sw);
    initModule_Entities (sw);
    initModule_DB (sw);
    initModule_Search (sw);
    initModule_Index (sw);


    sw->followsymlinks = 0;
    sw->TotalWords = 0;
    sw->TotalFiles = 0;
    sw->dirlist = NULL;
    sw->indexlist = NULL;
    sw->replacelist = NULL;
    sw->lasterror = RC_OK;
    sw->verbose = VERBOSE;
    sw->indexComments = 1;
    sw->nocontentslist = NULL;
    sw->DefaultDocType=NODOCTYPE;
    sw->indexcontents = NULL;
    sw->storedescription = NULL;
    sw->suffixlist = NULL;
    sw->ignoremetalist = NULL;
    sw->dontbumptagslist = NULL;
    sw->mtime_limit = 0;

    sw->applyautomaticmetanames = 0;


    sw->truncateDocSize = 0;      /* default: no truncation of docs    */
    
        /* File system parameters */
    sw->pathconlist=sw->dirconlist=sw->fileconlist=sw->titconlist=sw->fileislist=NULL;
    for(i=0;i<BIGHASHSIZE;i++) sw->inode_hash[i]=NULL;

    /* prog system parameters */
    sw->progparameterslist =  NULL;

    sw->equivalentservers=NULL;
    for(i=0;i<BIGHASHSIZE;i++) sw->url_hash[i]=NULL;

        /* Swap flag and temp files*/
    sw->swap_flag=SWAP_DEFAULT;

        /* Load Default Values */
    SwishDefaults(sw);
        /* Make rest of lookup tables */
    makeallstringlookuptables(sw);
    return(sw);
}



void SwishDefaults(SWISH *sw)
{

    sw->plimit=PLIMIT;
    sw->flimit=FLIMIT;
    sw->PhraseDelimiter=PHRASE_DELIMITER_CHAR;
        /* MetaNames indexing options (default values from config.h)*/
    sw->ReqMetaName=REQMETANAME;
    sw->OkNoMeta=OKNOMETA;
        /* http system parameters */
    sw->maxdepth=5;
    sw->delay=60;

}

/* Free memory for search results and parameters (properties ...) */
void SwishResetSearch(SWISH *sw)
{

                /* Free sort stuff */
    freeModule_ResultSort (sw);;
}

void SwishClose(SWISH *sw)
{
    IndexFILE *tmpindexlist;
    int i;

    if(sw) {
        /* Free search results and imput parameters */
        SwishResetSearch(sw);
          
        /* Close any pending DB */
        tmpindexlist = sw->indexlist;
        while(tmpindexlist)
        {
            if(tmpindexlist->DB)
                DB_Close(sw, tmpindexlist->DB);
            tmpindexlist=tmpindexlist->next;
        }

        freeModule_Filter (sw);
        freeModule_ResultOutput (sw);
        freeModule_SearchAlt (sw);
        freeModule_Entities (sw);
        freeModule_DB (sw);
        freeModule_Index (sw);

        /* Since it is possible to invoke SwishSearch several times
                ** with the same SWISH handle, the freeModule_ResultSort stuff
                ** must be in SwishResetSearch */
        /* freeModule_ResultSort (sw); */

                        /* Free file structures */
        freefileoffsets(sw);
            /* Free MetaNames and close files */
        tmpindexlist=sw->indexlist;
        while(tmpindexlist)
        {
            if(tmpindexlist->header.metaCounter)
            {
                for(i=0;i<tmpindexlist->header.metaCounter;i++)
                {
                            efree(tmpindexlist->header.metaEntryArray[i]->metaName);
                            if(tmpindexlist->header.metaEntryArray[i]->sorted_data)
                                    efree(tmpindexlist->header.metaEntryArray[i]->sorted_data);
                            efree(tmpindexlist->header.metaEntryArray[i]);
                }
                efree(tmpindexlist->header.metaEntryArray);
            }
                        /* Free stopwords structures */
            freestophash(&tmpindexlist->header);
            freeStopList(&tmpindexlist->header);

             freebuzzwordhash(&tmpindexlist->header);

            free_header(&tmpindexlist->header);
            /* Free compression lookup tables */
            if(tmpindexlist->header.locationlookup)
            {
                for(i=0;i<tmpindexlist->header.locationlookup->n_entries;i++)
                    efree(tmpindexlist->header.locationlookup->all_entries[i]);
                efree(tmpindexlist->header.locationlookup);
            }    
            if(tmpindexlist->header.structurelookup)
            {
                for(i=0;i<tmpindexlist->header.structurelookup->n_entries;i++)
                    efree(tmpindexlist->header.structurelookup->all_entries[i]);
                efree(tmpindexlist->header.structurelookup);
            }    
            if(tmpindexlist->header.structfreqlookup)
            {
                for(i=0;i<tmpindexlist->header.structfreqlookup->n_entries;i++)
                    efree(tmpindexlist->header.structfreqlookup->all_entries[i]);
                efree(tmpindexlist->header.structfreqlookup);
            }
            if(tmpindexlist->header.pathlookup)
            {
                for(i=0;i<tmpindexlist->header.pathlookup->n_entries;i++)
                {
                    efree(tmpindexlist->header.pathlookup->all_entries[i]->val);
                    efree(tmpindexlist->header.pathlookup->all_entries[i]);
                }
                efree(tmpindexlist->header.pathlookup);
            }    
/* Removed due to patents 
            if(tmpindexlist->header.applyFileInfoCompression && tmpindexlist->n_dict_entries)
            {
                for(i=0;i<tmpindexlist->n_dict_entries;i++)
                    efree(tmpindexlist->dict[i]);
            }
*/
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
    if (sl)    freeStringList(sl);
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
    if(props && props[0]) 
    {
        slprops=parse_line(props);
        for(i=0;i<slprops->n;i++)
            addSearchResultDisplayProperty(sw,slprops->word[i]);
    }

    if(sort && sort[0]) 
    {
        slsort=parse_line(sort);
        for(i=0;i<slsort->n;) 
        {
            sortmode=1;  /* Default mode is ascending */
            field=slsort->word[i++];
            if(i<slsort->n) 
            {
                if(!strcasecmp(slsort->word[i],"asc")) 
                {
                    sortmode=-1;  /* Ascending */
                    i++;
                } 
                else
                { 
                    if(!strcasecmp(slsort->word[i],"desc")) 
                    {
                        sortmode=1;  /* Ascending */
                        i++;
                    }
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


int SwishSeek(SWISH *sw,int pos)
{
int i;
RESULT *sp=NULL;
    if(!sw) return INVALID_SWISH_HANDLE;
    if(!sw->Search->db_results) return((sw->lasterror=SWISH_LISTRESULTS_EOF));
        /* Check if only one index file -> Faster SwishSeek */
    if(!sw->Search->db_results->next)
    {      
        for (i=0,sp=sw->Search->db_results->sortresultlist;sp && i<pos;i++)
        {
            sp = sp->nextsort;
        }
        sw->Search->db_results->currentresult=sp;
    }
    else
    {
        /* Well, we finally have more than one file */
        /* In this case we have no choice - We need to read the data from disk */
        /* The easy way: Let SwishNext do the job */
        for (i=0;sp && i<pos;i++)
        {
            sp=SwishNext(sw);
        }
    }
    if(!sp) return((sw->lasterror=SWISH_LISTRESULTS_EOF));
    return pos;
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
            *numstops=indexf->header.stopPos;
            return indexf->header.stopList;
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


