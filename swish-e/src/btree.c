#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "swish.h"
#include "swstring.h"
#include "compress.h"
#include "mem.h"
#include "btree.h"


/* A BTREE page cannot be smaller than BTREE_MinPageSize */
#define BTREE_MinPageSize 4096
/* BTREE max key length*/
#define BTREE_MaxKeySize BTREE_MinPageSize>>2

/* A BTREE page can be greater than BTREE_MaxPageSize */
#define BTREE_MaxPageSize 65536

#define SizeInt32 4
#define SizeInt16 2


/* Round in BTREE_MinPageSize */
#define BTREE_RoundPageSize(n) (((n) + BTREE_MinPageSize - 1) & (~(BTREE_MinPageSize - 1)))

#define BTREE_PageHeaderSize (6 * SizeInt32) 

#define BTREE_PageData(pg) ((pg)->data + BTREE_PageHeaderSize)
#define BTREE_EndData(pg) ((pg)->data + (pg)->data_end)
#define BTREE_KeyIndexOffset(data,i) (data + BTREE_PageHeaderSize + (i) * SizeInt16)
#define BTREE_KeyDataOffset(pg,i) ((*(BTREE_KeyIndexOffset((pg->data),(i))) <<8) + *(BTREE_KeyIndexOffset((pg->data),(i)) + 1))
#define BTREE_KeyData(pg,i) ((pg)->data + BTREE_KeyDataOffset((pg),(i)))

#define BTREE_SetNextPage(pg,num) ( *(int *)((pg)->data + 0 * SizeInt32) = PACKLONG(num))
#define BTREE_SetPrevPage(pg,num) ( *(int *)((pg)->data + 1 * SizeInt32) = PACKLONG(num))
#define BTREE_SetSize(pg,num)     ( *(int *)((pg)->data + 2 * SizeInt32) = PACKLONG(num))
#define BTREE_SetNumKeys(pg,num)  ( *(int *)((pg)->data + 3 * SizeInt32) = PACKLONG(num))
#define BTREE_SetFlags(pg,num)    ( *(int *)((pg)->data + 4 * SizeInt32) = PACKLONG(num))
#define BTREE_SetDataEnd(pg,num)  ( *(int *)((pg)->data + 5 * SizeInt32) = PACKLONG(num))

#define BTREE_GetNextPage(pg,num) ( (num) = UNPACKLONG(*(int *)((pg)->data + 0 * SizeInt32)))
#define BTREE_GetPrevPage(pg,num) ( (num) = UNPACKLONG(*(int *)((pg)->data + 1 * SizeInt32)))
#define BTREE_GetSize(pg,num)     ( (num) = UNPACKLONG(*(int *)((pg)->data + 2 * SizeInt32)))
#define BTREE_GetNumKeys(pg,num)  ( (num) = UNPACKLONG(*(int *)((pg)->data + 3 * SizeInt32)))
#define BTREE_GetFlags(pg,num)    ( (num) = UNPACKLONG(*(int *)((pg)->data + 4 * SizeInt32)))
#define BTREE_GetDataEnd(pg,num)  ( (num) = UNPACKLONG(*(int *)((pg)->data + 5 * SizeInt32)))

/* Flags */
#define BTREE_ROOT_NODE 0x1
#define BTREE_LEAF_NODE 0x2


int BTREE_WritePageToDisk(FILE *fp, BTREE_Page *pg)
{
    BTREE_SetNextPage(pg,pg->next);
    BTREE_SetPrevPage(pg,pg->prev);
    BTREE_SetSize(pg,pg->size);
    BTREE_SetNumKeys(pg,pg->n);
    BTREE_SetFlags(pg,pg->flags);
    BTREE_SetDataEnd(pg,pg->data_end);
    fseek(fp,pg->page_number * BTREE_MinPageSize,SEEK_SET);
    return fwrite(pg->data,pg->size,1,fp);
}

int BTREE_WritePage(BTREE *b, BTREE_Page *pg)
{
int hash = pg->page_number % BTREE_CACHE_SIZE;
BTREE_Page *tmp;
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

int BTREE_FlushCache(BTREE *b)
{
int i;
BTREE_Page *tmp, *next;
    for(i = 0; i < BTREE_CACHE_SIZE; i++)
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
                    BTREE_WritePageToDisk(b->fp, tmp);
                    tmp->modified = 0;
                }
                if(tmp != b->cache[i])
                {
                    efree(tmp);
                }
                tmp = next;
            }
            b->cache[i]->next_cache = NULL;
        }
    }
    return 0;
}

int BTREE_CleanCache(BTREE *b)
{
int i;
BTREE_Page *tmp,*next;
    for(i = 0; i < BTREE_CACHE_SIZE; i++)
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

BTREE_Page *BTREE_ReadPageFromDisk(FILE *fp, unsigned long page_number)
{
BTREE_Page *pg = (BTREE_Page *)emalloc(sizeof(BTREE_Page) + BTREE_MinPageSize);

    fseek(fp,page_number * BTREE_MinPageSize,SEEK_SET);
    fread(pg->data,BTREE_MinPageSize, 1, fp);

    BTREE_GetNextPage(pg,pg->next);
    BTREE_GetPrevPage(pg,pg->prev);
    BTREE_GetSize(pg,pg->size);
    BTREE_GetNumKeys(pg,pg->n);
    BTREE_GetFlags(pg,pg->flags);
    BTREE_GetDataEnd(pg,pg->data_end);

    pg->page_number = page_number;
    pg->modified = 0;
    return pg;
}

BTREE_Page *BTREE_ReadPage(BTREE *b, unsigned long page_number)
{
int hash = page_number % BTREE_CACHE_SIZE;
BTREE_Page *tmp;
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

    tmp = BTREE_ReadPageFromDisk(b->fp, page_number);
    tmp->modified = 0;
    tmp->in_use = 1;
    tmp->next_cache = b->cache[hash];
    b->cache[hash] = tmp;
    return tmp;
}

BTREE_Page *BTREE_NewPage(BTREE *b, unsigned int size, unsigned int flags)
{
BTREE_Page *pg;
long offset;
FILE *fp = b->fp;
int hash;
    /* Round up size */
    size = BTREE_RoundPageSize(size);

    if(size > BTREE_MaxPageSize)
        return NULL;

    /* Get file pointer */
    if(fseek(fp,0,SEEK_END) !=0)
    {
        printf("mal\n");
    }
    offset = ftell(fp);
    /* Round up file pointer */
    offset = BTREE_RoundPageSize(offset);

    /* Set new file pointer - data will be aligned */
    if(fseek(fp,offset, SEEK_SET)!=0 || offset != ftell(fp))
    {
        printf("mal\n");
    }

    pg = (BTREE_Page *)emalloc(sizeof(BTREE_Page) + size);
    memset(pg,0,sizeof(BTREE_Page) + size);
    /* Reserve space in file */
    if(fwrite(pg->data,1,size,fp)!=size || ((long)size + offset) != ftell(fp))
    {
        printf("mal\n");
    }

    pg->next = 0;
    pg->prev = 0;
    pg->size = size;
    pg->flags = flags;
    pg->data_end = BTREE_PageHeaderSize;
    pg->n = 0;

    pg->page_number = offset/BTREE_MinPageSize;

    /* add to cache */
    pg->modified = 1;
    pg->in_use = 1;
    hash = pg->page_number % BTREE_CACHE_SIZE;
    pg->next_cache = b->cache[hash];
    b->cache[hash] = pg;
    return pg;
}

void BTREE_FreePage(BTREE *b, BTREE_Page *pg)
{
int hash = pg->page_number % BTREE_CACHE_SIZE;
BTREE_Page *tmp;

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

int BTREE_CompareKeys(unsigned char *key1, int key_len1, unsigned char *key2, int key_len2)
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


int BTREE_GetPositionForKey(BTREE_Page *pg, unsigned char *key, int key_len, int *comp)
{
int i,j,k,isbigger=-1;
int key_len_k;
unsigned char *key_k;
    /* Use binary search for adding key */
    /* Look for the position to insert using a binary search */
    i = pg->n - 1;
    j = k = 0;
    while (i >= j)
    {
        k = j + (i - j) / 2;
        key_k = BTREE_KeyData(pg,k);
        key_len_k = uncompress2(&key_k);
        isbigger = BTREE_CompareKeys(key,key_len,key_k,key_len_k);
        if (!isbigger)
            break;
        else if (isbigger > 0)
            j = k + 1;
        else
            i = k - 1;
    }

    *comp = isbigger;

    return k;
}

unsigned int BTREE_GetKeyFromPage(BTREE *b, BTREE_Page *pg, unsigned char *key, int key_len, unsigned char **found, int *found_len)
{
int k,comp = 0;

    k = BTREE_GetPositionForKey(pg, key, key_len, &comp);

    if(comp > 0 && k == (int) pg->n && k)
        k--;

    if(comp < 0 && k) 
        k--;
    
    b->current_page = pg->page_number;
    b->current_position = k;

    *found = BTREE_KeyData(pg,k);
    *found_len = uncompress2(found);
    return UNPACKLONG(*(unsigned long *) (*found + *found_len));
}

int BTREE_AddKeyToPage(BTREE_Page *pg, int position, unsigned char *key, int key_len, unsigned long data_pointer)
{
unsigned char buffer[BTREE_MaxPageSize];
int j,k;
unsigned char *p;
unsigned char *new_key_start, *new_key_end;
int new_entry_len , tmp;

    data_pointer = PACKLONG(data_pointer);

    k = position;

    /* Write key */
    /* Write from Page header upto the key being inserted */
    p = buffer;
    memcpy(p,pg->data,BTREE_KeyIndexOffset(pg->data,k) - pg->data);
    p += BTREE_KeyIndexOffset(pg->data,k) - pg->data;

    p += (pg->n - k + 1) * SizeInt16;

    if(k == (int)pg->n)
    {
        if(k)
        {
            memcpy(p,BTREE_KeyData(pg,0),BTREE_EndData(pg) - BTREE_KeyData(pg,0));
            p += BTREE_EndData(pg) - BTREE_KeyData(pg,0);
        }
    }
    else
    {
        if(k)
        {
            memcpy(p,BTREE_KeyData(pg,0),BTREE_KeyData(pg,k) - BTREE_KeyData(pg,0));
            p += BTREE_KeyData(pg,k) - BTREE_KeyData(pg,0);
        }
    }

    new_key_start = p;
    p = compress3(key_len, p);
    memcpy(p , key, key_len);
    p += key_len;
    memcpy(p, &data_pointer, SizeInt32);
    p += SizeInt32;
    new_key_end = p;
    new_entry_len = new_key_end - new_key_start;

    if(k < (int) pg->n)
    {
        memcpy(p,BTREE_KeyData(pg,k), BTREE_EndData(pg) - BTREE_KeyData(pg,k));
        p += BTREE_EndData(pg) - BTREE_KeyData(pg,k);
    }

    for(j = 0; j < k; j++)
    {
        tmp = BTREE_KeyDataOffset(pg,j) + SizeInt16;
        *(BTREE_KeyIndexOffset(buffer,j)) =  (unsigned char)((tmp & 0xff00) >>8);
        *(BTREE_KeyIndexOffset(buffer,j) + 1) = (unsigned char) (tmp & 0xff);
    }

    for(j = (int)(pg->n - 1) ; j >=k; j--)
    {
        tmp = BTREE_KeyDataOffset(pg,j) + new_entry_len + SizeInt16;
        *(BTREE_KeyIndexOffset(buffer,j + 1)) =  (unsigned char)((tmp & 0xff00) >>8);
        *(BTREE_KeyIndexOffset(buffer,j + 1) + 1) = (unsigned char) (tmp & 0xff);
    }

    tmp = new_key_start - buffer;
    *(BTREE_KeyIndexOffset(buffer,k)) =  (unsigned char)((tmp & 0xff00) >>8);
    *(BTREE_KeyIndexOffset(buffer,k) + 1) = (unsigned char) (tmp & 0xff);

    memcpy(pg->data, buffer, p - buffer);
    pg->n++;
    pg->data_end = p - buffer;

    return k;
}

int BTREE_DelKeyInPage(BTREE_Page *pg, int pos)
{
unsigned char buffer[BTREE_MaxPageSize];
unsigned char *p, *q;
unsigned char *del_key_start, *del_key_end;
int del_entry_len , tmp;
int j, k = pos;



    /* Write key */
    /* Write from Page header upto the key being deleted */
    p = buffer;

    memcpy(p,pg->data,BTREE_KeyIndexOffset(pg->data,k) - pg->data);
       p += BTREE_KeyIndexOffset(pg->data,k) - pg->data;

    if((k + 1) < (int)pg->n)
    {
        memcpy(p,BTREE_KeyIndexOffset(pg->data,k + 1), BTREE_KeyIndexOffset(pg->data,pg->n) - BTREE_KeyIndexOffset(pg->data,k + 1));
          p += BTREE_KeyIndexOffset(pg->data,pg->n) - BTREE_KeyIndexOffset(pg->data,k + 1);
    }


    if(k)
    {
        memcpy(p,BTREE_KeyData(pg,0),BTREE_KeyData(pg,k) - BTREE_KeyData(pg,0));
        p += BTREE_KeyData(pg,k) - BTREE_KeyData(pg,0);
    }
    if((k + 1) < (int)pg->n)
    {
        memcpy(p,BTREE_KeyData(pg,k + 1),BTREE_EndData(pg) - BTREE_KeyData(pg,k + 1));
        p += BTREE_EndData(pg) - BTREE_KeyData(pg,k + 1);
    }

    /* Compute length of deleted key */
    del_key_start = q = BTREE_KeyData(pg,k);
    q += uncompress2(&q);
    q += SizeInt32;
    del_key_end = q;
    del_entry_len = del_key_end - del_key_start;

    for(j = 0; j < k; j++)
    {
        tmp = BTREE_KeyDataOffset(pg,j) - SizeInt16;
        *(BTREE_KeyIndexOffset(buffer,j)) =  (unsigned char)((tmp & 0xff00) >>8);
        *(BTREE_KeyIndexOffset(buffer,j) + 1) = (unsigned char) (tmp & 0xff);
    }

    for(j = (int)(pg->n - 1) ; j >k; j--)
    {
        tmp = BTREE_KeyDataOffset(pg,j) - del_entry_len - SizeInt16;
        *(BTREE_KeyIndexOffset(buffer,j - 1)) =  (unsigned char)((tmp & 0xff00) >>8);
        *(BTREE_KeyIndexOffset(buffer,j - 1) + 1) = (unsigned char) (tmp & 0xff);
    }

    memcpy(pg->data, buffer, p - buffer);
    pg->n--;
    pg->data_end = p - buffer;
 
    return k;
}

BTREE *BTREE_New(FILE *fp, unsigned int size)
{
BTREE *b;
int i;
    b = (BTREE *) emalloc(sizeof(BTREE));
    b->page_size = size;
    b->fp = fp;
    for(i = 0; i < BTREE_CACHE_SIZE; i++)
        b->cache[i] = NULL;

    return b;
}

BTREE *BTREE_Create(FILE *fp, unsigned int size)
{
BTREE *b;
BTREE_Page *root;
    /* Round up size */
    size = BTREE_RoundPageSize(size);

    if(size > BTREE_MaxPageSize)
        return NULL;

    b = BTREE_New(fp , size);
    root = BTREE_NewPage(b, size, BTREE_ROOT_NODE | BTREE_LEAF_NODE);

    b->root_page = root->page_number;

    BTREE_WritePage(b, root);
    BTREE_FreePage(b, root);

    return b;
}


BTREE *BTREE_Open(FILE *fp, int size, unsigned long root_page)
{
BTREE *b;
    /* Round up size */
    size = BTREE_RoundPageSize(size);

    if(size > BTREE_MaxPageSize)
        return NULL;

    b = BTREE_New(fp , size);

    b->root_page = root_page;

    return b;
}

unsigned long BTREE_Close(BTREE *bt)
{
unsigned long root_page = bt->root_page;
    BTREE_FlushCache(bt);
    BTREE_CleanCache(bt);
    efree(bt);
    return root_page;
}


BTREE_Page *BTREE_Walk(BTREE *b, unsigned char *key, int key_len)
{
BTREE_Page *pg = BTREE_ReadPage(b, b->root_page);
unsigned int i = 0;
unsigned long next_page;
unsigned char *found;
unsigned int found_len;
unsigned long father_page;

    b->tree[i++] = 0;  /* No father for root */    
    
    father_page = pg->page_number;
    while(!(pg->flags & BTREE_LEAF_NODE))
    {
        next_page = BTREE_GetKeyFromPage(b, pg, key, key_len, &found, &found_len);
        BTREE_FreePage(b, pg);
        pg = BTREE_ReadPage(b, next_page);
        b->tree[i++] = father_page;
        father_page = pg->page_number;
    }
    b->levels = i;
    return pg;
}

BTREE_Page *BTREE_SplitPage(BTREE *b, BTREE_Page *pg)
{
BTREE_Page *new_pg = BTREE_NewPage(b, pg->size, pg->flags);
int         i,n;
unsigned char *key_data, *p, *q, *start;
int key_len;
int tmp;

    n=pg->n / 2;

    /* Key data of new page starts here */
    p = q = BTREE_KeyIndexOffset(new_pg->data, n);

    for(i = 0; i < n; i++)
    {
        key_data = start = BTREE_KeyData(pg, pg->n - n + i);
        key_len = uncompress2(&key_data);

        memcpy(p, start, (key_data - start) + key_len + SizeInt32);
        tmp = p - new_pg->data;
        p += (key_data - start) + key_len + SizeInt32;

        *(BTREE_KeyIndexOffset(new_pg->data,i)) =  (unsigned char)((tmp & 0xff00) >>8);
        *(BTREE_KeyIndexOffset(new_pg->data,i) + 1) = (unsigned char) (tmp & 0xff);
    }

    new_pg->n = n;
    new_pg->data_end = p - new_pg->data;

    pg->n -= n;
    p = BTREE_KeyIndexOffset(pg->data, pg->n);
    for(i = 0; i < (int)pg->n ; i++)
    {
        key_data = start = BTREE_KeyData(pg,i);
        key_len = uncompress2(&key_data);

        memmove(p, start, (key_data - start) + key_len + SizeInt32);
        tmp = p - pg->data;
        p += (key_data - start) + key_len + SizeInt32;

        *(BTREE_KeyIndexOffset(pg->data,i)) =  (unsigned char)((tmp & 0xff00) >>8);
        *(BTREE_KeyIndexOffset(pg->data,i) + 1) = (unsigned char) (tmp & 0xff);
    }
    pg->data_end = p - pg->data;;

    return new_pg;
}


int BTREE_InsertInPage(BTREE *b, BTREE_Page *pg, unsigned char *key, int key_len, unsigned long data_pointer, int level, int update)
{
BTREE_Page *new_pg, *next_pg, *root_page, *father_pg, *tmp_pg;
unsigned int free_space, required_space;
int key_pos, key_len0;
unsigned char *key_data0;
int comp;

    required_space = MAXINTCOMPSIZE + key_len + SizeInt32;

    /* Check for Duplicate key if we are in a leaf page */
    key_pos = BTREE_GetPositionForKey(pg, key, key_len, &comp);

    if(comp == 0 && (pg->flags & BTREE_LEAF_NODE))
    {
        BTREE_FreePage(b, pg); /* Dup Key */
        return -1;
    }

    free_space = pg->size - pg->data_end;
    if(required_space <= free_space)
    {
        if (comp > 0)
            key_pos++;
        else if(comp == 0 && (pg->flags & BTREE_LEAF_NODE))
        {
            BTREE_FreePage(b, pg); /* Dup Key */
            return -1;
        }

        if(!(pg->flags & BTREE_LEAF_NODE) && update)
        {
            BTREE_DelKeyInPage(pg, key_pos);
        }

        BTREE_AddKeyToPage(pg, key_pos, key,  key_len, data_pointer);

        if(key_pos == 0)
        {

            if(!(pg->flags & BTREE_ROOT_NODE))
            {
                key_data0 = BTREE_KeyData(pg,0);
                key_len0 = uncompress2(&key_data0);
                father_pg = BTREE_ReadPage(b,b->tree[level]);
                BTREE_InsertInPage(b,father_pg, key_data0, key_len0, pg->page_number, level - 1, 1);
            }
        }
        BTREE_WritePage(b, pg);
        BTREE_FreePage(b, pg);
        return 0;
    }

    /* There is not enough free space - Split page */
    new_pg = BTREE_SplitPage(b, pg);
    if(pg->next)
    {
        next_pg = BTREE_ReadPage(b, pg->next);
        next_pg->prev = new_pg->page_number;
        BTREE_WritePage(b, next_pg);
        BTREE_FreePage(b, next_pg);
    }
    new_pg->next = pg->next;
    new_pg->prev = pg->page_number;
    pg->next = new_pg->page_number;

    key_data0 = BTREE_KeyData(new_pg,0);
    key_len0 = uncompress2(&key_data0);

            /* Let's see where to put the key */
    if(BTREE_CompareKeys(key, key_len, key_data0, key_len0) > 0)
    {
        tmp_pg = new_pg;
    }
    else
    {
        tmp_pg = pg;
    }

    key_pos = BTREE_GetPositionForKey(tmp_pg, key, key_len, &comp);
    if(comp>0)
        key_pos++;

    if(!(tmp_pg->flags & BTREE_LEAF_NODE) && update)
    {
        BTREE_DelKeyInPage(tmp_pg, key_pos);
    }
    BTREE_AddKeyToPage(tmp_pg, key_pos, key, key_len, data_pointer);

    if(pg->flags & BTREE_ROOT_NODE)
    {
        pg->flags &= ~BTREE_ROOT_NODE;
        new_pg->flags &= ~BTREE_ROOT_NODE;
        root_page = BTREE_NewPage(b,b->page_size, BTREE_ROOT_NODE);

        key_data0 = BTREE_KeyData(pg,0);
        key_len0 = uncompress2(&key_data0);
        BTREE_AddKeyToPage(root_page, 0, key_data0, key_len0 , pg->page_number);
        key_data0 = BTREE_KeyData(new_pg,0);
        key_len0 = uncompress2(&key_data0);
        BTREE_AddKeyToPage(root_page, 1, key_data0, key_len0, new_pg->page_number);

        b->root_page = root_page->page_number;
        BTREE_WritePage(b, pg);
        BTREE_FreePage(b, pg);
        BTREE_WritePage(b, new_pg);
        BTREE_FreePage(b, new_pg);
        BTREE_WritePage(b, root_page);
        BTREE_FreePage(b, root_page);

        return 0;
    } 

    if(key_pos == 0 && tmp_pg == pg)
    {
        if(!(pg->flags & BTREE_ROOT_NODE))
        {
            father_pg = BTREE_ReadPage(b,b->tree[level]);
            BTREE_InsertInPage(b,father_pg, key, key_len, pg->page_number, level - 1, 1);
        }
    
        BTREE_WritePage(b, pg);
        BTREE_FreePage(b, pg);

        key_data0 = BTREE_KeyData(new_pg,0);
        key_len0 = uncompress2(&key_data0);
        BTREE_FreePage(b, BTREE_Walk(b,key_data0,key_len0));
    }
    else
    {
        BTREE_WritePage(b, pg);
        BTREE_FreePage(b, pg);

        key_data0 = BTREE_KeyData(new_pg,0);
        key_len0 = uncompress2(&key_data0);
    }

    if(!(new_pg->flags & BTREE_ROOT_NODE))
    {
        father_pg = BTREE_ReadPage(b,b->tree[level]);
        BTREE_InsertInPage(b,father_pg, key_data0, key_len0, new_pg->page_number, level - 1, 0);
    }

    BTREE_WritePage(b, new_pg);
    BTREE_FreePage(b, new_pg);

    return 0;
}


int BTREE_Insert(BTREE *b, unsigned char *key, int key_len, unsigned long data_pointer)
{
BTREE_Page *pg = BTREE_Walk(b,key,key_len);

    if(key_len>BTREE_MaxKeySize)
        return 0;

    return BTREE_InsertInPage(b, pg, key, key_len, data_pointer, b->levels - 1, 0);
}

int BTREE_Update(BTREE *b, unsigned char *key, int key_len, unsigned long new_data_pointer)
{
int comp, k, key_len_k;
unsigned char *key_k;
BTREE_Page *pg = BTREE_Walk(b,key,key_len);


    /* Pack pointer */
    new_data_pointer = PACKLONG(new_data_pointer);

    /* Get key position */
    k = BTREE_GetPositionForKey(pg, key, key_len, &comp);

    if(comp)
    {
        return -1;  /*Key not found */
    }

    key_k = BTREE_KeyData(pg,k);

    key_len_k = uncompress2(&key_k);

    if ( key_len_k != key_len)
        return -1;   /* Error - Should never happen */

    key_k += key_len_k;

    memcpy(key_k, &new_data_pointer, SizeInt32);

    BTREE_WritePage(b, pg);
    BTREE_FreePage(b, pg);

    return 0;
}


long BTREE_Search(BTREE *b, unsigned char *key, int key_len, unsigned char **found, int *found_len, int exact_match)
{
BTREE_Page *pg = BTREE_ReadPage(b, b->root_page);
unsigned int i = 0;
unsigned long next_page;
unsigned char *key_k;
unsigned int key_len_k;
unsigned long father_page;
unsigned long data_pointer;

    b->tree[i++] = 0;  /* No father for root */    
    
    father_page = pg->page_number;
    while(!(pg->flags & BTREE_LEAF_NODE))
    {
        next_page = BTREE_GetKeyFromPage(b, pg, key, key_len, &key_k, &key_len_k);
        BTREE_FreePage(b, pg);
        pg = BTREE_ReadPage(b, next_page);
        b->tree[i++] = father_page;
        father_page = pg->page_number;
    }
    b->levels = i;
    data_pointer = BTREE_GetKeyFromPage(b, pg, key, key_len, &key_k, &key_len_k);

    if(exact_match)
    {
        if(BTREE_CompareKeys(key,key_len,key_k,key_len_k)!=0)
            return -1L;
    }

    *found_len = key_len_k;
    *found = emalloc(key_len_k);
    memcpy(*found,key_k,key_len_k);

    BTREE_FreePage(b,pg);
    return data_pointer;
}

long BTREE_Next(BTREE *b, unsigned char **found, int *found_len)
{
BTREE_Page *pg = BTREE_ReadPage(b, b->current_page);
unsigned long next_page;
long data_pointer;
unsigned char *key_k;
int key_len_k;
    b->current_position++;
    if(pg->n == b->current_position)
    {
        next_page = pg->next;
        BTREE_FreePage(b,pg);
        if(!next_page)
            return -1;
        pg = BTREE_ReadPage(b, next_page);
        b->current_page = next_page;
        b->current_position = 0;
    }
    key_k = BTREE_KeyData(pg,b->current_position);
    *found_len = key_len_k = uncompress2(&key_k);
    *found = emalloc(key_len_k);
    memcpy(*found,key_k,key_len_k);
    data_pointer = UNPACKLONG(*(unsigned long *) (key_k + key_len_k));

    BTREE_FreePage(b,pg);

    return data_pointer;
}

#ifdef DEBUG

#include <time.h>

#define N_TEST 300000

#define F_READ_BINARY           "rb"
#define F_WRITE_BINARY          "wb"
#define F_READWRITE_BINARY      "rb+"

#define F_READ_TEXT             "r"
#define F_WRITE_TEXT            "w"
#define F_READWRITE_TEXT        "r+"

int main()
{
FILE *fp;
BTREE *bt;
unsigned char buffer[20];
int i;
static int nums[N_TEST];
unsigned long root_page;
unsigned char *found;
int found_len;
    srand(time(NULL));

    goto test2;

    fp = fopen("kkkkk",F_WRITE_BINARY);
    fwrite("asjhd",1,5,fp);
    fclose(fp);
    fp = fopen("kkkkk",F_READWRITE_BINARY);

printf("\n\nIndexing\n\n");

    bt = BTREE_Create(fp, 15);
    for(i=N_TEST - 1;i>=0;i--)
    {
//        nums[i] = rand();
        nums[i]=i;


        sprintf(buffer,"%d",nums[i]);
//        sprintf(buffer,"%.12d",nums[i]);
        BTREE_Insert(bt,buffer,strlen(buffer),nums[i]);
        if(nums[i]!= BTREE_Search(bt,buffer,strlen(buffer),&found,&found_len,1))
            printf("\n\nmal %s\n\n",buffer);
        if(!(i%1000))
        {
            BTREE_FlushCache(bt);
            printf("%d             \r",i);
        }
    }

    root_page = BTREE_Close(bt);
    fclose(fp);

search:;
printf("\n\nSearching\n\n");

    fp = fopen("kkkkk",F_READ_BINARY);
    bt = BTREE_Open(fp,15,root_page);

    for(i=0;i<N_TEST;i++)
    {
        sprintf(buffer,"%d",nums[i]);
//        sprintf(buffer,"%.12d",nums[i]);

        if(nums[i] != BTREE_Search(bt,buffer,strlen(buffer),&found,&found_len,1))
            printf("\n\nmal %s\n\n",buffer);
        if(!(i%1000))
            printf("%d             \r",i);
    }

    fclose(fp);

test2:;


    fp = fopen("kkkkk",F_WRITE_BINARY);
    fclose(fp);
    fp = fopen("kkkkk",F_READWRITE_BINARY);

    fwrite("aaa",1,3,fp);

printf("\n\nIndexing\n\n");

    bt = BTREE_Create(fp, 15);
    for(i=0;i<N_TEST;i++)
    {
//        nums[i] = rand();
        nums[i]=i;
        sprintf(buffer,"%d",nums[i]);
//        sprintf(buffer,"%.12d",nums[i]);
        BTREE_Insert(bt,buffer,strlen(buffer),nums[i]);
        if(nums[i]!= BTREE_Search(bt,buffer,strlen(buffer),&found,&found_len,1))
            printf("\n\nmal %s\n\n",buffer);
        if(!(i%1000))
        {
            BTREE_FlushCache(bt);
            printf("%d            \r",i);
        }
    }

    root_page = BTREE_Close(bt);
    fclose(fp);

printf("\n\nSearching\n\n");

    fp = fopen("kkkkk",F_READ_BINARY);
    bt = BTREE_Open(fp,15,root_page);

    for(i=0;i<N_TEST;i++)
    {
        sprintf(buffer,"%d",nums[i]);
        if(nums[i] != BTREE_Search(bt,buffer,strlen(buffer),&found,&found_len,1))
            printf("\n\nmal %s\n\n",buffer);
        if(!(i%1000))
            printf("%d            \r",i);
    }


    fclose(fp);
}

#endif
