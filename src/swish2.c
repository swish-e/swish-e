/*
** $Id$
**
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**

** Mon May  9 11:06:18 CDT 2005
karman : NOTE that none of this looks like Kevin's code. Is it all Bill's
and simply mis-labeled?
**

    This file is part of Swish-e.

    Swish-e is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Swish-e is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along  with Swish-e; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    Swish-e includes a library for searching with a well-defined API. The library
    is named libswish-e.
    
    Linking libswish-e statically or dynamically with other modules is making a
    combined work based on Swish-e.  Thus, the terms and conditions of the GNU
    General Public License cover the whole combination.

    As a special exception, the copyright holders of Swish-e give you
    permission to link Swish-e with independent modules that communicate with
    Swish-e solely through the libswish-e API interface, regardless of the license
    terms of these independent modules, and to copy and distribute the
    resulting combined work under terms of your choice, provided that
    every copy of the combined work is accompanied by a complete copy of
    the source code of Swish-e (the version of Swish-e used to produce the
    combined work), being distributed under the terms of the GNU General
    Public License plus this exception.  An independent module is a module
    which is not derived from or based on Swish-e.

    Note that people who make modified versions of Swish-e are not obligated
    to grant this special exception for their modified versions; it is
    their choice whether to do so.  The GNU General Public License gives
    permission to release a modified version without this exception; this
    exception also makes it possible to release a modified version which
    carries forward this exception.
    
Mon May  9 10:57:22 CDT 2005 -- added GPL notice


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
*
**************************************************************************/


/* 
  -- init swish structure 
            indexf->header.fuzzy_data = set_fuzzy_mode( indexf->header.fuzzy_data, sl->word[1] );
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
    /* karman added -W opt which will override warn_level default at cmd line */
    sw->parser_warn_level = 2; /* report if libxml2 aborts processing a document. */
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

    /* free the in-use cached meta list */
    if ( indexf->meta_list )
      efree(indexf->meta_list);

    /* free the in-use cached property list */
    if ( indexf->prop_list )
      efree(indexf->prop_list);

    /* free data loaded into header */
    free_header(&indexf->header);


    /* free array of words for each letter (-k) $$$ eight bit */
    for (i = 0; i < 256; i++)
        if ( indexf->keywords[i])
            efree(indexf->keywords[i]);


    /* free the name of the index file */
    efree( indexf->line );

    /* free the stem cache if any */
    free_word_hash_table( &indexf->hashstemcache);

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
* SwishInit -- create a swish handle for the indexe or indexes passed in
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
*  Maybe this could be passed a variable length of arguments
*  so that swis.c:cmd_index() could call SwishAttach( sw, DB_READWRITE )
*  to have all similar code in one place.
*
* Returns false on Failure
**************************************************/

int     SwishAttach(SWISH * sw)
{
    IndexFILE *indexlist = sw->indexlist;  /* head of list of indexes */
    IndexFILE *tmplist;


    /* First of all . Read header default values from all index files */
    /* With this, we read wordchars, stripchars, ... */
    for (tmplist = indexlist; tmplist;)
        if ( !open_single_index( sw, tmplist, DB_READ ) )
            return 0;
        else
            tmplist = tmplist->next;


    return ( sw->lasterror == 0 );
}

/****************************************************************************
* open_single_index -- opens the index and reads in its header data
*
* Pass:
*   sw
*   indexf
*   db_mode  - open in read or read/write
*
* Returns
*   true if ok.
*
****************************************************************************/

int open_single_index( SWISH *sw, IndexFILE *indexf, int db_mode )
{
    INDEXDATAHEADER *header = &indexf->header;


    indexf->DB = (void *)DB_Open(sw, indexf->line, db_mode);

    if ( sw->lasterror )
        return 0;

    read_header(sw, header, indexf->DB);

    /* These values are used in ranking */

    sw->TotalFiles   += header->totalfiles - header->removedfiles;
    sw->TotalWordPos += header->total_word_positions - header->removed_word_positions;

    return 1;
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

/********************************************************************************
* SwishGetRefPtr - for use the SWISH::API to get the SV* of the swish handle
*
********************************************************************************/

void *SwishGetRefPtr( SWISH *sw )
{
    return sw->ref_count_ptr;
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




