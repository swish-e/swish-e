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
**       int FHASH_Insert(FHASH *f, unsigned char *key, int key_len, unsigned char *data, int data_len);
**           Adds a new entry pair (key,data) to the hash db file. Length of 
**               key is key_len, length of data is data_len
**
**       int FHASH_Search(FHASH *f, unsigned char *key, int key_len, unsigned char *data, int data_len);
**           Searchs and returns the data for a given key of length key_len
**           The data is returned in the data array that must be allocated by
**               the caller. If data buffer is not long enough, data will
**               be truncated
**           Returns the copied length in data
**
**       int FHASH_Update(FHASH *f, unsigned char *key, int key_len, unsigned char *data, int data_len);
**           Updates data for a given key of length key_len
**           Data must be of the same size of the original record
**           Returns 0 (OK) or 1 (no OK)
**
**       int FHASH_Delete(FHASH *f, unsigned char *key, int key_len);
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

FHASH *FHASH_Create(FILE *fp)
{
FHASH *f;
unsigned int i;
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
unsigned int i;
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
int i;
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

int FHASH_CompareKeys(unsigned char *key1, int key_len1, unsigned char *key2, int key_len2)
{
int rc;

    if(key_len1 > key_len2)
        rc = memcmp(key1,key2,key_len2);
    else
        rc = memcmp(key1,key2,key_len1);

    if(!rc)
        rc = key_len1 - key_len2;

    return rc;
}


unsigned int FHASH_hash(unsigned char *s, int len)
{
    unsigned int hashval;

    for (hashval = 0; len; s++,len--)
        hashval = (int) ((unsigned char) *s) + 31 * hashval;
    return hashval % FHASH_SIZE;
}

int FHASH_Insert(FHASH *f, unsigned char *key, int key_len, unsigned char *data, int data_len)
{
unsigned int hashval = FHASH_hash(key,key_len);
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

int FHASH_Search(FHASH *f, unsigned char *key, int key_len, unsigned char *data, int data_len)
{
unsigned int hashval = FHASH_hash(key,key_len);
sw_off_t next = f->hash_offsets[hashval];
FILE *fp = f->fp;
unsigned char stack_buffer[2048], *read_key;
int read_key_len, read_data_len;
sw_off_t tmp;
    while(next)
    {
        sw_fseek(fp,next,SEEK_SET);
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

int FHASH_Update(FHASH *f, unsigned char *key, int key_len, unsigned char *data, int data_len)
{
unsigned int hashval = FHASH_hash(key,key_len);
sw_off_t next = f->hash_offsets[hashval];
FILE *fp = f->fp;
unsigned char stack_buffer[2048], *read_key;
int read_key_len, read_data_len;
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

int FHASH_Delete(FHASH *f, unsigned char *key, int key_len)
{
unsigned int hashval = FHASH_hash(key,key_len);
sw_off_t next = f->hash_offsets[hashval];
sw_off_t prev = 0;
FILE *fp = f->fp;
unsigned char stack_buffer[2048], *read_key;
int read_key_len;
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

