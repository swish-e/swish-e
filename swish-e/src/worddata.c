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
#define WORDDATA_BlockSize (WORDDATA_PageSize >> 8)


/* Round to WORDDATA_PageSize */
#define WORDDATA_RoundPageSize(n) (sw_off_t)(((sw_off_t)(n) + (sw_off_t)(WORDDATA_PageSize - 1)) & (sw_off_t)(~(WORDDATA_PageSize - 1)))

/* Round a number to the upper BlockSize */
#define WORDDATA_RoundBlockSize(n) (((n) + WORDDATA_BlockSize - 1) & (~(WORDDATA_BlockSize - 1)))

/* Let's asign the first block for the header */
#define WORDDATA_PageHeaderSize (WORDDATA_BlockSize) 

/* Max data size to fit in a single page */
#define WORDDATA_MaxDataSize (WORDDATA_PageSize - WORDDATA_PageHeaderSize)

#define WORDDATA_PageData(pg) ((pg)->data + WORDDATA_PageHeaderSize)

#define WORDDATA_SetBlocksInUse(pg,num) ( *(int *)((pg)->data + 0 * sizeof(int)) = PACKLONG(num))
#define WORDDATA_GetBlocksInUse(pg,num) ( (num) = UNPACKLONG(*(int *)((pg)->data + 0 * sizeof(int))))

#define WORDDATA_SetNumRecords(pg,num) ( *(int *)((pg)->data + 1 * sizeof(int)) = PACKLONG(num))
#define WORDDATA_GetNumRecords(pg,num) ( (num) = UNPACKLONG(*(int *)((pg)->data + 1 * sizeof(int))))


int WORDDATA_WritePageToDisk(FILE *fp, WORDDATA_Page *pg)
{
    WORDDATA_SetBlocksInUse(pg,pg->used_blocks);
    WORDDATA_SetNumRecords(pg,pg->n);
    sw_fseek(fp,(sw_off_t)(pg->page_number * (sw_off_t)WORDDATA_PageSize),SEEK_SET);
    if ( sw_fwrite(pg->data,WORDDATA_PageSize,1,fp) != 1 )
        progerrno("Failed to write page to disk: "); 
    return 1;
}

int WORDDATA_WritePage(WORDDATA *b, WORDDATA_Page *pg)
{
int hash = (int)(pg->page_number % (sw_off_t)WORDDATA_CACHE_SIZE);
WORDDATA_Page *tmp;
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

    sw_fseek(fp,(sw_off_t)(page_number * (sw_off_t)WORDDATA_PageSize),SEEK_SET);
    sw_fread(pg->data,WORDDATA_PageSize, 1, fp);

    WORDDATA_GetBlocksInUse(pg,pg->used_blocks);
    WORDDATA_GetNumRecords(pg,pg->n);

    pg->page_number = page_number;
    pg->modified = 0;
    return pg;
}

WORDDATA_Page *WORDDATA_ReadPage(WORDDATA *b, sw_off_t page_number)
{
int hash = (int)(page_number % (sw_off_t)WORDDATA_CACHE_SIZE);
WORDDATA_Page *tmp;
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

    tmp = WORDDATA_ReadPageFromDisk(b->fp, page_number);
    tmp->modified = 0;
    tmp->in_use = 1;
    tmp->next_cache = b->cache[hash];
    b->cache[hash] = tmp;
    return tmp;
}


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
int i;
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
        free_blocks = 255 - b->last_del_page->used_blocks;
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
            free_blocks = 255 - b->last_put_page->used_blocks;
            if(required_length > (free_blocks * WORDDATA_BlockSize))
            {
                WORDDATA_FreePage(b,b->last_put_page);

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
        free_id = last_id + 1;

    q = buffer;
    memcpy(q,WORDDATA_PageData(last_page), p - WORDDATA_PageData(last_page));
    q += p - WORDDATA_PageData(last_page);
    q[0] = (unsigned char) free_id;
    q[1] = (unsigned char) (len >> 8);
    q[2] = (unsigned char) (len & 0xff);
    memcpy(q+3,data,len);
    q += WORDDATA_RoundBlockSize((3 + len));
    for(;i < last_page->n; i++)
    {
        /* Get the record length */
        r_len = ((((int)(p[1])) << 8) + (int)(p[2]));
        tmp = WORDDATA_RoundBlockSize((3 + r_len));
        memcpy(q,p,tmp);
        p += tmp;
        q += tmp;
    }
    memcpy(WORDDATA_PageData(last_page),buffer,q - buffer);
    last_page->n++;
    last_page->used_blocks += required_length / WORDDATA_BlockSize;
    WORDDATA_WritePage(b,last_page);
    return (b->lastid=(sw_off_t)((sw_off_t)(last_page->page_number << (sw_off_t)8) + (sw_off_t)free_id));
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
sw_off_t page_number = global_id >> (sw_off_t)8;
int id = (int)(global_id & (sw_off_t)0xff);
int r_id=-1,r_len=-1;
int i;
unsigned char *p;
unsigned char *data;

    if(!id)
    {
        return WORDDATA_GetBig(b,page_number,len);
    }
    if(b->last_get_page)
        WORDDATA_FreePage(b,b->last_get_page);

    b->last_get_page = WORDDATA_ReadPage(b,page_number);

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
