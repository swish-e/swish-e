/*
** $Id$
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
#include "headers.h"
#include "stemmer.h"

#include <stddef.h>  /* for offsetof macro */
#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif




/***************************************************************************
*  Routines for accessing index header data by name and header name
*
****************************************************************************/




typedef struct
{
    const char         *description;
    SWISH_HEADER_TYPE   data_type;
    int                 min_verbose_level;
    size_t              offset;
} HEADER_MAP;



static HEADER_MAP header_map[] = {
    {  "Name",              SWISH_STRING, 2,  offsetof( INDEXDATAHEADER, indexn ) },
    {  "Saved as",          SWISH_STRING, 2,  offsetof( INDEXDATAHEADER, savedasheader ) },
    {  "Total Words",       SWISH_NUMBER, 2,  offsetof( INDEXDATAHEADER, totalwords ) },
    {  "Total Files",       SWISH_NUMBER, 2,  offsetof( INDEXDATAHEADER, totalfiles ) },
    {  "Total Word Pos",    SWISH_NUMBER, 2,  offsetof( INDEXDATAHEADER, total_word_positions ) },
    {  "Indexed on",        SWISH_STRING, 2,  offsetof( INDEXDATAHEADER, indexedon ) },
    {  "Description",       SWISH_STRING, 2,  offsetof( INDEXDATAHEADER, indexd ) },
    {  "Pointer",           SWISH_STRING, 2,  offsetof( INDEXDATAHEADER, indexp ) },
    {  "Maintained by",     SWISH_STRING, 2,  offsetof( INDEXDATAHEADER, indexa ) },
    {  "MinWordLimit",      SWISH_NUMBER, 2,  offsetof( INDEXDATAHEADER, minwordlimit ) },
    {  "MaxWordLimit",      SWISH_NUMBER, 2,  offsetof( INDEXDATAHEADER, maxwordlimit ) },
    {  "WordCharacters",    SWISH_STRING, 2,  offsetof( INDEXDATAHEADER, wordchars ) },
    {  "BeginCharacters",   SWISH_STRING, 2,  offsetof( INDEXDATAHEADER, beginchars ) },
    {  "EndCharacters",     SWISH_STRING, 2,  offsetof( INDEXDATAHEADER, endchars ) },
    {  "IgnoreFirstChar",   SWISH_STRING, 2,  offsetof( INDEXDATAHEADER, ignorefirstchar ) },
    {  "IgnoreLastChar",    SWISH_STRING, 2,  offsetof( INDEXDATAHEADER, ignorelastchar ) },
    {  "StopWords",         SWISH_WORD_HASH, 3,  offsetof( INDEXDATAHEADER, hashstoplist ) },
    {  "BuzzWords",         SWISH_WORD_HASH, 3,  offsetof( INDEXDATAHEADER, hashbuzzwordlist ) },
    {  "Stemming Applied", SWISH_OTHER_DATA, 2, offsetof( INDEXDATAHEADER, fuzzy_data ) },
    {  "Soundex Applied",   SWISH_OTHER_DATA, 2, offsetof( INDEXDATAHEADER, fuzzy_data ) },
    {  "Fuzzy Mode",        SWISH_OTHER_DATA, 2, offsetof( INDEXDATAHEADER, fuzzy_data ) },
    {  "IgnoreTotalWordCountWhenRanking", SWISH_BOOL, 2, offsetof( INDEXDATAHEADER, ignoreTotalWordCountWhenRanking ) }
};




static SWISH_HEADER_VALUE fetch_header( IndexFILE *indexf, const char *name,  SWISH_HEADER_TYPE *data_type  );
static const char **create_string_list( SWISH *sw, struct swline *swline );
static const char **string_list_from_hash( SWISH *sw, WORD_HASH_TABLE table );

static DB_RESULTS *db_results_by_name( RESULTS_OBJECT *results, const char *index_name );

static SWISH_HEADER_VALUE fetch_single_header( IndexFILE *indexf, HEADER_MAP *header_map, SWISH_HEADER_TYPE *data_type );
static void print_header_value( SWISH *sw, const char *name, SWISH_HEADER_VALUE head_value, SWISH_HEADER_TYPE head_type );




/*********** PUBLIC *************************/


/***********************************************************************
*  print_index_headers - prints all the headers for a given indexf
*
*  Called by swish-e binary (swish.c)
*
************************************************************************/

void print_index_headers( IndexFILE *indexf )
{
    int i;
    int array_size = sizeof(header_map) / sizeof(header_map[0]);
    SWISH_HEADER_VALUE value;
    SWISH_HEADER_TYPE data_type;
    int verbose_level = indexf->sw->headerOutVerbose;

   
    for (i = 0; i < array_size; i++)
    {
        if ( header_map[i].min_verbose_level > verbose_level )
            continue;

        value = fetch_single_header( indexf, &header_map[i], &data_type );
        print_header_value( indexf->sw, header_map[i].description, value, data_type );
    }
}




/*********************************************************************
* SwishHeaderNames -- return a list of possible header names
*
**********************************************************************/

const char **SwishHeaderNames(SWISH *sw)
{
    int array_size = sizeof(header_map) / sizeof(header_map[0]);
    int i;

    if ( !sw )
        progerr("SwishHeaderNames requires a valid swish handle");
        

    if ( sw->header_names )
        return sw->header_names;

    sw->header_names = (const char **)emalloc( sizeof(char *) * ( 1 + array_size ) );        

   
    for (i = 0; i < array_size; i++)
        sw->header_names[i] = (const char *)header_map[i].description;

    sw->header_names[i] = NULL;

    return sw->header_names;
}
        
        
/*********************************************************************
* SwishIndexNames -- return a list of associated index file names
*
**********************************************************************/

    
const char **SwishIndexNames(SWISH *sw)
{
    IndexFILE          *indexf;
    int                 index_count;

    if ( !sw )
        progerr("SwishIndexNames requires a valid swish handle");
    

    if ( sw->index_names )
        return sw->index_names;


    for ( index_count = 0, indexf = sw->indexlist; indexf; indexf = indexf->next )
        index_count++;
        
    if ( !index_count ) /* should not happen */
        progerr("Swish Handle does not have any associated index files!?!?");

    sw->index_names = (const char **)emalloc( sizeof(char *) * (1+index_count) );

    for ( index_count = 0, indexf = sw->indexlist; indexf; indexf = indexf->next )
        sw->index_names[index_count++] = (const char *)indexf->line;

    sw->index_names[index_count] = NULL;
    return sw->index_names;
}


/*********************************************************************
* SwishResultIndexValue -- lookup a header via a result structure
*
**********************************************************************/

SWISH_HEADER_VALUE SwishResultIndexValue( RESULT *result, const char *name, SWISH_HEADER_TYPE *data_type )
{
    return fetch_header( result->db_results->indexf, name, data_type );
}




/********************************************************************************
* SwishHeaderValue -- lookup a header via a index file name, and a header name
*
*********************************************************************************/

SWISH_HEADER_VALUE SwishHeaderValue( SWISH *sw, const char *index_name, const  char *cur_header, SWISH_HEADER_TYPE *data_type )
{
    IndexFILE          *indexf;
    SWISH_HEADER_VALUE  value;

    value.string = NULL;

    if ( !sw )
        progerr("SwishHeaderValue requires a valid swish handle");

    indexf = indexf_by_name( sw, index_name );

    if ( indexf )
        return fetch_header( indexf, cur_header, data_type );

    *data_type = SWISH_HEADER_ERROR;
    set_progerr( HEADER_READ_ERROR, sw, "Index file '%s' is not an active index file", index_name );
    return value;
}






/************* Local support function **********************************/

                  



/****************************************************************************
* fetch_single_header -- returns a SWISH_HEADER_VALUE
*
*   Pass:
*       indexf, HEADER_MAP element, and a *data_type
*
*   Return:
*       SWISH_HEADER_VALUE
*   
*
*****************************************************************************/

static SWISH_HEADER_VALUE fetch_single_header( IndexFILE *indexf, HEADER_MAP *header_map, SWISH_HEADER_TYPE *data_type )
{
    SWISH_HEADER_VALUE value;
    INDEXDATAHEADER    *header = &indexf->header;

    value.string = NULL;

    *data_type = header_map->data_type;

    switch ( header_map->data_type )
    {
        case SWISH_STRING:
            value.string = *(const char **) ( (char *)header + header_map->offset );
            return value;

        case SWISH_NUMBER:
        case SWISH_BOOL:
            value.number = *(unsigned long *) ( (char *)header + header_map->offset );
            return value;

        case SWISH_LIST:
        {
            struct swline *first_item = *(struct swline **)( (char *)header + header_map->offset );
            value.string_list = create_string_list( indexf->sw, first_item );
            return value;
        }

        case SWISH_WORD_HASH:
        {
            WORD_HASH_TABLE table = *(WORD_HASH_TABLE *)( (char *)header + header_map->offset );
            *data_type = SWISH_LIST;
            value.string_list = string_list_from_hash( indexf->sw, table ); 
            return value;
        }




        case SWISH_OTHER_DATA:
            if ( strcasecmp( "Fuzzy Mode", header_map->description ) == 0 )
            {
                value.string = fuzzy_string( header->fuzzy_data );
                *data_type = SWISH_STRING;
                return value;

            }

            else if ( strcasecmp( "Stemming Applied", header_map->description ) == 0 )
            {
                value.number = stemmer_applied( header->fuzzy_data );

                *data_type = SWISH_BOOL;
                return value;
            }

            else if ( strcasecmp( "Soundex Applied", header_map->description ) == 0 )
            {
                value.number = FUZZY_SOUNDEX == fuzzy_mode_value( header->fuzzy_data ) ? 1 : 0;
                *data_type = SWISH_BOOL;
                return value;
            }
            else
                progerr("Invalid OTHER header '%s'", header_map->description );


        default:
            progerr("Invalid HEADER type '%d'", header_map->data_type );
    }

    return value;  /* make MS compiler happy */
}

/************************************************************
* fetch_header - fetches a header by name
*
*************************************************************/
    

static SWISH_HEADER_VALUE fetch_header( IndexFILE *indexf, const char *name,  SWISH_HEADER_TYPE *data_type  )
{
    int i;
    int array_size = sizeof(header_map) / sizeof(header_map[0]);
    SWISH_HEADER_VALUE value;

    value.string = NULL;
   
    for (i = 0; i < array_size; i++)
    {
        if ( strcasecmp(header_map[i].description, name) != 0 )
            continue;  /* nope */

        return fetch_single_header( indexf, &header_map[i], data_type );

    }

    *data_type = SWISH_HEADER_ERROR;
    set_progerr( HEADER_READ_ERROR, indexf->sw, "Index file '%s' does not have header '%s'", indexf->line, name );
    return value;
}







SWISH_HEADER_VALUE SwishParsedWords( RESULTS_OBJECT *results, const char *index_name )
{
    SWISH_HEADER_VALUE value;
    DB_RESULTS *db_results;

    if ( !results )
        progerr("Must pass a results object to SwishParsedWords");

    value.string_list = NULL;        

    db_results = db_results_by_name( results, index_name );

    if ( db_results )
        value.string_list = create_string_list( results->sw, db_results->parsed_words );

    return value;
}

SWISH_HEADER_VALUE SwishRemovedStopwords( RESULTS_OBJECT *results, const char *index_name )
{
    SWISH_HEADER_VALUE value;
    DB_RESULTS *db_results;

    if ( !results )
        progerr("Must pass a results object to SwishRemovedStopwords");

    value.string_list = NULL;        

    db_results = db_results_by_name( results, index_name );

    if ( db_results )
        value.string_list = create_string_list( results->sw, db_results->removed_stopwords );
    return value;
}




/**********************************************************************
* create_string_list - creates a list of strings from a swline
* $$$ should this return NULL if there's none to return, or an empty list?
*
***********************************************************************/

static const char **create_string_list( SWISH *sw, struct swline *swline )
{
    int i;
    struct swline *cur_item;

    /* first count up how many items there are */

    i = 1;  /* always need one */
    for ( cur_item = swline; cur_item; cur_item = cur_item->next )
        i++;

    if ( i > sw->temp_string_buffer_len )
    {
        sw->temp_string_buffer_len = i;
        sw->temp_string_buffer = (const char **)erealloc( sw->temp_string_buffer, sizeof(char *) * i );
    }

    i = 0;
    for ( cur_item = swline; cur_item; cur_item = cur_item->next )
        sw->temp_string_buffer[i++] = (const char*)cur_item->line;

    sw->temp_string_buffer[i] = NULL;  /* end of list */

    return sw->temp_string_buffer;
}

/**********************************************************************
* string_list_from_hash - creates a list of strings from a swline
* $$$ should this return NULL if there's none to return, or an empty list?
*
***********************************************************************/


static const char **string_list_from_hash( SWISH *sw, WORD_HASH_TABLE table )
{
    int i;
    struct swline *sp, *next;
    int count;

    i = table.count + 1;  /* always return one */

    if ( i > sw->temp_string_buffer_len )
    {
        sw->temp_string_buffer_len = i;
        sw->temp_string_buffer = (const char **)erealloc( sw->temp_string_buffer, sizeof(char *) * i );
    }
     
    /* first count them up */
    count = 0;

    if ( table.count )
    {
        for (i = 0; i < HASHSIZE; i++)
        {
            if ( !table.hash_array[i])
                continue;
            
            sp = table.hash_array[i];
            while (sp)
            {
                next = sp->next;
                sw->temp_string_buffer[count++] = sp->line;
                sp = next;
            }
        }
    }

    sw->temp_string_buffer[count] = NULL; /* end of list */

    return sw->temp_string_buffer;

}
    

    
    
/* no longer static since we want to use this in metanames.c and stemmer.c
ther'es probably a better way to organize this... karman Mon Nov  8 21:37:44 CST 2004
*/

IndexFILE *indexf_by_name( SWISH *sw, const char *index_name )
{
    IndexFILE *indexf = sw->indexlist;

    while ( indexf )
    {
        if (strcmp( index_name, indexf->line ) == 0 )
            return indexf;

        indexf = indexf->next;
    }
    return NULL;
}

static DB_RESULTS *db_results_by_name( RESULTS_OBJECT *results, const char *index_name )
{
    DB_RESULTS *db_results = results->db_results;

    while ( db_results )
    {
        if (strcmp( index_name, db_results->indexf->line ) == 0)
            return db_results;

        db_results = db_results->next;
    }
    return NULL;
}


/**************************************************************************
* print_index_headers - prints list of headers for the given index
*
*   Note:
*       This is not used in the library code.  Perhaps move elsewhere
*
***************************************************************************/


static void print_header_value( SWISH *sw, const char *name, SWISH_HEADER_VALUE head_value, SWISH_HEADER_TYPE head_type )
{
    const char **string_list;
    
    printf("# %s:", name );

    switch ( head_type )
    {
        case SWISH_STRING:
            printf(" %s\n", head_value.string ? head_value.string : "" );
            return;

        case SWISH_NUMBER:
            printf(" %lu\n", head_value.number );
            return;

        case SWISH_BOOL:
            printf(" %s\n", head_value.boolean ? "1" : "0" );
            // printf(" %s\n", head_value.boolean ? "Yes" : "No" );
            return;

        case SWISH_LIST:
            string_list = head_value.string_list;
            
            while ( *string_list )
            {
                printf(" %s", *string_list );
                string_list++;
            }
            printf("\n");
            return;

        case SWISH_HEADER_ERROR:
            SwishAbortLastError( sw );

        default:
            printf(" Unknown header type '%d'\n", (int)head_type );
            return;
    }
}


