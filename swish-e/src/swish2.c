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
#include "mem.h"
#include "error.h"
#include "list.h"
#include "search.h"
#include "index.h"
#include "file.h"
#include "http.h"
#include "merge.h"
#include "docprop.h"
#include "hash.h"
#include "entities.h"
#include "filter.h"
#include "search_alt.h"
#include "result_output.h"
#include "result_sort.h"
#include "db.h"
#include "fs.h"
#include "swish_words.h"
#include "extprog.h"
#include "metanames.h"
#include "proplimit.h"
#include "parse_conffile.h"
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif


/* Moved here so it's in the library */
unsigned int DEBUG_MASK = 0;



/* 
  -- init swish structure 
*/

SWISH  *SwishNew()
{
    SWISH  *sw;

    /* Default is to write errors to stdout */
    set_error_handle(stdout);

    sw = emalloc(sizeof(SWISH));
    memset(sw, 0, sizeof(SWISH));

    initModule_DB(sw);
    initModule_SearchAlt(sw);
    initModule_ResultSort(sw);
    initModule_Swish_Words(sw);  /* allocate a buffer */

    initModule_Filter(sw);
    initModule_Entities(sw);
    initModule_Index(sw);
    initModule_FS(sw);
    initModule_HTTP(sw);
    initModule_Prog(sw);


    sw->lasterror = RC_OK;
    sw->lasterrorstr[0] = '\0';
    sw->verbose = VERBOSE;
    sw->DefaultDocType = NODOCTYPE;

#ifdef HAVE_ZLIB
    sw->PropCompressionLevel = Z_DEFAULT_COMPRESSION;
#endif



    /* Make rest of lookup tables */
    makeallstringlookuptables(sw);
    return (sw);
}




/* Free memory for search results and parameters (properties ...) */
void    SwishResetSearch(SWISH * sw)
{

    /* Free sort stuff */
    resetModule_ResultSort(sw);

    sw->lasterror = RC_OK;
    sw->lasterrorstr[0] = '\0';
}

void    SwishClose(SWISH * sw)
{
    IndexFILE *tmpindexlist;
    int     i;

    if (sw) {
        /* Free search results and imput parameters */
        SwishResetSearch(sw);

        /* Close any pending DB */
        tmpindexlist = sw->indexlist;
        while (tmpindexlist) {
            if (tmpindexlist->DB)
                DB_Close(sw, tmpindexlist->DB);
            tmpindexlist = tmpindexlist->next;
        }

        freeModule_Swish_Words(sw);
        freeModule_Filter(sw);
        freeModule_SearchAlt(sw);
        freeModule_Entities(sw);
        freeModule_DB(sw);
        freeModule_Index(sw);
        freeModule_ResultSort(sw);
        freeModule_FS(sw);
        freeModule_HTTP(sw);
        freeModule_Prog(sw);



        /* Free MetaNames and close files */
        tmpindexlist = sw->indexlist;

        /* Free ReplaceRules regular expressions */
        free_regex_list(&sw->replaceRegexps);

        /* Free ExtractPath list */
        free_Extracted_Path(sw);

        /* FileRules?? */

        /* meta name for ALT tags */
        if ( sw->IndexAltTagMeta )
        {
            efree( sw->IndexAltTagMeta );
            sw->IndexAltTagMeta = NULL;
        }



        while (tmpindexlist) {

            /* free the property string cache, if used */
            if ( tmpindexlist->prop_string_cache )
            {
                int i;
                for ( i=0; i<tmpindexlist->header.metaCounter; i++ )
                    if ( tmpindexlist->prop_string_cache[i] )
                        efree( tmpindexlist->prop_string_cache[i] );

                efree( tmpindexlist->prop_string_cache );
                tmpindexlist->prop_string_cache = NULL;
            }

            
            /* free the meteEntry array */
            if (tmpindexlist->header.metaCounter)
                freeMetaEntries(&tmpindexlist->header);

            /* Free stopwords structures */
            freestophash(&tmpindexlist->header);
            freeStopList(&tmpindexlist->header);

            freebuzzwordhash(&tmpindexlist->header);

            free_header(&tmpindexlist->header);

            for (i = 0; i < 256; i++)
                if (tmpindexlist->keywords[i])
                    efree(tmpindexlist->keywords[i]);


            tmpindexlist = tmpindexlist->next;
        }

        freeindexfile(sw->indexlist);

        if (sw->Prop_IO_Buf) {
            efree(sw->Prop_IO_Buf);
            sw->Prop_IO_Buf = NULL;
        }

        /* Free SWISH struct */


        freeSwishConfigOptions( sw );  // should be freeConfigOptions( sw->config )
        efree(sw);
    }
}

/**************************************************
* SwishOpen - Create a swish handle
* Returns a swish handle
* Caller much check sw->lasterror for errors
* and call SwishClose() to free memory
**************************************************/


SWISH  *SwishInit(char *indexfiles)
{
    StringList *sl = NULL;
    SWISH  *sw;
    int     i;

    sw = SwishNew();
    if (!indexfiles || !*indexfiles)
    {
        set_progerr(INDEX_FILE_ERROR, sw, "No index file supplied" );
        return sw;
    }
    

    /* Parse out index files, and append to indexlist */
    sl = parse_line(indexfiles);

    if ( 0 == sl->n )
    {
        set_progerr(INDEX_FILE_ERROR, sw, "No index file supplied" );
        return sw;
    }

    

    for (i = 0; i < sl->n; i++)
        addindexfile(sw, sl->word[i]);

    if (sl)
        freeStringList(sl);

    if ( !sw->lasterror )
        SwishAttach(sw);

    return sw;
}


/**************************************************
* SwishOpen - Create a swish handle
* Returns NULL on error -- no error message available
* Frees memory on error
* This is depreciated form
**************************************************/


SWISH  *SwishOpen(char *indexfiles)
{
    SWISH  *sw = SwishInit( indexfiles );

    if ( sw->lasterror )
    {
        SwishClose(sw);
        sw = NULL;
    }

    return sw;
}



/**************************************************
* SwishAttach - Connect to the database
* Returns false on Failure
**************************************************/

int     SwishAttach(SWISH * sw)
{
    IndexFILE *indexlist;

    IndexFILE *tmplist;
 
    indexlist = sw->indexlist;
    sw->TotalWords = 0;
    sw->TotalFiles = 0;


    /* First of all . Read header default values from all index fileis */
    /* With this, we read wordchars, stripchars, ... */
    for (tmplist = indexlist; tmplist;)
    {

        tmplist->DB = (void *)DB_Open(sw, tmplist->line, DB_READ);
        if ( sw->lasterror )
            return 0;

        read_header(sw, &tmplist->header, tmplist->DB);


        sw->TotalWords += tmplist->header.totalwords;
        sw->TotalFiles += tmplist->header.totalfiles;
        tmplist = tmplist->next;
    }

    return ( sw->lasterror == 0 ); 
}




int     SwishSearch(SWISH * sw, char *words, int structure, char *props, char *sort)
{
    StringList *slprops = NULL;
    StringList *slsort = NULL;
    int     i,
            sortmode;
    int     header_level;
    char   *field;

    if (!sw)
    {
        sw->lasterror = INVALID_SWISH_HANDLE;
        return INVALID_SWISH_HANDLE;
    }


    /* If previous search - reset its values (results, props ) */
    SwishResetSearch(sw);

    if (props && props[0]) {
        slprops = parse_line(props);
        for (i = 0; i < slprops->n; i++)
            addSearchResultDisplayProperty(sw, slprops->word[i]);
    }

    if (sort && sort[0]) {
        slsort = parse_line(sort);
        for (i = 0; i < slsort->n;) {
            sortmode = 1;       /* Default mode is ascending */
            field = slsort->word[i++];
            if (i < slsort->n) {
                if (!strcasecmp(slsort->word[i], "asc")) {
                    sortmode = -1; /* Ascending */
                    i++;
                } else {
                    if (!strcasecmp(slsort->word[i], "desc")) {
                        sortmode = 1; /* Ascending */
                        i++;
                    }
                }
            }
            addSearchResultSortProperty(sw, field, sortmode);
        }
    }
    i = 0;

    header_level = sw->ResultOutput->headerOutVerbose;
    sw->ResultOutput->headerOutVerbose = 0;

//    i = search(sw, words, structure); /* search with no eco */

    sw->ResultOutput->headerOutVerbose = header_level;
    if (slsort)
        freeStringList(slsort);
    if (slprops)
        freeStringList(slprops);
    return i;
}




char    tmp_header_buffer[50];  /*  Not thread safe $$$ */

/** Argh!  This is as ugly as the config parsing code **/ 

char   *SwishHeaderParameter(IndexFILE * indexf, char *parameter_name)
{
    if (!strcasecmp(parameter_name, WORDCHARSPARAMNAME))
        return indexf->header.wordchars;

    else if (!strcasecmp(parameter_name, BEGINCHARSPARAMNAME))
        return indexf->header.beginchars;

    else if (!strcasecmp(parameter_name, ENDCHARSPARAMNAME))
        return indexf->header.endchars;

    else if (!strcasecmp(parameter_name, IGNOREFIRSTCHARPARAMNAME))
        return indexf->header.ignorefirstchar;

    else if (!strcasecmp(parameter_name, IGNORELASTCHARPARAMNAME))
        return indexf->header.ignorelastchar;



    else if (!strcasecmp(parameter_name, NAMEHEADERPARAMNAME))
        return indexf->header.indexn;

    else if (!strcasecmp(parameter_name, DESCRIPTIONPARAMNAME))
        return indexf->header.indexd;

    else if (!strcasecmp(parameter_name, POINTERPARAMNAME))
        return indexf->header.indexp;

    else if (!strcasecmp(parameter_name, MAINTAINEDBYPARAMNAME))
        return indexf->header.indexa;

    else if (!strcasecmp(parameter_name, INDEXEDONPARAMNAME))
        return indexf->header.indexedon;



    else if (!strcasecmp(parameter_name, STEMMINGPARAMNAME)) {
        if (indexf->header.fuzzy_mode == FUZZY_STEMMING )
            return "1";
        else
            return "0";

    } else if (!strcasecmp(parameter_name, SOUNDEXPARAMNAME)) {
        if (indexf->header.fuzzy_mode == FUZZY_SOUNDEX )
            return "1";
        else
            return "0";

    } else if (!strcasecmp(parameter_name, FUZZYMODEPARAMNAME)) {
            return fuzzy_mode_to_string( indexf->header.fuzzy_mode );
            

    } else if (!strcasecmp(parameter_name, FILECOUNTPARAMNAME)) {
        sprintf(tmp_header_buffer, "%d", indexf->header.totalfiles);
        return tmp_header_buffer;

    } else
        return "";
}

char  **SwishStopWords(SWISH * sw, char *filename, int *numstops)
{
    IndexFILE *indexf;

    indexf = sw->indexlist;
    while (indexf) {
        if (!strcasecmp(indexf->line, filename)) {
            *numstops = indexf->header.stopPos;
            return indexf->header.stopList;
        }
    }
    *numstops = 0;
    return NULL;
}

char   *SwishWords(SWISH * sw, char *filename, char c)
{
    IndexFILE *indexf;

    indexf = sw->indexlist;
    while (indexf) {
        if (!strcasecmp(indexf->line, filename)) {
            return getfilewords(sw, c, indexf);
        }
    }
    return "";
}
