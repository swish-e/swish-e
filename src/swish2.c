/*
** $Id$
**
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

#include "swish.h"
#include "swstring.h"
#include "mem.h"
#include "error.h"
#include "list.h"
#include "search.h"
#include "file.h"
#include "merge.h"
#include "docprop.h"
#include "hash.h"
/* #include "search_alt.h" */
#include "db.h"
#include "swish_words.h"
#include "metanames.h"
#include "proplimit.h"
#include "stemmer.h"
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

static IndexFILE *free_index( IndexFILE *indexf );



/* Moved here so it's in the library */
unsigned int DEBUG_MASK = 0;

/*************************************************************************
* SwishNew -- create a search swish structure
*
*  NOTE: ANY CHANGES HERE SHOULD ALSO BE MADE IN swish.c swish_new
*
*  $$$ Please join these routines!
*
**************************************************************************/


/* 
  -- init swish structure 
*/

SWISH  *SwishNew()
{
    SWISH  *sw;

    sw = emalloc(sizeof(SWISH));
    memset(sw, 0, sizeof(SWISH));

    initModule_DB(sw);
    initModule_Swish_Words(sw);  /* allocate a buffer */


    sw->lasterror = RC_OK;
    sw->lasterrorstr[0] = '\0';
    sw->verbose = VERBOSE;
    sw->headerOutVerbose = 1;
    
    sw->DefaultDocType = NODOCTYPE;

#ifdef HAVE_ZLIB
    sw->PropCompressionLevel = Z_DEFAULT_COMPRESSION;
#endif



    /* Make rest of lookup tables */
    makeallstringlookuptables(sw);  /* isvowel */
    return (sw);
}



static IndexFILE *free_index( IndexFILE *indexf )
{
    IndexFILE  *next = indexf->next;
    SWISH      *sw = indexf->sw;
    int         i;
    
    /* Close any pending DB */
    if ( indexf->DB )
        DB_Close(sw, indexf->DB);


    /* free the meteEntry array */
    if ( indexf->header.metaCounter)
        freeMetaEntries(&indexf->header);


    /* free and close all stemmed related stuff */
    free_fuzzy_mode(&indexf->header.fuzzy_data);

    
    /* free data loaded into header */
    free_header(&indexf->header);


    /* free array of words for each letter (-k) $$$ eight bit */
    for (i = 0; i < 256; i++)
        if ( indexf->keywords[i])
            efree(indexf->keywords[i]);


    /* free the name of the index file */
    efree( indexf->line );

    /* finally free up the index itself */
    efree( indexf );

    return next;
}

void free_swish_memory( SWISH *sw )
{
    IndexFILE *cur_indexf;


    /* Free up associated index file */
    cur_indexf = sw->indexlist;

    while (cur_indexf)
        cur_indexf = free_index( cur_indexf );


    /* Common to searching and indexing */
    freeModule_Swish_Words(sw);
    freeModule_DB(sw);


    /* Free temporary buffers -- mostly used for the library API to pass data to users */

    if (sw->Prop_IO_Buf) {
        efree(sw->Prop_IO_Buf);
        sw->Prop_IO_Buf = NULL;
    }

    if ( sw->header_names )
        efree( sw->header_names );

    if ( sw->index_names )
        efree( sw->index_names );

    if ( sw->temp_string_buffer )
        efree( sw->temp_string_buffer );


    if ( sw->stemmed_word )
        efree( sw->stemmed_word );

}

/*************************************************************************
* SwishClose -- frees up the swish handle
*
*
**************************************************************************/



void    SwishClose(SWISH * sw)
{

    if ( !sw )
        return;


    free_swish_memory( sw );

    efree(sw);
}




/*************************************************************************
* SwishInit -- create a swish hanlde
*
*
**************************************************************************/


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
* SwishAttach - Connect to the database
*  This just opens the index files
*
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

/********************************************************************************
* SwishSetRefPtr - for use the SWISH::API to save the SV* of the swish handle
*
********************************************************************************/

void SwishSetRefPtr( SWISH *sw, void *address )
{
    if ( !address )
        progerr("SwishSetRefPtr - passed null address");

    sw->ref_count_ptr = address;
}


/*********************************************************************************
* SwishWords -- returns all the words that begin with the specified character
*
*
**********************************************************************************/

const char *SwishWordsByLetter(SWISH * sw, char *filename, char c) 
{ 
    IndexFILE *indexf;

    indexf = sw->indexlist;
    while (indexf) {
        if (!strcasecmp(indexf->line, filename)) {
            return getfilewords(sw, c, indexf);
        }
        indexf = indexf->next;
    }
    /* Not really an "WORD_NOT_FOUND" error */
    set_progerr(WORD_NOT_FOUND, sw, "Invalid index file '%s' passed to SwishWordsByLetter", filename );
    return NULL;
}

/*************************************************************************
* SwishStemWord -- utility function to stem a word
*
* This stores the stemmed word locally so it can be freed
*
**************************************************************************/

char *SwishStemWord( SWISH *sw, char *word )
{
    int  length = strlen( word ) + 100;

    if ( !word )
        return NULL;

    if ( length > sw->stemmed_word_len )
    {
        sw->stemmed_word_len = length+1;
        sw->stemmed_word = erealloc( sw->stemmed_word, sw->stemmed_word_len );
    }

    strcpy( sw->stemmed_word, word );

    /* Default to english stemmer */
    if ( ! sw->indexlist->header.fuzzy_data.fuzzy_routine )
        set_fuzzy_mode( &sw->indexlist->header.fuzzy_data, "Stemming_en" );

    /* set return value only if stem returns OK */
    if ( sw->indexlist->header.fuzzy_data.fuzzy_routine(&sw->stemmed_word, &sw->stemmed_word_len,sw->indexlist->header.fuzzy_data.fuzzy_args) == STEM_OK )
        return sw->stemmed_word;

    return NULL;
}

