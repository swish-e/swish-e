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
#include "swstring.h"
#include "hash.h"
#include "mem.h"
#include "search.h"

/* Hashes a string. Common routine
*/

unsigned string_hash(char *s, int hash_size)
{
    unsigned hashval;

    for (hashval = 0; *s != '\0'; s++)
        hashval = (int) ((unsigned char) *s) + 31 * hashval;
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

/* Hashes a int. Common routine
*/
unsigned int_hash(int i, int hash_size)
{
    return i % hash_size;
}

/* Hashes a int.
*/
unsigned numhash(int i)
{
    return int_hash(i, HASHSIZE);
}

/* Hashes a int for a larger hash table.
*/
unsigned bignumhash(int i)
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
*       void;
*******************************************************************/

void add_word_to_hash_table( WORD_HASH_TABLE *table_ptr, char *word, int hash_size)
{
    struct swline **hash_array = table_ptr->hash_array;
    unsigned hashval;
    struct swline *sp;
    int len;

    /* Create the array if it doesn't exist */
    if ( !hash_array )
    {
        int ttl_bytes = sizeof(struct swline *) * (hash_size = (hash_size ? hash_size : HASHSIZE));
       
        table_ptr->mem_zone = (void *) Mem_ZoneCreate("Word Hash Zone", 0, 0); 
        //hash_array = (struct swline  **)emalloc( ttl_bytes );
        hash_array = (struct swline  **) Mem_ZoneAlloc( (MEM_ZONE *)table_ptr->mem_zone, ttl_bytes );
        memset( hash_array, 0, ttl_bytes );
        table_ptr->hash_array = hash_array;
        table_ptr->hash_size = hash_size;
        table_ptr->count = 0;
    }
    else
        if ( is_word_in_hash_table( *table_ptr, word ) )
            return;

    hashval = string_hash(word,hash_size);

    /* Create a new entry */            
    sp = (struct swline *) Mem_ZoneAlloc((MEM_ZONE *)table_ptr->mem_zone, sizeof(struct swline));

    len = strlen(word) + 1;
    sp->line = (char *) Mem_ZoneAlloc((MEM_ZONE *)table_ptr->mem_zone, len);
    memcpy(sp->line,word,len);

    /* Add word to head of list */
    
    sp->next = hash_array[hashval];
    hash_array[hashval] = sp;

    table_ptr->count++;
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
    int             hash_size = table_ptr->hash_size;
    int     i;
    struct swline *sp,
           *tmp;

    if ( !hash_array )
        return;

    Mem_ZoneFree((MEM_ZONE **)&table_ptr->mem_zone);
    table_ptr->hash_array = NULL;
    table_ptr->hash_size = 0;
    table_ptr->count = 0;
}

