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
#include "string.h"
#include "hash.h"
#include "mem.h"
#include "search.h"

/* Hashes a string.
*/

unsigned hash(s)
     char   *s;
{
    unsigned hashval;

    for (hashval = 0; *s != '\0'; s++)
        hashval = (int) ((unsigned char) *s) + 31 * hashval;
    return hashval % HASHSIZE;
}

/* Hashes a string for a larger hash table.
*/

unsigned bighash(s)
     char   *s;
{
    unsigned hashval;

    for (hashval = 0; *s != '\0'; s++)
        hashval = (int) ((unsigned char) *s) + 31 * hashval;
    return hashval % BIGHASHSIZE;
}

/* Hashes a int.
*/

unsigned numhash(i)
     int     i;
{
    return i % HASHSIZE;
}

/* Hashes a int for a larger hash table.
*/

unsigned bignumhash(i)
     int     i;
{
    return i % BIGHASHSIZE;
}

/* Hashes a string for a larger hash table (for search).
*/

unsigned verybighash(s)
     char   *s;
{
    unsigned hashval;

    for (hashval = 0; *s != '\0'; s++)
        hashval = (int) ((unsigned char) *s) + 31 * hashval;
    return hashval % VERYBIGHASHSIZE;
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

void add_word_to_hash_table( WORD_HASH_TABLE *table_ptr, char *word)
{
    struct swline **hash_array = table_ptr->hash_array;
    unsigned hashval;
    struct swline *sp;

    hashval = hash(word);


    /* Create the array if it doesn't exist */
    if ( !hash_array )
    {
        int ttl_bytes = sizeof(struct swline *) * HASHSIZE;
        
        hash_array = (struct swline  **)emalloc( ttl_bytes );
        memset( hash_array, 0, ttl_bytes );
        table_ptr->hash_array = hash_array;
        table_ptr->count = 0;
    }
    else
        if ( is_word_in_hash_table( *table_ptr, word ) )
            return;

    /* Create a new entry */            
    sp = (struct swline *) emalloc(sizeof(struct swline));

    sp->line = (char *) estrdup(word);

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
*       true if word found
*
*******************************************************************/

int is_word_in_hash_table( WORD_HASH_TABLE table, char *word)
{
    unsigned hashval;
    struct swline *sp;

    if ( !table.hash_array )
        return 0;

    hashval = hash(word);
    sp = table.hash_array[hashval];

    while (sp != NULL)
    {
        if (!strcmp(sp->line, word))
            return 1;
        sp = sp->next;
    }
    return 0;
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
    int     i;
    struct swline *sp,
           *tmp;

    if ( !hash_array )
        return;

    for (i = 0; i < HASHSIZE; i++)
    {
        if ( !hash_array[i])
            continue;
            
        sp = hash_array[i];
        while (sp)
        {
            tmp = sp->next;
            efree(sp->line);
            efree(sp);
            sp = tmp;
        }
    }
    efree( hash_array );
    
    table_ptr->hash_array = NULL;
    table_ptr->count = 0;
}

