/*
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
**-----------------------------------------------------------------
**
**  Virtual Array Code. 
**  11/2001 jmruiz - The intention of this routines is storing and reading
**                   elemnts of arrays of long numbers avoiding the 
**                   allocation in memory of the total array. In other words,
**                   if we need to read only 10 elements of the array, we
**                   will must try to  make the minimal I/O memory and disk
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
**  ARRAY *ARRAY_Open(FILE *fp, unsigned long root_page) 
**    Opens an existent Virtual Array. root_page is de value returned by
**    Array_Close. Returns de handle of the array.
**
**  unsigned long ARRAY_Close(ARRAY *arr)
**    Closes and frees memory. arr is the value returned by ARRAY_Create or
**    ARRAY_Open. Returns the root page of the array. This value must be
**
**  int ARRAY_Put(ARRAY *arr, int index, unsigned long value)
**    Writes the array element arr[index]=value to the virtual array
**
**  unsigned long ARRAY_Get(ARRAY *arr, int index)
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


/* A ARRAY page size */
#define ARRAY_PageSize 4096

#define SizeInt32 4

/* Round to ARRAY_PageSize */
#define ARRAY_RoundPageSize(n) (((sw_off_t)(n) + (sw_off_t)(ARRAY_PageSize - 1)) & (sw_off_t)(~(ARRAY_PageSize - 1)))

#define ARRAY_PageHeaderSize (1 * SizeInt32) 

#define ARRAY_PageData(pg) ((pg)->data + ARRAY_PageHeaderSize)
#define ARRAY_Data(pg,i) (ARRAY_PageData((pg)) + (i) * SizeInt32)

#define ARRAY_SetNextPage(pg,num) ( *(int *)((pg)->data + 0 * SizeInt32) = PACKLONG(num))

#define ARRAY_GetNextPage(pg,num) ( (num) = UNPACKLONG(*(int *)((pg)->data + 0 * SizeInt32)))


int ARRAY_WritePageToDisk(FILE *fp, ARRAY_Page *pg)
{
    ARRAY_SetNextPage(pg,pg->next);
    sw_fseek(fp,(sw_off_t)((sw_off_t)pg->page_number * (sw_off_t)ARRAY_PageSize),SEEK_SET);
    return sw_fwrite(pg->data,ARRAY_PageSize,1,fp);
}

int ARRAY_WritePage(ARRAY *b, ARRAY_Page *pg)
{
int hash = pg->page_number % ARRAY_CACHE_SIZE;
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

int ARRAY_FlushCache(ARRAY *b)
{
int i;
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

int ARRAY_CleanCache(ARRAY *b)
{
int i;
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

ARRAY_Page *ARRAY_ReadPageFromDisk(FILE *fp, unsigned long page_number)
{
ARRAY_Page *pg = (ARRAY_Page *)emalloc(sizeof(ARRAY_Page) + ARRAY_PageSize);

    sw_fseek(fp,(sw_off_t)((sw_off_t)page_number * (sw_off_t)ARRAY_PageSize),SEEK_SET);
    sw_fread(pg->data,ARRAY_PageSize, 1, fp);

    ARRAY_GetNextPage(pg,pg->next);

    pg->page_number = page_number;
    pg->modified = 0;
    return pg;
}

ARRAY_Page *ARRAY_ReadPage(ARRAY *b, unsigned long page_number)
{
int hash = page_number % ARRAY_CACHE_SIZE;
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
int hash;
int size = ARRAY_PageSize;

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

    pg->page_number = (unsigned long)(offset/(sw_off_t)ARRAY_PageSize);

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
int hash = pg->page_number % ARRAY_CACHE_SIZE;
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

ARRAY *ARRAY_New(FILE *fp, unsigned int size)
{
ARRAY *b;
int i;
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
int size = ARRAY_PageSize;

    b = ARRAY_New(fp , size);
    root = ARRAY_NewPage(b);

    b->root_page = root->page_number;

    ARRAY_WritePage(b, root);
    ARRAY_FreePage(b, root);

    return b;
}


ARRAY *ARRAY_Open(FILE *fp, unsigned long root_page)
{
ARRAY *b;
int size = ARRAY_PageSize;

    b = ARRAY_New(fp , size);

    b->root_page = root_page;

    return b;
}

unsigned long ARRAY_Close(ARRAY *bt)
{
unsigned long root_page = bt->root_page;
    ARRAY_FlushCache(bt);
    ARRAY_CleanCache(bt);
    efree(bt);
    return root_page;
}


int ARRAY_Put(ARRAY *b, int index, unsigned long value)
{
unsigned long next_page; 
ARRAY_Page *root_page, *tmp = NULL, *prev; 
int i, hash, page_reads, page_index;

    page_reads = index / ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeInt32);
    hash = page_reads % ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeInt32);
    page_reads /= ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeInt32);
    page_index = index % ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeInt32);

    root_page = ARRAY_ReadPage(b, b->root_page);
    next_page = UNPACKLONG(*(unsigned long *)ARRAY_Data(root_page, hash));

    prev = NULL;
    for(i = 0; i <= page_reads; i++)
    {
        if(!next_page)
        {
            tmp = ARRAY_NewPage(b);
            ARRAY_WritePage(b,tmp);
            if(!i)
            {
                *(unsigned long *)ARRAY_Data(root_page,hash) = PACKLONG(tmp->page_number);
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
    *(unsigned long *)ARRAY_Data(tmp,page_index) = PACKLONG(value);
    ARRAY_WritePage(b,tmp);
    ARRAY_FreePage(b,tmp);
    ARRAY_FreePage(b,root_page);

    return 0;
}


unsigned long ARRAY_Get(ARRAY *b, int index)
{
unsigned long next_page, value; 
ARRAY_Page *root_page, *tmp;
int i, hash, page_reads, page_index;

    page_reads = index / ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeInt32);
    hash = page_reads % ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeInt32);
    page_reads /= ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeInt32);
    page_index = index % ((ARRAY_PageSize - ARRAY_PageHeaderSize) / SizeInt32);

    root_page = ARRAY_ReadPage(b, b->root_page);
    next_page = UNPACKLONG(*(unsigned long *)ARRAY_Data(root_page, hash));

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
    value = UNPACKLONG(*(unsigned long *)ARRAY_Data(tmp,page_index));
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
int i;
static unsigned long nums[N_TEST];
unsigned long root_page;
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
            printf("\n\nmal %d\n\n",i);
        if(!(i%1000))
        {
            ARRAY_FlushCache(bt);
            printf("%d            \r",i);
        }
    }

    root_page = ARRAY_Close(bt);
    sw_fclose(fp);

printf("\n\nUnfreed %d\n\n",num);
printf("\n\nSearching\n\n");

    fp = sw_fopen("kkkkk",F_READ_BINARY);
    bt = ARRAY_Open(fp, root_page);

    for(i=0;i<N_TEST;i++)
    {
        if(nums[i] != ARRAY_Get(bt,i))
            printf("\n\nmal %d\n\n",i);
        if(!(i%1000))
            printf("%d            \r",i);
    }

    root_page = ARRAY_Close(bt);

    sw_fclose(fp);
printf("\n\nUnfreed %d\n\n",num);

}

#endif
