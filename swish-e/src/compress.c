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
**
** 2001-02-12 rasc   errormsg "print" changed...
**
*/

#include "swish.h"
#include "string.h"
#include "compress.h"
#include "mem.h"
#include "error.h"
#include "merge.h"
#include "docprop.h"
#include "index.h"
#include "search.h"
#include "hash.h"
#include "ramdisk.h"
#include "swish_qsort.h"
#include "file.h"


/* 2001-05 jmruiz */
/* Routines for compressing numbers - Macros converted to routines */

/* Compress a number and writes it to a file */
void    compress1(int num, FILE * fp, int (*f_putc) (int, FILE *))
{
    int     _i = 0,
            _r = num;
    unsigned char _s[5];

    while (_r)
    {
        _s[_i++] = _r & 127;
        _r >>= 7;
    }
    while (--_i >= 0)
        f_putc(_s[_i] | (_i ? 128 : 0), fp);
}

/* Compress a number and writes it to a buffer */
/* buffer must be previously allocated */
/* returns the decreased buffer pointer after storing the compressed number in it */
unsigned char *SW_compress2(int num, unsigned char *buffer)
{
    int     _i = num;

    while (_i)
    {
        *buffer = _i & 127;
        if (_i != num)
            *buffer |= 128;
        _i >>= 7;
        buffer--;
    }

    return buffer;
}

/* Compress a number and writes it to a buffer */
/* buffer must be previously allocated */
/* returns the incrmented buffer pointer after storing the compressed number in it */
unsigned char *compress3(int num, unsigned char *buffer)
{
    int     _i = 0,
            _r = num;
    unsigned char _s[5];

    while (_r)
    {
        _s[_i++] = _r & 127;
        _r >>= 7;
    }
    while (--_i >= 0)
        *buffer++ = (_s[_i] | (_i ? 128 : 0));

    return buffer;
}

/* Uncompress a number from a file */
int     uncompress1(FILE * fp, int (*f_getc) (FILE *))
{
    int     _c;
    int     num = 0;

    do
    {
        _c = (int) f_getc(fp);
        num <<= 7;
        num |= _c & 127;
        if (!num)
            break;
    }
    while (_c & 128);

    return num;
}

/* same routine but this works with a memory forward buffer instead of file */
/* it also increases the buffer pointer */
int     uncompress2(unsigned char **buffer)
{
    int     _c;
    int     num = 0;
    unsigned char *p = *buffer;

    do
    {
        _c = (int) ((unsigned char) *p++);
        num <<= 7;
        num |= _c & 127;
        if (!num)
            break;
    }
    while (_c & 128);

    *buffer = p;
    return num;
}

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

/* Same routine - Packs long in buffer */
void    PACKLONG2(unsigned long num, unsigned char *buffer)
{
    buffer[0] = (unsigned char) ((num >> 24) & 0xFF);
    buffer[1] = (unsigned char) ((num >> 16) & 0xFF);
    buffer[2] = (unsigned char) ((num >> 8) & 0xFF);
    buffer[3] = (unsigned char) (num & 0xFF);
}


unsigned long UNPACKLONG(unsigned long num)
{
    unsigned char *_s = (unsigned char *) &num;

    return (_s[0] << 24) + (_s[1] << 16) + (_s[2] << 8) + _s[3];
}

/* Same macro - UnPacks long from buffer */
unsigned long UNPACKLONG2(unsigned char *buffer)
{
    return (buffer[0] << 24) + (buffer[1] << 16) + (buffer[2] << 8) + buffer[3];
}



/***********************************************************************************
*   09/00 Jose Ruiz 
*   Function to compress location data in memory 
*
*   Compresses a LOCATION entry
*
*   A single position LOCATION goes from 20 to 3 bytes.
*   three positions goes from 28 to 5.
* 
************************************************************************************/

#define IS_FLAG              0x80  /* Binary 10000000 */   /* Always set */
#define STRUCTURE_EQ_1       0x40  /* Binary 01000000 */
#define FREQ_AND_POS_EQ_1    0x20  /* Binary 00100000 */
#define POS_4_BIT            0x10  /* Binary 00010000 */

void compress_location_values(unsigned char **buf,unsigned char **flagp,int filenum,int structure,int frequency, int position0)
{
    unsigned char *p = *buf;
    unsigned char *flag;

    /* Make room for flag and init it */
    flag = p;
    *flagp = p;
    p++;

    *flag = IS_FLAG;

    /* Add file number */
    p = compress3(filenum, p);

    /* Add structure */
    if(structure == 1)
        (*flag) |= STRUCTURE_EQ_1;
    else
        *p++ = (unsigned char) structure;

    /* Check for special case frequency ==1 and position[0] == 1 */
    if(frequency == 1 && position0 == 1)
    {
        *flag |= FREQ_AND_POS_EQ_1;
    }
    else
    {
        if(frequency < 16)
             (*flag) |= frequency; /* Store in flag - last 4 bit 0000xxxx */
        else
             p = compress3(frequency, p);
    }
    *buf = p;
}

void compress_location_positions(unsigned char **buf,unsigned char *flag,int frequency, int *position)
{
    unsigned char *p = *buf;
    int i;

    if(! ((*flag) & FREQ_AND_POS_EQ_1))
    {
        /* Adjust positions to store them incrementally */
        if(position[0] < 16)
            (*flag) |= POS_4_BIT;
        else
            (*flag) &= ~POS_4_BIT; 
        for(i = frequency - 1; i > 0 ; i--)
        {
            position[i] -= position[i-1];
            if(position[i] >= 16)
                (*flag) &= ~POS_4_BIT; 
        }
        /* write the position data working from the end of the buffer */
        if((*flag) & POS_4_BIT)
        {
            for (i = 0; i < frequency; i++)
            {
                if(i % 2)
                    p[i/2] |= (unsigned char) position[i];
                else
                    p[i/2] = (unsigned char) position[i] << 4;
            }
            p += ((frequency + 1)/2);
        }
        else
        {
            for (i = 0; i < frequency; i++)
                p = compress3(position[i], p);
        }
    }
    *buf = p;
}

unsigned char *compress_location(SWISH * sw, IndexFILE * indexf, LOCATION * l)
{
    unsigned char *p,
           *q;
    int     i,
            max_size;
    unsigned char *flag;
    struct MOD_Index *idx = sw->Index;

    /* check if the work buffer is long enough */
    /* just to avoid bufferoverruns */
    /* In the worst case and integer will need 5 bytes */
    /* but fortunatelly this is very uncommon */

//***JMRUIZ    max_size = ((sizeof(LOCATION) / sizeof(int) + 1) + (l->frequency - 1)) * 5;
    max_size = sizeof(unsigned char) + sizeof(LOCATION *) + (((sizeof(LOCATION) / sizeof(int) + 1) + (l->frequency - 1)) * 5);


    /* reallocate if needed */
    if (max_size > idx->len_compression_buffer)
    {
        idx->len_compression_buffer = max_size + 200;
        idx->compression_buffer = erealloc(idx->compression_buffer, idx->len_compression_buffer);
    }


    /* Pointer to the buffer */
    p = idx->compression_buffer;

    /* Add extra bytes for handling linked list */
//***JMRUIZ

    memcpy(p,&l->next,sizeof(LOCATION *));
    p += sizeof(LOCATION *);

    /* Add the metaID */
    p = compress3(l->metaID,p);

    compress_location_values(&p,&flag,l->filenum,l->structure,l->frequency, l->position[0]);
    compress_location_positions(&p,flag,l->frequency,l->position);



    /* Get the length of all the data */
    i = p - idx->compression_buffer;


    /* Did we underrun our buffer? */
    if (i > idx->len_compression_buffer)
        progerr("Internal error in compress_location routine");


    q = (unsigned char *) Mem_ZoneAlloc(idx->currentChunkLocZone, i);
    memcpy(q, idx->compression_buffer, i);

    return (unsigned char *) q;
}


void uncompress_location_values(unsigned char **buf,unsigned char *flag, int *filenum,int *structure,int *frequency)
{
    unsigned char *p = *buf;

    *structure = 0;
    *frequency = 0;

    *flag = *p++;

    (*structure) = ((*flag) & STRUCTURE_EQ_1) ? 1 : 0;

    if((*flag) & FREQ_AND_POS_EQ_1)
    {
        *frequency = 1;
    }
    else
        (*frequency) |= (*flag) & 15;   /* Binary 00001111 */

    *filenum = uncompress2(&p);

    if(! (*structure))
        *structure = *p++;
    if(! (*frequency))
        *frequency = uncompress2(&p);
    *buf = p;
}

void uncompress_location_positions(unsigned char **buf, unsigned char flag, int frequency, int *position)
{
    int i, tmp;
    unsigned char *p = *buf;

    /* Chaeck for special case frequency == 1 and position[0] == 1 */
    if(flag & FREQ_AND_POS_EQ_1)
    {
        position[0] = 1;
        return;
    }

    /* Check if positions where stored as two values per byte or the old "compress" style */
    if(flag & POS_4_BIT)
    {
        for (i = 0; i < frequency; i++)
        {
            if(i%2)
                position[i] = p[i/2] & 0x0F;
            else
                position[i] = p[i/2] >> 4;
        }
        p += ((frequency +1)/2);
    }
    else
    {
        for (i = 0; i < frequency; i++)
        {
            tmp = uncompress2(&p);
            position[i] = tmp;
        }
    }
    /* Position were compressed incrementally. So restore them */
    for(i = 1; i < frequency; i++)
        position[i] += position[i-1];
    /* Update buffer pointer */
    *buf = p;
}

/* 09/00 Jose Ruiz 
** Extract compressed location
*/
LOCATION *uncompress_location(SWISH * sw, IndexFILE * indexf, unsigned char *p)
{
    LOCATION *loc;
    int     metaID,
            filenum,
            structure,
            frequency;
    unsigned char flag;

    metaID = uncompress2(&p);

    uncompress_location_values(&p,&flag,&filenum,&structure,&frequency);

    loc = (LOCATION *) emalloc(sizeof(LOCATION) + (frequency - 1) * sizeof(int));

    loc->metaID = metaID;
    loc->filenum = filenum;
    loc->structure = structure;
    loc->frequency = frequency;

    uncompress_location_positions(&p,flag,frequency,loc->position);

    return loc;
}


/* 09/00 Jose Ruiz
** Compress all non yet compressed location data of an entry
*/
void    CompressCurrentLocEntry(SWISH * sw, IndexFILE * indexf, ENTRY * e)
{
    LOCATION *l, *prev, *next, *comp;
   
    for(l = e->currentChunkLocationList,prev = NULL ; l != e->currentlocation; )
    {
        next = l->next;
        comp = (LOCATION *) compress_location(sw, indexf, l);
        if(l == e->currentChunkLocationList)
            e->currentChunkLocationList =comp;
        if(prev)
            memcpy(prev, &comp, sizeof(LOCATION *));   /* Use memcpy to avoid alignment problems */
        prev = comp;
        l = next;        
    } 
    e->currentlocation = e->currentChunkLocationList;
}

/* 09/00 Jose Ruiz
** Function to swap location data to a temporal file or ramdisk to save
** memory. Unfortunately we cannot use it with IgnoreLimit option
** enabled.
** The data has been compressed previously in memory.
** Returns the pointer to the file.
*/
long    SwapLocData(SWISH * sw, unsigned char *buf, int lenbuf)
{
    long    pos;
    struct MOD_Index *idx = sw->Index;

    if (!idx->fp_loc_write)
    {
        idx->fp_loc_write = create_tempfile(sw, F_WRITE_BINARY, "loc", &idx->swap_location_name, 0 );

        idx->swap_tell = ftell;
        idx->swap_write = fwrite;
        idx->swap_close = fclose;
        idx->swap_seek = fseek;
        idx->swap_read = fread;
        idx->swap_getc = fgetc;
        idx->swap_putc = fputc;
        /* 2001-09 jmruiz - FIX- Write one byte to avoid starting at 0 */
		/* If not, This can cause problems with pointers to NULL in allLocationlist */

    }

    if (idx->swap_write("", 1, 1, idx->fp_loc_write) != (unsigned int) 1)
    {
        progerr("Cannot write location to swap file");
    }

    pos = idx->swap_tell(idx->fp_loc_write);
    compress1(lenbuf, idx->fp_loc_write, idx->swap_putc);
    if (idx->swap_write(buf, 1, lenbuf, idx->fp_loc_write) != (unsigned int) lenbuf)
    {
        progerr("Cannot write location to swap file");
    }
    return pos;
}

/* 09/00 Jose Ruiz
** Gets the location data from the swap file
** Returns a memory compressed location data
*/
unsigned char *unSwapLocData(SWISH * sw, long pos)
{
    unsigned char *buf;
    int     lenbuf;
    struct MOD_Index *idx = sw->Index;


    /* Reopen in read mode for (for faster reads, I suppose) */
    if (!idx->fp_loc_read)
    {
        idx->swap_close(idx->fp_loc_write);
        idx->fp_loc_write = NULL;
        if (!(idx->fp_loc_read = fopen(idx->swap_location_name, F_READ_BINARY)))
            progerrno("Could not open temp file %s", idx->swap_location_name);
    }

    idx->swap_seek(idx->fp_loc_read, pos, SEEK_SET);
    lenbuf = uncompress1(idx->fp_loc_read, idx->swap_getc);
    buf = (unsigned char *) Mem_ZoneAlloc(idx->totalLocZone,lenbuf);
    idx->swap_read(buf, lenbuf, 1, idx->fp_loc_read);
    return buf;
}


/* Gets all LOCATIONs for an entry and puts them in memory */
void unSwapLocDataEntry(SWISH *sw,ENTRY *e)
{
    int     i,
            j,
            k,
            metaID;
    unsigned char *ptmp,
           *ptmp2,
           *compressed_data;
    int    *pi = NULL;
    LOCATION *l, *prev=NULL, **lp;
    LOCATION **tmploc;
    LOCATION **p_array,*array[MAX_STACK_POSITIONS];
    int array_size;
    long fileoffset;

    if(!e->allLocationList)
        return;

    p_array = array;
    array_size = sizeof(array)/sizeof(LOCATION *);

    /* Read all locations */
    for(i = 0, l = e->allLocationList; l; i++)
    {
        if(i == MAX_STACK_POSITIONS)
        {
            p_array = (LOCATION **)emalloc((array_size *=2) * sizeof(LOCATION *));
            memcpy(p_array,array,MAX_STACK_POSITIONS * sizeof(LOCATION *));
        }
        if(array_size==i)
        {
            p_array = (LOCATION **) erealloc(p_array,(array_size *=2) * sizeof(LOCATION *));
        }
        fileoffset = (long)l;
        p_array[i] = (LOCATION *)unSwapLocData(sw,fileoffset);
        l=*(LOCATION **)p_array[i];    /* Get fileoffset to next location */
        /* store current file offset for later sort */
        tmploc = (LOCATION **) p_array[i];
        *tmploc = (LOCATION *) fileoffset;
    }

    /* Now, let's sort by metanum, offset in file */

    /* Compute array wide for sort */
    j = 2 * sizeof(int) + sizeof(void *);

    /* Compute array size */
    ptmp = (void *) emalloc(j * i);

    /* Build an array with the elements to compare
       and pointers to data */

    for(k=0, ptmp2 = ptmp ; k < i; k++)
    {
        pi = (int *) ptmp2;

        compressed_data = (unsigned char *)p_array[k];
        tmploc = (LOCATION **) p_array[k];
        fileoffset = (long) *tmploc;
        /* Jump fileoffset */
        compressed_data += sizeof(LOCATION *);

        metaID = uncompress2(&compressed_data);
        pi[0] = metaID;
        pi[1] = fileoffset;
        ptmp2 += 2 * sizeof(int);

        lp = (LOCATION **)ptmp2;
        *lp = p_array[k];
        ptmp2 += sizeof(void *);
    }

    /* Sort them */
    swish_qsort(ptmp, i, j, &icomp2);

    /* Store results */
    for (k = 0, ptmp2 = ptmp; k < i; k++)
    {
        ptmp2 += 2 * sizeof(int);

        l = *(LOCATION **)ptmp2;
        if(!k)
            e->allLocationList = l;
        else
        {
            tmploc = (LOCATION **)prev;
            *tmploc = l;
        }
        ptmp2 += sizeof(void *);
        prev = l;
    }
    tmploc = (LOCATION **)l;
    *tmploc = NULL;

    /* Free the memory of the sorting array */
    efree(ptmp);

    if(p_array != array)
        efree(p_array);
}




