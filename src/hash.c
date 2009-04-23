/*
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**
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
    
    See the COPYING file that accompanies the Swish-e distribution for details
    of the GNU GPL and the special exception available for linking against
    the Swish-e library.
    
** Mon May  9 15:51:39 CDT 2005
** added GPL


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
#include "swstring.h"
#include "hash.h"
#include "mem.h"
#include "search.h"

/* Hashes a string. Common routine
*/

unsigned string_hash(char *s, SWINT_T hash_size)
{
    unsigned hashval;

    for (hashval = 0; *s != '\0'; s++)
        hashval = (SWINT_T) ((unsigned char) *s) + 31 * hashval;
    return hashval % hash_size;
}

/* Hashes a string.
*/
unsigned hash(char *s)
{
    return string_hash(s,HASHSIZE);
}

/* Hashes a string for a larger hash table.
*/
unsigned bighash(char *s)
{
    return string_hash(s,BIGHASHSIZE);
}

/* Hashes a SWINT_T. Common routine
*/
unsigned int_hash(SWINT_T i, SWINT_T hash_size)
{
    return i % hash_size;
}

/* Hashes a SWINT_T.
*/
unsigned numhash(SWINT_T i)
{
    return int_hash(i, HASHSIZE);
}

/* Hashes a SWINT_T for a larger hash table.
*/
unsigned bignumhash(SWINT_T i)
{
    return int_hash(i, BIGHASHSIZE);
}

/* Hashes a string for a larger hash table (for search).
*/
unsigned verybighash(char *s)
{
    return string_hash(s, VERYBIGHASHSIZE);
}


/******************************************************************
* add_word_to_hash_table -  Adds a word to a hash table.
*
*   Call with:
*       address of an array of swline pointers
*
*   Returns:
*       swline that was added
*******************************************************************/

struct swline *add_word_to_hash_table( WORD_HASH_TABLE *table_ptr, char *word, SWINT_T hash_size)
{
    struct swline **hash_array = table_ptr->hash_array;
    unsigned hashval;
    struct swline *sp;
    SWINT_T len;

    /* Create the array if it doesn't exist */
    if ( !hash_array )
    {
        SWINT_T ttl_bytes = sizeof(struct swline *) * (hash_size = (hash_size ? hash_size : HASHSIZE));
       
        table_ptr->mem_zone = (void *) Mem_ZoneCreate("Word Hash Zone", 0, 0); 
        //hash_array = (struct swline  **)emalloc( ttl_bytes );
        hash_array = (struct swline  **) Mem_ZoneAlloc( (MEM_ZONE *)table_ptr->mem_zone, ttl_bytes );
        memset( hash_array, 0, ttl_bytes );
        table_ptr->hash_array = hash_array;
        table_ptr->hash_size = hash_size;
        table_ptr->count = 0;
    }
    else
        if ( (sp = is_word_in_hash_table( *table_ptr, word )) )
            return sp;

    hashval = string_hash(word,hash_size);

    /* Create a new entry */            
    len = strlen(word);
    sp = (struct swline *) Mem_ZoneAlloc((MEM_ZONE *)table_ptr->mem_zone, sizeof(struct swline) + len);

    memcpy(sp->line,word,len + 1);

    /* Add word to head of list */
    
    sp->next = hash_array[hashval];
    hash_array[hashval] = sp;

    table_ptr->count++;

    return sp;
}

/******************************************************************
* is_word_in_hash_table -
*
*   Call with:
*       array of swline pointers
*
*   Returns:
*       true (swline) if word found, NULL if not found
*
*******************************************************************/

struct swline * is_word_in_hash_table( WORD_HASH_TABLE table, char *word)
{
    unsigned hashval;
    struct swline *sp;

    if ( !table.hash_array )
        return 0;

    hashval = string_hash(word, table.hash_size);
    sp = table.hash_array[hashval];

    while (sp != NULL)
    {
        if (!strcmp(sp->line, word))
            return sp;
        sp = sp->next;
    }
    return NULL;
}

/******************************************************************
* free_word_hash_table - 
*
*   Call with:
*       address of an array of swline pointers
*
*   Returns:
*       true if word found
*
*******************************************************************/

void free_word_hash_table( WORD_HASH_TABLE *table_ptr)
{
    struct swline **hash_array = table_ptr->hash_array;

    if ( !hash_array )
        return;

    Mem_ZoneFree((MEM_ZONE **)&table_ptr->mem_zone);
    table_ptr->hash_array = NULL;
    table_ptr->hash_size = 0;
    table_ptr->count = 0;
}

