#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define STANDALONE
#define DEBUG


#ifdef STANDALONE

/* Rourtines to make long integers portable */
unsigned long PACKLONG(unsigned long num)
{
    unsigned long _i = 0L;
    unsigned char *_s;

    if (num)
    {
        _s = (unsigned char *) &_i;
        _s[0] = (unsigned char) ((num >> 24) & 0xFF);
        _s[1] = (unsigned char) ((num >> 16) & 0xFF);
        _s[2] = (unsigned char) ((num >> 8) & 0xFF);
        _s[3] = (unsigned char) (num & 0xFF);
        return _i;
    }
    return num;
}


unsigned long UNPACKLONG(unsigned long num)
{
    unsigned char *_s = (unsigned char *) &num;

    return (_s[0] << 24) + (_s[1] << 16) + (_s[2] << 8) + _s[3];
}


static int num = 0;
char *emalloc(unsigned int size)
{
num++;
return malloc(size);
}

void efree(void *p)
{
num--;
free(p);
}


#else

#include "mem.h"

unsigned long PACKLONG(unsigned long num);
unsigned long UNPACKLONG(unsigned long num);
unsigned char *compress3(int num, unsigned char *buffer);
int uncompress2(unsigned char **buffer);

#endif  /* STANDALONE */


#include "worddata.h"


/* WORDDATA page size */
/* !!!! Must no be less than 4096 and not greater than 65536*/
#define WORDDATA_PageSize 4096
/* WORDDATA Block size */
#define WORDDATA_BlockSize (WORDDATA_PageSize >> 8)

#define SizeInt32 4



/* Round to WORDDATA_PageSize */
#define WORDDATA_RoundPageSize(n) (((n) + WORDDATA_PageSize - 1) & (~(WORDDATA_PageSize - 1)))

/* Round a number to the upper BlockSize */
#define WORDDATA_RoundBlockSize(n) (((n) + WORDDATA_BlockSize - 1) & (~(WORDDATA_BlockSize - 1)))

/* Let's asign the first block for the header */
#define WORDDATA_PageHeaderSize (WORDDATA_BlockSize) 

/* Max data size to fit in a single page */
#define WORDDATA_MaxDataSize (WORDDATA_PageSize - WORDDATA_PageHeaderSize)

#define WORDDATA_PageData(pg) ((pg)->data + WORDDATA_PageHeaderSize)
#define WORDDATA_Data(pg,i) (WORDDATA_PageData((pg)) + (i) * SizeInt32)

#define WORDDATA_SetBlocksInUse(pg,num) ( *(int *)((pg)->data + 0 * SizeInt32) = PACKLONG(num))
#define WORDDATA_GetBlocksInUse(pg,num) ( (num) = UNPACKLONG(*(int *)((pg)->data + 0 * SizeInt32)))

#define WORDDATA_SetNumRecords(pg,num) ( *(int *)((pg)->data + 1 * SizeInt32) = PACKLONG(num))
#define WORDDATA_GetNumRecords(pg,num) ( (num) = UNPACKLONG(*(int *)((pg)->data + 1 * SizeInt32)))


int WORDDATA_WritePageToDisk(FILE *fp, WORDDATA_Page *pg)
{
    WORDDATA_SetBlocksInUse(pg,pg->used_blocks);
    WORDDATA_SetNumRecords(pg,pg->n);
    fseek(fp,pg->page_number * WORDDATA_PageSize,SEEK_SET);
    return fwrite(pg->data,WORDDATA_PageSize,1,fp);
}

int WORDDATA_WritePage(WORDDATA *b, WORDDATA_Page *pg)
{
int hash = pg->page_number % WORDDATA_CACHE_SIZE;
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
        }
    }
    return 0;
}

WORDDATA_Page *WORDDATA_ReadPageFromDisk(FILE *fp, unsigned long page_number)
{
WORDDATA_Page *pg = (WORDDATA_Page *)emalloc(sizeof(WORDDATA_Page) + WORDDATA_PageSize);

    fseek(fp,page_number * WORDDATA_PageSize,SEEK_SET);
    fread(pg->data,WORDDATA_PageSize, 1, fp);

    WORDDATA_GetBlocksInUse(pg,pg->used_blocks);
    WORDDATA_GetNumRecords(pg,pg->n);

    pg->page_number = page_number;
    pg->modified = 0;
    return pg;
}

WORDDATA_Page *WORDDATA_ReadPage(WORDDATA *b, unsigned long page_number)
{
int hash = page_number % WORDDATA_CACHE_SIZE;
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
long offset;
FILE *fp = b->fp;
int hash;
int size = WORDDATA_PageSize;

    /* Get file pointer */
    if(fseek(fp,0,SEEK_END) !=0)
    {
        printf("mal\n");
    }
    offset = ftell(fp);
    /* Round up file pointer */
    offset = WORDDATA_RoundPageSize(offset);

    /* Set new file pointer - data will be aligned */
    if(fseek(fp,offset, SEEK_SET)!=0 || offset != ftell(fp))
    {
        printf("mal\n");
    }

    pg = (WORDDATA_Page *)emalloc(sizeof(WORDDATA_Page) + size);
    memset(pg,0,sizeof(WORDDATA_Page) + size);
    /* Reserve space in file */
    if(fwrite(pg->data,1,size,fp)!=size || ((long)size + offset) != ftell(fp))
    {
        printf("mal\n");
    }

    pg->used_blocks = 0;
    pg->n = 0; /* Number of records */

    pg->page_number = offset/WORDDATA_PageSize;

    /* add to cache */
    pg->modified = 1;
    pg->in_use = 1;
    hash = pg->page_number % WORDDATA_CACHE_SIZE;
    pg->next_cache = b->cache[hash];
    b->cache[hash] = pg;
    return pg;
}

void WORDDATA_FreePage(WORDDATA *b, WORDDATA_Page *pg)
{
int hash = pg->page_number % WORDDATA_CACHE_SIZE;
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
    b->fp = fp;
    b->last_page = 0;
    for(i = 0; i < WORDDATA_CACHE_SIZE; i++)
        b->cache[i] = NULL;

    return b;
}


WORDDATA *WORDDATA_Open(FILE *fp)
{
WORDDATA *b;

    b = WORDDATA_New(fp);

    return b;
}

void WORDDATA_Close(WORDDATA *bt)
{
    WORDDATA_FlushCache(bt);
    WORDDATA_CleanCache(bt);
    efree(bt);
}


unsigned long WORDDATA_PutBig(WORDDATA *b, unsigned int len, unsigned char *data)
{
long offset;
unsigned long p_len = (unsigned long)PACKLONG((unsigned long)len);
FILE *fp = b->fp;
unsigned long id;

    /* Get file pointer */
    if(fseek(fp,0,SEEK_END) !=0)
    {
        printf("mal\n");
    }
    offset = ftell(fp);
    /* Round up file pointer */
    offset = WORDDATA_RoundPageSize(offset);

    /* Set new file pointer - data will be aligned */
    if(fseek(fp,offset, SEEK_SET)!=0 || offset != ftell(fp))
    {
        printf("mal\n");
    }

    id = ((unsigned long) (offset / WORDDATA_PageSize)) << 8;

    /* Write packed length */
    fwrite(&p_len,1,SizeInt32,fp);
    /* Write data */
    fwrite(data,1,len,fp);

    /* New offset */
    offset = ftell(fp);
    /* Round up file pointer */
    offset = WORDDATA_RoundPageSize(offset);
    /* Set new file pointer - data will be aligned */
    if(fseek(fp,offset, SEEK_SET)!=0 || offset != ftell(fp))
    {
        printf("mal\n");
    }

    return id;
}


unsigned long WORDDATA_Put(WORDDATA *b, unsigned int len, unsigned char *data)
{
int required_length;
int free_blocks;
int i, r_id, r_len, tmp;
int last_id, free_id;
unsigned char *p,*q;
unsigned char buffer[WORDDATA_PageSize];
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

    /* let's see if the data fits in page */
    if(b->last_page)
    {
        free_blocks = 255 - b->last_page->used_blocks;
        if(required_length > (free_blocks * WORDDATA_BlockSize))
        {
            WORDDATA_FreePage(b,b->last_page);
            b->last_page = WORDDATA_NewPage(b);
        }
    }
    else
    {
        b->last_page = WORDDATA_NewPage(b);
    }
    
    for(i = 0, free_id = 0, last_id = 0, p = WORDDATA_PageData(b->last_page); i < b->last_page->n; i++)
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
    memcpy(q,WORDDATA_PageData(b->last_page), p - WORDDATA_PageData(b->last_page));
    q += p - WORDDATA_PageData(b->last_page);
    q[0] = (unsigned char) free_id;
    q[1] = (unsigned char) len >> 8;
    q[2] = (unsigned char) (len & 0xff);
    memcpy(q+3,data,len);
    q += WORDDATA_RoundBlockSize((3 + len));
    for(;i < b->last_page->n; i++)
    {
        /* Get the record length */
        r_len = ((((int)(p[1])) << 8) + (int)(p[2]));
        tmp = WORDDATA_RoundBlockSize((3 + r_len));
        memcpy(q,p,tmp);
        p += tmp;
        q += tmp;
    }
    memcpy(WORDDATA_PageData(b->last_page),buffer,q - buffer);
    b->last_page->n++;
    b->last_page->used_blocks += required_length / WORDDATA_BlockSize;
    WORDDATA_WritePage(b,b->last_page);
    return (unsigned long)((b->last_page->page_number << 8) + free_id);
}

unsigned char *WORDDATA_GetBig(WORDDATA *b, unsigned long page_number, unsigned int *len)
{
long offset = (long) (page_number * WORDDATA_PageSize);
unsigned long p_len;
unsigned char *data;
    fseek(b->fp, offset, SEEK_SET);
    fread(&p_len,1,SizeInt32,b->fp);
    *len = UNPACKLONG(p_len);
    data = (unsigned char *)emalloc(*len);
    fread(data,1,*len,b->fp);
    return data;
}

unsigned char *WORDDATA_Get(WORDDATA *b, unsigned long global_id, unsigned int *len)
{
unsigned long page_number = global_id >> 8;
int id = (int)(global_id & 0xff);
int r_id,r_len;
int i;
unsigned char *p;
unsigned char *data;

    if(!id)
    {
        return WORDDATA_GetBig(b,page_number,len);
    }
    if(b->last_page)
        WORDDATA_FreePage(b,b->last_page);

    b->last_page = WORDDATA_ReadPage(b,page_number);

    for(i = 0, p = WORDDATA_PageData(b->last_page); i < b->last_page->n; i++)
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



    fp = fopen("kkkkk",FILEMODE_WRITE);
    fclose(fp);
    fp = fopen("kkkkk",FILEMODE_READWRITE);

    fwrite("aaa",1,3,fp);

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
    fclose(fp);

printf("\n\nUnfreed %d\n\n",num);
printf("\n\nSearching\n\n");

    fp = fopen("kkkkk",FILEMODE_READ);
    bt = WORDDATA_Open(fp);

    for(i=0;i<N_TEST;i++)
    {
        if(memcmp(WORDDATA_Get(bt,nums[i],&len),"1234567890123456",16)!=0)
            printf("\n\nmal %d\n\n",i);
        if(!(i%1000))
            printf("%d            \r",i);
    }

    WORDDATA_Close(bt);

    fclose(fp);
printf("\n\nUnfreed %d\n\n",num);

}

#endif
