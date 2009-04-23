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
    
** Mon May  9 17:58:02 CDT 2005
** added GPL

**-----------------------------------------------------------------
**
**  Virtual Array Code. 
**  11/2001 jmruiz - The intention of this routines is storing and reading
**                   elemnts of arrays of SWINT_T numbers avoiding the 
**                   allocation in memory of the total array. In other words,
**                   if we need to read only 10 elements of the array, we
**                   will must try to make the minimal I/O memory and disk
**                   operations.
**
**                   To do that, the data is stored in aligned pages in disk   
**                   Also, a simple cache system is used to speed I/O file
**                   operations.
**
**                   The virtual array is extensible. In other words, you can
**                   add elements whenever you want
**
**    Main routines:
**
**  ARRAY *ARRAY_Create(FILE *fp)   
**    Creates a virtual array. Returns the handle of the array
**
**  ARRAY *ARRAY_Open(FILE *fp, sw_off_t root_page) 
**    Opens an existent Virtual Array. root_page is de value returned by
**    Array_Close. Returns de handle of the array.
**
**  sw_off_t ARRAY_Close(ARRAY *arr)
**    Closes and frees memory. arr is the value returned by ARRAY_Create or
**    ARRAY_Open. Returns the root page of the array. This value must be
**
**  SWINT_T ARRAY_Put(ARRAY *arr, SWINT_T index, SWUINT_T value)
**    Writes the array element arr[index]=value to the virtual array
**
**  SWUINT_T ARRAY_Get(ARRAY *arr, SWINT_T index)
**    Reads the array element index. Returns de value arr[index]
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "swish.h"
#include "mem.h"
#include "compress.h"
#include "array.h"
#include "error.h"


/* A ARRAY page size */
#define ARRAY_PageSize 4096

#define SizeOfElement sizeof(SWINT_T)

/* Round to ARRAY_PageSize */
#define ARRAY_RoundPageSize(n) (((sw_off_t)(n) + (sw_off_t)(ARRAY_PageSize - 1)) & (~(sw_off_t)(ARRAY_PageSize - 1)))

#define ARRAY_PageHeaderSize (1 * sizeof(sw_off_t)) 

#define ARRAY_PageData(pg) ((pg)->data + ARRAY_PageHeaderSize)
#define ARRAY_Data(pg,i) (ARRAY_PageData((pg)) + (i) * SizeOfElement)

#define ARRAY_SetNextPage(pg,num) (sw_off_t)( *(sw_off_t *)((pg)->data + 0 * sizeof(sw_off_t)) = PACKFILEOFFSET(num))

#define ARRAY_GetNextPage(pg,num) ( (num) = UNPACKFILEOFFSET(*(sw_off_t *)((pg)->data + 0 * sizeof(sw_off_t))))


SWINT_T ARRAY_WritePageToDisk(FILE *fp, ARRAY_Page *pg)
{
    ARRAY_SetNextPage(pg,pg->next);
    sw_fseek(fp,(sw_off_t)((sw_off_t)pg->page_number * (sw_off_t)ARRAY_PageSize),SEEK_SET);
    return sw_fwrite(pg->data,ARRAY_PageSize,1,fp);
}

SWINT_T ARRAY_WritePage(ARRAY *b, ARRAY_Page *pg)
{
SWINT_T hash = pg->page_number % ARRAY_CACHE_SIZE;
ARRAY_Page *tmp;
    pg->modified =1;
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

SWINT_T ARRAY_FlushCache(ARRAY *b)
{
SWINT_T i;
ARRAY_Page *tmp, *next;
    for(i = 0; i < ARRAY_CACHE_SIZE; i++)
    {
        if((tmp = b->cache[i]))
        {
            while(tmp)
            {
#ifdef DEBUG
                if(tmp->in_use)
                {
                    printf("DEBUG Error in FlushCache: Page in use\n");
                    exit(0);
                }
#endif
                next = tmp->next_cache;
                if(tmp->modified)
                {
                    ARRAY_WritePageToDisk(b->fp, tmp);
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

SWINT_T ARRAY_CleanCache(ARRAY *b)
{
SWINT_T i;
ARRAY_Page *tmp,*next;
    for(i = 0; i < ARRAY_CACHE_SIZE; i++)
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

ARRAY_Page *ARRAY_ReadPageFromDisk(FILE *fp, sw_off_t page_number)
{
ARRAY_Page *pg = (ARRAY_Page *)emalloc(sizeof(ARRAY_Page) + ARRAY_PageSize);

    sw_fseek(fp,(sw_off_t)(page_number * (sw_off_t)ARRAY_PageSize),SEEK_SET);
    sw_fread(pg->data,ARRAY_PageSize, 1, fp);

    ARRAY_GetNextPage(pg,pg->next);

    pg->page_number = page_number;
    pg->modified = 0;
    return pg;
}

ARRAY_Page *ARRAY_ReadPage(ARRAY *b, sw_off_t page_number)
{
SWINT_T hash = (SWINT_T)(page_number % (sw_off_t)ARRAY_CACHE_SIZE);
ARRAY_Page *tmp;
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

    tmp = ARRAY_ReadPageFromDisk(b->fp, page_number);
    tmp->modified = 0;
    tmp->in_use = 1;
    tmp->next_cache = b->cache[hash];
    b->cache[hash] = tmp;
    return tmp;
}

ARRAY_Page *ARRAY_NewPage(ARRAY *b)
{
ARRAY_Page *pg;
sw_off_t offset;
FILE *fp = b->fp;
SWINT_T hash;
SWINT_T size = ARRAY_PageSize;

    /* Get file pointer */
    if(sw_fseek(fp,(sw_off_t)0,SEEK_END) !=0)
        progerrno("Failed to seek to eof: ");

    offset = sw_ftell(fp);
    /* Round up file pointer */
    offset = ARRAY_RoundPageSize(offset);

    /* Set new file pointer - data will be aligned */
    if(sw_fseek(fp,offset, SEEK_SET)!=0 || offset != sw_ftell(fp))
        progerrno("Failed during seek: ");


    pg = (ARRAY_Page *)emalloc(sizeof(ARRAY_Page) + size);
    memset(pg,0,sizeof(ARRAY_Page) + size);
    /* Reserve space in file */
    if(sw_fwrite(pg->data,1,size,fp)!=size || ((sw_off_t)size + offset) != sw_ftell(fp))
        progerrno("Failed to write ARRAY_page: ");

    pg->next = 0;

    pg->page_number = offset / (sw_off_t)ARRAY_PageSize;

    /* add to cache */
    pg->modified = 1;
    pg->in_use = 1;
    hash = pg->page_number % ARRAY_CACHE_SIZE;
    pg->next_cache = b->cache[hash];
    b->cache[hash] = pg;
    return pg;
}

void ARRAY_FreePage(ARRAY *b, ARRAY_Page *pg)
{
SWINT_T hash = pg->page_number % ARRAY_CACHE_SIZE;
ARRAY_Page *tmp;

    tmp = b->cache[hash];

#ifdef DEBUG
    if(!(tmp = b->cache[hash]))
    {
        /* This should never happen!!!! */
        printf("Error in FreePage\n");
        exit(0);
    }
#endif

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

ARRAY *ARRAY_New(FILE *fp, SWUINT_T size)
{
ARRAY *b;
SWINT_T i;
    b = (ARRAY *) emalloc(sizeof(ARRAY));
    b->page_size = size;
    b->fp = fp;
    for(i = 0; i < ARRAY_CACHE_SIZE; i++)
        b->cache[i] = NULL;

    return b;
}

ARRAY *ARRAY_Create(FILE *fp)
{
ARRAY *b;
ARRAY_Page *root;
SWINT_T size = ARRAY_PageSize;

    b = ARRAY_New(fp , size);
    root = ARRAY_NewPage(b);

    b->root_page = root->page_number;

    ARRAY_WritePage(b, root);
    ARRAY_FreePage(b, root);

    return b;
}


ARRAY *ARRAY_Open(FILE *fp, sw_off_t root_page)
{
ARRAY *b;
SWINT_T size = ARRAY_PageSize;

    b = ARRAY_New(fp , size);

    b->root_page = root_page;

    return b;
}

sw_off_t ARRAY_Close(ARRAY *bt)
{
sw_off_t root_page = bt->root_page;
    ARRAY_FlushCache(bt);
    ARRAY_CleanCache(bt);
    efree(bt);
    return root_page;
}


SWINT_T ARRAY_Put(ARRAY *b, SWINT_T index, SWUINT_T value)
{
sw_off_t next_page; 
ARRAY_Page *root_page, *tmp = NULL, *prev; 
SWINT_T i, hash, page_reads, page_index;

    page_reads = index / ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeOfElement);
    hash = page_reads % ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeOfElement);
    page_reads /= ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeOfElement);
    page_index = index % ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeOfElement);

    root_page = ARRAY_ReadPage(b, b->root_page);
    next_page = UNPACKFILEOFFSET(*(sw_off_t *)ARRAY_Data(root_page, hash));

    prev = NULL;
    for(i = 0; i <= page_reads; i++)
    {
        if(!next_page)
        {
            tmp = ARRAY_NewPage(b);
            ARRAY_WritePage(b,tmp);
            if(!i)
            {
                *(sw_off_t *)ARRAY_Data(root_page,hash) = PACKFILEOFFSET(tmp->page_number);
                 ARRAY_WritePage(b,root_page);
            }
            else
            {
                prev->next = tmp->page_number;
                ARRAY_WritePage(b,prev);
            }
        } 
        else
        {
            tmp = ARRAY_ReadPage(b, next_page);
        }
        if(prev)
            ARRAY_FreePage(b,prev);
        prev = tmp;
        next_page = tmp->next;
    }
    *(SWUINT_T *)ARRAY_Data(tmp,page_index) = PACKLONG(value);
    ARRAY_WritePage(b,tmp);
    ARRAY_FreePage(b,tmp);
    ARRAY_FreePage(b,root_page);

    return 0;
}


SWUINT_T ARRAY_Get(ARRAY *b, SWINT_T index)
{
sw_off_t next_page;
SWUINT_T value; 
ARRAY_Page *root_page, *tmp;
SWINT_T i, hash, page_reads, page_index;

    page_reads = index / ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeOfElement);
    hash = page_reads % ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeOfElement);
    page_reads /= ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeOfElement);
    page_index = index % ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeOfElement);

    root_page = ARRAY_ReadPage(b, b->root_page);
/* $$$$ to be fixed $$$ */
    next_page = UNPACKLONG(*(SWINT_T *)ARRAY_Data(root_page, hash));

    tmp = NULL;
    for(i = 0; i <= page_reads; i++)
    {
        if(tmp)
            ARRAY_FreePage(b, tmp);
        if(!next_page)
        {
            ARRAY_FreePage(b,root_page);
            return 0L;
        } 
        else
        {
            tmp = ARRAY_ReadPage(b, next_page);
        }
        next_page = tmp->next;
    }
    value = UNPACKLONG(*(SWUINT_T *)ARRAY_Data(tmp,page_index));
    ARRAY_FreePage(b,tmp);
    ARRAY_FreePage(b,root_page);

    return value;
}



#ifdef DEBUG

#include <time.h>

#define N_TEST 50000000

#define F_READ_BINARY           "rb"
#define F_WRITE_BINARY          "wb"
#define F_READWRITE_BINARY      "rb+"

#define F_READ_TEXT             "r"
#define F_WRITE_TEXT            "w"
#define F_READWRITE_TEXT        "r+"

int main()
{
FILE *fp;
ARRAY *bt;
SWINT_T i;
static SWUINT_T nums[N_TEST];
SWUINT_T root_page;
    srand(time(NULL));



    fp = sw_fopen("kkkkk",F_WRITE_BINARY);
    sw_fclose(fp);
    fp = sw_fopen("kkkkk",F_READWRITE_BINARY);

    sw_fwrite("aaa",1,3,fp);

printf("\n\nIndexing\n\n");

    bt = ARRAY_Create(fp);
    for(i=0;i<N_TEST;i++)
    {
        nums[i] = rand();
//        nums[i]=i;
        ARRAY_Put(bt,i,nums[i]);
        if(nums[i]!= ARRAY_Get(bt,i))
            printf("\n\nmal %lld\n\n",i);
        if(!(i%1000))
        {
            ARRAY_FlushCache(bt);
            printf("%lld            \r",i);
        }
    }

    root_page = ARRAY_Close(bt);
    sw_fclose(fp);

printf("\n\nUnfreed %lld\n\n",num);
printf("\n\nSearching\n\n");

    fp = sw_fopen("kkkkk",F_READ_BINARY);
    bt = ARRAY_Open(fp, root_page);

    for(i=0;i<N_TEST;i++)
    {
        if(nums[i] != ARRAY_Get(bt,i))
            printf("\n\nmal %lld\n\n",i);
        if(!(i%1000))
            printf("%lld            \r",i);
    }

    root_page = ARRAY_Close(bt);

    sw_fclose(fp);
printf("\n\nUnfreed %lld\n\n",num);

}

#endif
