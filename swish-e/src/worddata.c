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
    
    
/********************************************************************************************
 * Here are some comments about how this works...
 * Note José Manuel Ruiz - April 27. 2005
 *
 *
 * The information is stored in pages. The pages are of fixed size. Its size is
 * defined by WORDDATA_PageSize. It must be good that this value is identical or
 * at least multiple of the I/O page size.
 *
 * If the size of worddata to be inserted do not fit in 1 page it is stored
 * en several contigous pages. This is the reason for 2 functions:
 *
 * WORDDATA_Put --> For worddata that fits in one page
 * WORDDATA_PutBig -> For worddata that does not fir in one page
 *
 * (There are equivalents for read and delete)
 *
 * For data that fits in 1 page...
 *
 * The data is stored in chunks of blocks. The size of the basic block is
 * defined by WORDDATA_BlockSize.
 *
 * The worddata is prefixed by 3 bytes:
 * - The first byte is an id from 1 to 0xff (so we can store up to 255
 *   worddatas in a page). This number is unique and identifies a worddata
 *   chunk
 *
 * - The bytes 2 and 3 are the size of worddata. To get the size apply: (p[1]
 *   << 8) + p[2]
 *
 * As you see, using a block size of 16 bytes, for a wordata of 14 bytes we
 * will need 32 bytes (2 blocks of 16 bytes): 1 byte for the id, 2 bytes for
 * the length (00 0E) and 14 bytes for the data (17 bytes rounded up to 32
 * bytes). So, in this case we are wasting 15 bytes. But it is the worst case.
 *
 ********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
#define DEBUG
*/

#include "swish.h"
#include "mem.h"
#include "compress.h"
#include "worddata.h"
#include "error.h"

/* WORDDATA page size */
/* !!!! Must no be less than 4096 and not greater than 65536*/
#define WORDDATA_PageSize 4096
/* WORDDATA Block size */
/* !!!! For a WORDDATA_PageSize of 4096, the Block size is 16 */
#define WORDDATA_BlockSize (WORDDATA_PageSize >> 8)
/* Using 1 byte for storing the block id, this leads to only 255 max blocks */
#define WORDDATA_Max_Blocks_Per_Page 0xff


/* Round to WORDDATA_PageSize */
#define WORDDATA_RoundPageSize(n) (sw_off_t)(((sw_off_t)(n) + (sw_off_t)(WORDDATA_PageSize - 1)) & (sw_off_t)(~(WORDDATA_PageSize - 1)))

/* Round a number to the upper BlockSize */
#define WORDDATA_RoundBlockSize(n) (((n) + WORDDATA_BlockSize - 1) & (~(WORDDATA_BlockSize - 1)))

/* Let's asign the first block for the header */
/* Check this if we put more data in the header !!!! */
#define WORDDATA_PageHeaderSize (WORDDATA_BlockSize) 

/* Max data size to fit in a single page */
#define WORDDATA_MaxDataSize (WORDDATA_PageSize - WORDDATA_PageHeaderSize)

#define WORDDATA_PageData(pg) ((pg)->data + WORDDATA_PageHeaderSize)

#define WORDDATA_SetBlocksInUse(pg,num) ( *(int *)((pg)->data + 0 * sizeof(int)) = PACKLONG(num))
#define WORDDATA_GetBlocksInUse(pg,num) ( (num) = UNPACKLONG(*(int *)((pg)->data + 0 * sizeof(int))))

#define WORDDATA_SetNumRecords(pg,num) ( *(int *)((pg)->data + 1 * sizeof(int)) = PACKLONG(num))
#define WORDDATA_GetNumRecords(pg,num) ( (num) = UNPACKLONG(*(int *)((pg)->data + 1 * sizeof(int))))


/* Routine to write the page to disk 
*/
int WORDDATA_WritePageToDisk(FILE *fp, WORDDATA_Page *pg)
{
    /* Set the page basic data*/
    /* Number of blocks in use */
    WORDDATA_SetBlocksInUse(pg,pg->used_blocks);
    /* Number of worddata entries in the page */
    WORDDATA_SetNumRecords(pg,pg->n);
    /* Seek to file pointer and write */
    sw_fseek(fp,(sw_off_t)(pg->page_number * (sw_off_t)WORDDATA_PageSize),SEEK_SET);
    if ( sw_fwrite(pg->data,WORDDATA_PageSize,1,fp) != 1 )
        progerrno("Failed to write page to disk: "); 
    return 1;
}

int WORDDATA_WritePage(WORDDATA *b, WORDDATA_Page *pg)
{
int hash = (int)(pg->page_number % (sw_off_t)WORDDATA_CACHE_SIZE);
WORDDATA_Page *tmp;
    /* Mark page as modified */
    pg->modified =1;
    /* If page is already in cache return. If not, add it to the cache */
    if((tmp = b->cache[hash]))
    {
        while(tmp)
        {
            if(tmp->page_number == pg->page_number)
            {
                return 0;
            }
            tmp = tmp->next_cache;
        }
    }
    pg->next_cache = b->cache[hash];
    b->cache[hash] = pg;
    return 0;
}

int WORDDATA_FlushCache(WORDDATA *b)
{
int i;
WORDDATA_Page *tmp, *next;
    for(i = 0; i < WORDDATA_CACHE_SIZE; i++)
    {
        if((tmp = b->cache[i]))
        {
            while(tmp)
            {
                next = tmp->next_cache;
                if(tmp->modified)
                {
                    WORDDATA_WritePageToDisk(b->fp, tmp);
                    tmp->modified = 0;
                }
                if(tmp != b->cache[i])
                    efree(tmp);

                tmp = next;
            }
            b->cache[i]->next_cache = NULL;
        }
    }
    return 0;
}

/* Routine to remove the page page_number from the cache */
void WORDDATA_CleanCachePage(WORDDATA *b,sw_off_t page_number)
{
int hash = (int)(page_number % (sw_off_t)WORDDATA_CACHE_SIZE);
WORDDATA_Page *tmp,*next,*prev = NULL;

    /* Search for page in cache */
    if((tmp = b->cache[hash]))
    {
        while(tmp)
        {
            if(tmp->page_number == page_number)
            {
                next = tmp->next_cache;
                efree(tmp);
                if(prev) 
                    prev->next_cache = next;
                else
                    b->cache[hash] = next;
                return;
            }
            prev = tmp;
            tmp = tmp->next_cache;
        }
    }
}

int WORDDATA_CleanCache(WORDDATA *b)
{
int i;
WORDDATA_Page *tmp,*next;
    for(i = 0; i < WORDDATA_CACHE_SIZE; i++)
    {
        if((tmp = b->cache[i]))
        {
            while(tmp)
            {
                next = tmp->next_cache;
                efree(tmp);
                tmp = next;
            }
            b->cache[i] = NULL;
        }
    }
    return 0;
}

WORDDATA_Page *WORDDATA_ReadPageFromDisk(FILE *fp, sw_off_t page_number)
{
WORDDATA_Page *pg = (WORDDATA_Page *)emalloc(sizeof(WORDDATA_Page) + WORDDATA_PageSize);

    if(sw_fseek(fp,(sw_off_t)(page_number * (sw_off_t)WORDDATA_PageSize),SEEK_SET)!=0 || ((sw_off_t)(page_number * (sw_off_t)WORDDATA_PageSize) != sw_ftell(fp)))
        progerrno("Failed to read page from disk: "); 

    sw_fread(pg->data,WORDDATA_PageSize, 1, fp);

    /* Load  basic data from page */
    /* Blocks in use */
    WORDDATA_GetBlocksInUse(pg,pg->used_blocks);
    /* Number of entries */
    WORDDATA_GetNumRecords(pg,pg->n);

    /* Page number */
    pg->page_number = page_number;
    /* Mark page as not modified */
    pg->modified = 0;
    return pg;
}

WORDDATA_Page *WORDDATA_ReadPage(WORDDATA *b, sw_off_t page_number)
{
int hash = (int)(page_number % (sw_off_t)WORDDATA_CACHE_SIZE);
WORDDATA_Page *tmp;

    /* Search for page in cache */
    if((tmp = b->cache[hash]))
    {
        while(tmp)
        {
            if(tmp->page_number == page_number)
            {
                return tmp;
            }
            tmp = tmp->next_cache;
        }
    }

    /* Not in cache. Read it from disk */
    tmp = WORDDATA_ReadPageFromDisk(b->fp, page_number);
    tmp->modified = 0;

    /* mark page as being used */
    tmp->in_use = 1;

    /* Add page to cache */
    tmp->next_cache = b->cache[hash];
    b->cache[hash] = tmp;
    return tmp;
}

/* Routine to get a new page */
WORDDATA_Page *WORDDATA_NewPage(WORDDATA *b)
{
WORDDATA_Page *pg;
sw_off_t offset;
FILE *fp = b->fp;
int hash;
int i;
sw_off_t page_number = (sw_off_t)0;
unsigned char empty_buffer[WORDDATA_PageSize];
    /* Let's see if we have a previous available page */
    if(b->num_Reusable_Pages)
    {
        /* First, look for a page of the same size */
        for(i = 0; i < b->num_Reusable_Pages ; i++)
        {
            if(WORDDATA_PageSize == b->Reusable_Pages[i].page_size)
                break;
        }
        /* If not found, let's try with a bigger one if exits */
        if(i == b->num_Reusable_Pages)
        {
            for(i = 0; i < b->num_Reusable_Pages ; i++)
            {
                if(WORDDATA_PageSize < b->Reusable_Pages[i].page_size)
                    break;
            }
        }
        /* If we got one page return it */
        if(i != b->num_Reusable_Pages)
        {
            page_number = b->Reusable_Pages[i].page_number;
            if(WORDDATA_PageSize == b->Reusable_Pages[i].page_size)
            {
                for(++i;i<b->num_Reusable_Pages;i++)
                {
                    /* remove page */
                    b->Reusable_Pages[i-1].page_number=b->Reusable_Pages[i].page_number;
                    b->Reusable_Pages[i-1].page_size=b->Reusable_Pages[i].page_size;
                }
                b->num_Reusable_Pages--;
            }
            else
            {
                b->Reusable_Pages[i].page_number ++;
                b->Reusable_Pages[i].page_size -= WORDDATA_PageSize;
            }
        }
    }
    /* If there is not any reusable page let's get it from disk */
    if(! page_number)
    {
        /* Get file pointer */
        if(sw_fseek(fp,(sw_off_t)0,SEEK_END) !=0)
            progerrno("Internal error seeking: "); 


        offset = sw_ftell(fp);
        /* Round up file pointer */
        offset = WORDDATA_RoundPageSize(offset);

        /* Set new file pointer - data will be aligned */
        if(sw_fseek(fp,offset, SEEK_SET)!=0 || offset != sw_ftell(fp))
            progerrno("Internal error seeking: "); 

        /* Reserve space in file */
        memset(empty_buffer,'0',WORDDATA_PageSize);

        if(sw_fwrite(empty_buffer,1,WORDDATA_PageSize,fp)!=WORDDATA_PageSize || ((sw_off_t)WORDDATA_PageSize + offset) != sw_ftell(fp))
            progerrno("Faild to write page data: ");

        page_number = offset / (sw_off_t)WORDDATA_PageSize;
    }

    pg = (WORDDATA_Page *)emalloc(sizeof(WORDDATA_Page) + WORDDATA_PageSize);
    memset(pg,0,sizeof(WORDDATA_Page) + WORDDATA_PageSize);
        /* Reserve space in file */

    pg->used_blocks = 0;
    pg->n = 0; /* Number of records */

    pg->page_number = page_number;

    /* add to cache */
    pg->modified = 1;
    pg->in_use = 1;
    hash = (int)(pg->page_number % (sw_off_t)WORDDATA_CACHE_SIZE);
    pg->next_cache = b->cache[hash];
    b->cache[hash] = pg;
    return pg;
}

void WORDDATA_FreePage(WORDDATA *b, WORDDATA_Page *pg)
{
int hash = (int)(pg->page_number % (sw_off_t)WORDDATA_CACHE_SIZE);

WORDDATA_Page *tmp;

    tmp = b->cache[hash];

    while(tmp)
    {
        if (tmp->page_number != pg->page_number)
            tmp = tmp->next_cache;
        else
        {
            tmp->in_use = 0;
            break;
        }
    }
}

WORDDATA *WORDDATA_New(FILE *fp)
{
WORDDATA *b;
    b = (WORDDATA *) emalloc(sizeof(WORDDATA));
    memset(b,0,sizeof(WORDDATA));
    b->fp = fp;
    return b;
}


WORDDATA *WORDDATA_Open(FILE *fp)
{
    return WORDDATA_New(fp);
}

void WORDDATA_Close(WORDDATA *bt)
{
    WORDDATA_FlushCache(bt);
    WORDDATA_CleanCache(bt);
    efree(bt);
}


sw_off_t WORDDATA_PutBig(WORDDATA *b, unsigned int len, unsigned char *data)
{
sw_off_t offset;
unsigned long p_len = (unsigned long)PACKLONG((unsigned long)len);
int size = WORDDATA_RoundPageSize(sizeof(p_len) + len);
FILE *fp = b->fp;
sw_off_t id;
sw_off_t page_number = (sw_off_t)0;
int i;
    /* Let's see if we have a previous available page */
    if(b->num_Reusable_Pages)
    {
        /* First, look for a page of the same size */
        for(i = 0; i < b->num_Reusable_Pages ; i++)
        {
            if(size == b->Reusable_Pages[i].page_size)
                break;
        }
        /* If not found, let's try with a bigger one if exits */
        if(i == b->num_Reusable_Pages)
        {
            for(i = 0; i < b->num_Reusable_Pages ; i++)
            {
                if(size < b->Reusable_Pages[i].page_size)
                    break;
            }
        }
        /* If we got one page return it */
        if(i != b->num_Reusable_Pages)
        {
            page_number = b->Reusable_Pages[i].page_number;
            if(size == b->Reusable_Pages[i].page_size)
            {
                for(++i;i<b->num_Reusable_Pages;i++)
                {
                    /* remove page */
                    b->Reusable_Pages[i-1].page_number=b->Reusable_Pages[i].page_number;
                    b->Reusable_Pages[i-1].page_size=b->Reusable_Pages[i].page_size;
                }
                b->num_Reusable_Pages--;
            }
            else
            {
                b->Reusable_Pages[i].page_number += size/WORDDATA_PageSize;
                b->Reusable_Pages[i].page_size -= size;
            }
        }
    }
    if(! page_number)
    {
        /* Get file pointer */
        if(sw_fseek(fp,(sw_off_t)0,SEEK_END) !=0)
            progerrno("Internal error seeking: "); 

        offset = sw_ftell(fp);
        /* Round up file pointer */
        offset = WORDDATA_RoundPageSize(offset);
    }
    else
    {
        offset = page_number * (sw_off_t)WORDDATA_PageSize;
    }
    /* Set new file pointer - data will be aligned */
    if(sw_fseek(fp,offset, SEEK_SET)!=0 || offset != sw_ftell(fp))
        progerrno("Internal error seeking: "); 

    id = (sw_off_t)(((offset / (sw_off_t)WORDDATA_PageSize)) << (sw_off_t)8);

    /* Write packed length */
    sw_fwrite(&p_len,1,sizeof(p_len),fp);
    /* Write data */
    sw_fwrite(data,1,len,fp);

    /* New offset */
    offset = sw_ftell(fp);
    /* Round up file pointer */
    offset = WORDDATA_RoundPageSize(offset);
    /* Set new file pointer - data will be aligned */
    if(sw_fseek(fp,offset, SEEK_SET)!=0 || offset != sw_ftell(fp))
        progerrno("Internal error seeking: "); 

    b->lastid = id;
    return id;
}


sw_off_t WORDDATA_Put(WORDDATA *b, unsigned int len, unsigned char *data)
{
int required_length;
int free_blocks;
int i, r_id, r_len, tmp;
int last_id, free_id;
unsigned char *p,*q;
unsigned char buffer[WORDDATA_PageSize];
WORDDATA_Page *last_page=NULL;
    /* Check if data fits in a single page */
    /* We need 1 byte for the id plus two bytes for the size */ 
    required_length = len + 1 + 2;
    /* Round it to the upper block size */
    required_length = WORDDATA_RoundBlockSize(required_length);
    if(required_length > WORDDATA_MaxDataSize)
    {
        /* Store long record in file */
        return WORDDATA_PutBig(b,len,data);
    }

    /* let's see if the data fits in the last page */
    /* First - Check for a page with a Del Operation */
    if(b->last_del_page)
    {
        free_blocks = WORDDATA_Max_Blocks_Per_Page - b->last_del_page->used_blocks;
        if(!(required_length > (free_blocks * WORDDATA_BlockSize)))
        {
            last_page = b->last_del_page;
        }
    }
    if(!last_page)
    {
        if( b->last_put_page)
        {
            /* Now check for the last page in a put operation */
            free_blocks = WORDDATA_Max_Blocks_Per_Page - b->last_put_page->used_blocks;
            if(required_length > (free_blocks * WORDDATA_BlockSize))
            {
                WORDDATA_FreePage(b,b->last_put_page);

                /* Save some memory - Do some flush of the data */
                if(!(b->page_counter % WORDDATA_CACHE_SIZE))
                {
                    WORDDATA_FlushCache(b);
                    WORDDATA_CleanCache(b);
                    b->page_counter = 0;
                    b->last_get_page = b->last_put_page = b->last_del_page =  0;
                }
                b->page_counter++;
                b->last_put_page = WORDDATA_NewPage(b);
            }
        }
        else
        {
            /* Save some memory - Do some flush flush of the data */
            if(!(b->page_counter % WORDDATA_CACHE_SIZE))
            {
                WORDDATA_FlushCache(b);
                WORDDATA_CleanCache(b);
                b->page_counter = 0;
                b->last_get_page = b->last_put_page = b->last_del_page =  0;
            }
            b->page_counter++;
            b->last_put_page = WORDDATA_NewPage(b);
        }
        last_page = b->last_put_page;
    }
    
    for(i = 0, free_id = 0, last_id = 0, p = WORDDATA_PageData(last_page); i < last_page->n; i++)
    {
        /* Get the record id */
        r_id = (int) (p[0]);
        /* Get the record length */
        r_len = ((((int)(p[1])) << 8) + (int)(p[2]));
        if((r_id - last_id) > 1)   /* find a reusable id */
        {
            free_id = last_id + 1;
            break;
        }
        last_id = r_id;
        p += WORDDATA_RoundBlockSize((3 + r_len));
    }
    if(!free_id)
        free_id = last_id + 1; /* The first block (0) is for the header */

    /* Let's use a temporal buffer and make the modifications in it */
    q = buffer;
    /* Init the buffer with the page content */
    /* p points to the start of the offset for the new worddata in the "real" page */
    memcpy(q,WORDDATA_PageData(last_page), p - WORDDATA_PageData(last_page));
    q += p - WORDDATA_PageData(last_page);
    /* Put id and size for worddata */
    q[0] = (unsigned char) free_id;
    q[1] = (unsigned char) (len >> 8);
    q[2] = (unsigned char) (len & 0xff);
    /* Put data */
    memcpy(q+3,data,len);
    /* Point to the next block */
    q += WORDDATA_RoundBlockSize((3 + len));
    /* Write worddata with ids greater than the new one after it */
    for(;i < last_page->n; i++)
    {
        /* Get the record length */
        r_len = ((((int)(p[1])) << 8) + (int)(p[2]));
        tmp = WORDDATA_RoundBlockSize((3 + r_len));
        memcpy(q,p,tmp);
        p += tmp;
        q += tmp;
    }
    /* Write the temp buffer into the page */
    memcpy(WORDDATA_PageData(last_page),buffer,q - buffer);
    last_page->n++;
    last_page->used_blocks += required_length / WORDDATA_BlockSize;
    WORDDATA_WritePage(b,last_page);

    /* Return the pointer to the data as page_number + id */
    /* The most significant byte is the id. The rest are for the page number */
    b->lastid=(sw_off_t)((sw_off_t)(last_page->page_number << (sw_off_t)8) + (sw_off_t)free_id);
    return(b->lastid);
}

unsigned char *WORDDATA_GetBig(WORDDATA *b, sw_off_t page_number, unsigned int *len)
{
sw_off_t offset = page_number * (sw_off_t)WORDDATA_PageSize;
unsigned long p_len;
unsigned char *data;
    sw_fseek(b->fp, offset, SEEK_SET);
    sw_fread(&p_len,1,sizeof(p_len),b->fp);
    *len = UNPACKLONG(p_len);
    data = (unsigned char *)emalloc(*len);
    sw_fread(data,1,*len,b->fp);
    return data;
}

unsigned char *WORDDATA_Get(WORDDATA *b, sw_off_t global_id, unsigned int *len)
{
/* Get the page number and id from the global_id */
sw_off_t page_number = global_id >> (sw_off_t)8;
int id = (int)(global_id & (sw_off_t)0xff);
int r_id=-1,r_len=-1;
int i;
unsigned char *p;
unsigned char *data;

    /* Special case. If id is null, the data did not fit in a normal page */
    /* So, go to get a big Worddata */
    if(!id)
    {
        return WORDDATA_GetBig(b,page_number,len);
    }
    /* reset last_get_page */
    if(b->last_get_page)
        WORDDATA_FreePage(b,b->last_get_page);

    b->last_get_page = WORDDATA_ReadPage(b,page_number);

    /* Search for the id in the page */
    for(i = 0, p = WORDDATA_PageData(b->last_get_page); i < b->last_get_page->n; i++)
    {
        /* Get the id */
        r_id = (int) (p[0]);
        /* Get the record length */
        r_len = ((((int)(p[1])) << 8) + (int)(p[2]));
        if(r_id == id)   /* find the id */
            break;
        p += WORDDATA_RoundBlockSize((3 + r_len));
    }

    /* If found read worddata */
    if(id == r_id)
    {
        data = (unsigned char *) emalloc(r_len);
        memcpy(data , p + 3 , r_len);
        *len = r_len;
    }
    else   /* Error */
    {
        data = NULL;
        *len = 0;
    }
    return data;
}

void WORDDATA_DelBig(WORDDATA *b, sw_off_t page_number, unsigned int *len)
{
sw_off_t offset = page_number * (sw_off_t)WORDDATA_PageSize;
unsigned long p_len;
    sw_fseek(b->fp, offset, SEEK_SET);
    sw_fread(&p_len,1,sizeof(p_len),b->fp);
    *len = UNPACKLONG(p_len) + sizeof(p_len);

    if(b->num_Reusable_Pages < WORDDATA_MAX_REUSABLE_PAGES)
    {
       b->Reusable_Pages[b->num_Reusable_Pages].page_number = page_number;
       b->Reusable_Pages[b->num_Reusable_Pages++].page_size = WORDDATA_RoundPageSize(*len);
    }
}

void WORDDATA_Del(WORDDATA *b, sw_off_t global_id, unsigned int *len)
{
sw_off_t page_number = global_id >> (sw_off_t)8;
int id = (int)(global_id & (sw_off_t)0xff);
int r_id=-1,r_len=-1,tmp;
int i;
unsigned char *p, *q;
int deleted_length;

    if(!id)
    {
        WORDDATA_DelBig(b,page_number,len);
        return;
    }
    if(b->last_del_page)
        WORDDATA_FreePage(b,b->last_del_page);

    b->last_del_page = WORDDATA_ReadPage(b,page_number);

    for(i = 0, p = WORDDATA_PageData(b->last_del_page); i < b->last_del_page->n; i++)
    {
        /* Get the id */
        r_id = (int) (p[0]);
        /* Get the record length */
        r_len = ((((int)(p[1])) << 8) + (int)(p[2]));
        if(r_id == id)   /* id found */
            break;
        p += WORDDATA_RoundBlockSize((3 + r_len));
    }

    if(id == r_id)
    {
        *len = r_len;
        deleted_length = WORDDATA_RoundBlockSize(r_len);
        /* Move rest of worddata to put them contigous (Remove the hole) */
        /* q points to the hole, p to the next record */
        q = p;
        p += WORDDATA_RoundBlockSize((3 + r_len));
        for(++i;i < b->last_del_page->n; i++)
        {
           /* Get the record length */
           r_len = ((((int)(p[1])) << 8) + (int)(p[2]));
           tmp = WORDDATA_RoundBlockSize((3 + r_len));
           memcpy(q,p,tmp);
           p += tmp;
           q += tmp;
        }
        b->last_del_page->n--;
        b->last_del_page->used_blocks -= deleted_length / WORDDATA_BlockSize;
        if(!b->last_del_page->n)
        {
            if(b->num_Reusable_Pages < WORDDATA_MAX_REUSABLE_PAGES)
            {
                b->Reusable_Pages[b->num_Reusable_Pages].page_number = page_number;
                b->Reusable_Pages[b->num_Reusable_Pages++].page_size = WORDDATA_PageSize;
            }
            /* If this page was also used in a put or get operation we must
            ** also resets it */
            if(b->last_get_page && b->last_get_page->page_number == b->last_del_page->page_number)
                 b->last_get_page = 0;
            if(b->last_put_page && b->last_put_page->page_number == b->last_del_page->page_number)
                 b->last_put_page = 0;
            /* Finally remove page from cache if exists */
            WORDDATA_CleanCachePage(b,b->last_del_page->page_number);
            /* And resets it */
            b->last_del_page = 0;
        }
        else
        {
            WORDDATA_WritePage(b,b->last_del_page);
        }
    }
    else   /* Error */
    {
        *len = 0;
    }

    return;
}


#ifdef DEBUG

#include <time.h>

#define N_TEST 5000

#ifdef _WIN32
#define FILEMODE_READ           "rb"
#define FILEMODE_WRITE          "wb"
#define FILEMODE_READWRITE      "rb+"
#elif defined(__VMS)
#define FILEMODE_READ           "rb"
#define FILEMODE_WRITE          "wb"
#define FILEMODE_READWRITE      "rb+"
#else
#define FILEMODE_READ           "r"
#define FILEMODE_WRITE          "w"
#define FILEMODE_READWRITE      "r+"
#endif

int main()
{
FILE *fp;
WORDDATA *bt;
int i,len;
static unsigned long nums[N_TEST];
    srand(time(NULL));



    fp = sw_fopen("kkkkk",FILEMODE_WRITE);
    sw_fclose(fp);
    fp = sw_fopen("kkkkk",FILEMODE_READWRITE);

    sw_fwrite("aaa",1,3,fp);

printf("\n\nIndexing\n\n");

    bt = WORDDATA_Open(fp);
    for(i=0;i<N_TEST;i++)
    {
        nums[i] = WORDDATA_Put(bt,16,"1234567890123456");
        if(memcmp(WORDDATA_Get(bt,nums[i],&len),"1234567890123456",16)!=0)
            printf("\n\nmal %d\n\n",i);
        if(!(i%1000))
        {
            WORDDATA_FlushCache(bt);
            printf("%d            \r",i);
        }
    }

    WORDDATA_Close(bt);
    sw_fclose(fp);

printf("\n\nUnfreed %d\n\n",num);
printf("\n\nSearching\n\n");

    fp = sw_fopen("kkkkk",FILEMODE_READ);
    bt = WORDDATA_Open(fp);

    for(i=0;i<N_TEST;i++)
    {
        if(memcmp(WORDDATA_Get(bt,nums[i],&len),"1234567890123456",16)!=0)
            printf("\n\nmal %d\n\n",i);
        if(!(i%1000))
            printf("%d            \r",i);
    }

    WORDDATA_Close(bt);

    sw_fclose(fp);
printf("\n\nUnfreed %d\n\n",num);

}

#endif
