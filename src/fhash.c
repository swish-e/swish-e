/*

$Id$

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

*/



/**************************************************************************
** 05-2002 jmruiz                                                        
**
** Very simple routines to maintain a hash database on disk.
**
** Default values (see fhash.h):
**       FHASH_SIZE - Size of hash table
**
** Routines:
**       FHASH *FHASH_Create(FILE *fp);
**           Creates a db hash file in file fp
**           Returns a pointer to hash db file
**
**       FHASH *FHASH_Open(FILE *fp, sw_off_t start);
**           Opens an existing hash db file in file fp, starting at offset
**               start.
**           Returns a pointer to hash db file
**       sw_off_t FHASH_Close(FHASH *f);
**           Closes and writes the hash table to the file
**           Returns the pointer to the db hash file inside the file
**
**       SWINT_T FHASH_Insert(FHASH *f, unsigned char *key, SWINT_T key_len, unsigned char *data, SWINT_T data_len);
**           Adds a new entry pair (key,data) to the hash db file. Length of 
**               key is key_len, length of data is data_len
**
**       SWINT_T FHASH_Search(FHASH *f, unsigned char *key, SWINT_T key_len, unsigned char *data, SWINT_T data_len);
**           Searchs and returns the data for a given key of length key_len
**           The data is returned in the data array that must be allocated by
**               the caller. If data buffer is not SWINT_T enough, data will
**               be truncated
**           Returns the copied length in data
**
**       SWINT_T FHASH_Update(FHASH *f, unsigned char *key, SWINT_T key_len, unsigned char *data, SWINT_T data_len);
**           Updates data for a given key of length key_len
**           Data must be of the same size of the original record
**           Returns 0 (OK) or 1 (no OK)
**
**       SWINT_T FHASH_Delete(FHASH *f, unsigned char *key, SWINT_T key_len);
**           Deletes the entry for the given key of length key_len
**           Returns 0 (OK) or 1 (no OK)
**
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "swish.h"
#include "compress.h"
#include "mem.h"
#include "fhash.h"
#include "error.h"

FHASH *FHASH_Create(FILE *fp)
{
FHASH *f;
SWUINT_T i;
sw_off_t tmp = (sw_off_t)0;

    f = (FHASH *) emalloc(sizeof(FHASH));

    /* Init hash table */
    for(i = 0; i < FHASH_SIZE; i++)
       f->hash_offsets[i] = (sw_off_t)0;

    /* Go to the end of the file */
    if(sw_fseek(fp,(sw_off_t)0,SEEK_END) !=0)
        progerrno("Failed to seek to eof: ");

    /* Get pointer to hash table */
    f->start = sw_ftell(fp);

    /* Pack tmp */
    tmp = PACKFILEOFFSET(tmp);

    /* Write an empty hash table - Preserve space on disk */
    for(i = 0; i < FHASH_SIZE; i++)
        sw_fwrite((unsigned char *)&tmp,sizeof(tmp),1,fp);

    f->fp = fp;

    return f;
}


FHASH *FHASH_Open(FILE *fp, sw_off_t start)
{
FHASH *f;
SWUINT_T i;
sw_off_t tmp;

    f = (FHASH *) emalloc(sizeof(FHASH));
    f->start = start;
    f->fp = fp;

    /* put file pointer at start of hash table */
    sw_fseek(fp,start,SEEK_SET);

    /* Read hash table */
    for(i = 0; i < FHASH_SIZE ; i++)
    {
        sw_fread((unsigned char *)&tmp,sizeof(tmp), 1, fp);
        f->hash_offsets[i] = UNPACKFILEOFFSET(tmp);
    }
    return f;
}

sw_off_t FHASH_Close(FHASH *f)
{
sw_off_t start = f->start;
sw_off_t tmp;
FILE *fp = f->fp;
SWINT_T i;
    /* put file pointer at start of hash table */
    sw_fseek(fp,start,SEEK_SET);

    /* Read hash table */
    for(i = 0; i < FHASH_SIZE ; i++)
    {
        tmp = PACKFILEOFFSET(f->hash_offsets[i]);
        sw_fwrite((unsigned char *)&tmp,sizeof(tmp), 1, fp);
    }

    /* release memory */
    efree(f);

    /* Return offset to start table */
    return start;
}

SWINT_T FHASH_CompareKeys(unsigned char *key1, SWINT_T key_len1, unsigned char *key2, SWINT_T key_len2)
{
SWINT_T rc;

    if(key_len1 > key_len2)
        rc = memcmp(key1,key2,key_len2);
    else
        rc = memcmp(key1,key2,key_len1);

    if(!rc)
        rc = key_len1 - key_len2;

    return rc;
}


SWUINT_T FHASH_hash(unsigned char *s, SWINT_T len)
{
    SWUINT_T hashval;

    for (hashval = 0; len; s++,len--)
        hashval = (SWINT_T) ((unsigned char) *s) + 31 * hashval;
    return hashval % FHASH_SIZE;
}

SWINT_T FHASH_Insert(FHASH *f, unsigned char *key, SWINT_T key_len, unsigned char *data, SWINT_T data_len)
{
SWUINT_T hashval = FHASH_hash(key,key_len);
sw_off_t new,next;
FILE *fp = f->fp;

    sw_fseek(fp,(sw_off_t)0,SEEK_END);
    new = sw_ftell(fp);

    next = f->hash_offsets[hashval];
    next = PACKFILEOFFSET(next);

    sw_fwrite((unsigned char *)&next,sizeof(next), 1, fp);
    compress1(key_len,fp,fputc);
    sw_fwrite((unsigned char *)key, key_len, 1, fp);
    compress1(data_len,fp,fputc);
    sw_fwrite((unsigned char *)data, data_len, 1, fp);
    f->hash_offsets[hashval] = new;
    return 0;
}

SWINT_T FHASH_Search(FHASH *f, unsigned char *key, SWINT_T key_len, unsigned char *data, SWINT_T data_len)
{
    /* Calculate the has value for the key passed in and lookup the seek pointer */
    SWUINT_T    hashval = FHASH_hash(key,key_len);
    sw_off_t        next = f->hash_offsets[hashval];

    sw_off_t        tmp;

    FILE            *fp = f->fp;
    unsigned char   stack_buffer[2048], *read_key;
    SWINT_T             read_key_len;
    SWINT_T             read_data_len;
    SWINT_T             retval;

    while(next)
    {
        if ( 0 != sw_fseek(fp,next,SEEK_SET) )
            /* Will key be null terminated? */
            progerrno( "Failed to seek to offset %lld looking for key '%s' :", next, key );

        retval = sw_fread((unsigned char *)&tmp,1,sizeof(tmp),fp);
        if (feof(fp))
            progerrno( "eof() while Attempting to read '%lld' bytes from file hash: ", sizeof(tmp) );

        if ( sizeof(tmp) != retval )
            progerrno( "Only read '%lld' bytes but expected '%lld' while reading file hash: ", retval, sizeof(tmp) );



        next = UNPACKFILEOFFSET(tmp);

        if((read_key_len = uncompress1(fp,fgetc)) > sizeof(stack_buffer))
            read_key = emalloc(read_key_len);
        else
            read_key = stack_buffer;

        sw_fread((unsigned char *)read_key,read_key_len,1,fp);
        if(FHASH_CompareKeys(read_key, read_key_len, key, key_len) == 0)
        {
            read_data_len = uncompress1(fp,fgetc);
            if(read_data_len > data_len)
                 read_data_len = data_len;
            sw_fread((unsigned char *)data,read_data_len,1,fp);

            if(read_key != stack_buffer)
                efree(read_key);
            return read_data_len;
        }
        if(read_key != stack_buffer)
            efree(read_key);
    }
    memset(data,0,data_len);
    return 0;
}

SWINT_T FHASH_Update(FHASH *f, unsigned char *key, SWINT_T key_len, unsigned char *data, SWINT_T data_len)
{
SWUINT_T hashval = FHASH_hash(key,key_len);
sw_off_t next = f->hash_offsets[hashval];
FILE *fp = f->fp;
unsigned char stack_buffer[2048], *read_key;
SWINT_T read_key_len, read_data_len;
sw_off_t tmp;
    while(next)
    {
        sw_fseek(fp,next,SEEK_END);
        sw_fread((unsigned char *)&tmp,sizeof(tmp),1,fp);
        next = UNPACKFILEOFFSET(tmp);

        if((read_key_len = uncompress1(fp,fgetc)) > sizeof(stack_buffer))
            read_key = emalloc(read_key_len);
        else
            read_key = stack_buffer;

        sw_fread((unsigned char *)read_key,read_key_len,1,fp);
        if(FHASH_CompareKeys(read_key, read_key_len, key, key_len) == 0)
        {
            read_data_len = uncompress1(fp,fgetc);
            if(read_data_len > data_len)
                 read_data_len = data_len;
            sw_fwrite((unsigned char *)data,read_data_len,1,fp);

            if(read_key != stack_buffer)
                efree(read_key);
            return 0;
        }
        if(read_key != stack_buffer)
            efree(read_key);
    }
    return 1;
}

SWINT_T FHASH_Delete(FHASH *f, unsigned char *key, SWINT_T key_len)
{
SWUINT_T hashval = FHASH_hash(key,key_len);
sw_off_t next = f->hash_offsets[hashval];
sw_off_t prev = 0;
FILE *fp = f->fp;
unsigned char stack_buffer[2048], *read_key;
SWINT_T read_key_len;
sw_off_t tmp;
    while(next)
    {
        sw_fseek(fp,next,SEEK_END);
        sw_fread((unsigned char *)&tmp,sizeof(tmp),1,fp);

        if((read_key_len = uncompress1(fp,fgetc)) > sizeof(stack_buffer))
            read_key = emalloc(read_key_len);
        else
            read_key = stack_buffer;

        sw_fread((unsigned char *)read_key,read_key_len,1,fp);
        if(FHASH_CompareKeys(read_key, read_key_len, key, key_len) == 0)
        {
            next = UNPACKFILEOFFSET(tmp);
            if(!prev)
                f->hash_offsets[hashval] = next;
            else
            {
                sw_fseek(fp,prev,SEEK_SET);
                sw_fwrite((unsigned char *)&tmp,sizeof(tmp),1,fp);
            }
            if(read_key != stack_buffer)
                efree(read_key);
            return 0;
        }
        if(read_key != stack_buffer)
            efree(read_key);

        prev =next;
        next = UNPACKFILEOFFSET(tmp);
    }
    return 1;
}

