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

#define IS_FLAG              0x80  /* Binary 10000000 */   
#define COMMON_STRUCTURE     0x60  /* Binary 01100000 */
#define COMMON_IN_FILE       0x20  /* Binary 00100000 */
#define COMMON_IN_RESERVED   0x40  /* Binary 01000000 */
#define POS_4_BIT            0x10  /* Binary 00010000 */
/************************************************************************

From Jose on Feb 13, 2002

IS_FLAG is to indicate that the byte is a flag. As far as I remember, I 
needed it to avoid null values.

When COMMON_STRUCTURE is on, this means that all the positions
have the same structure value. This helps a lot with non html
files and can save a lot of space.

When FREQ_AND_POS_EQ_1 is on, this means that freq is 1 and
pos[0]=1. Mmm, I am not sure if this is very useful now. Let me
explain better. This was useful for xml files with fields that contains
just one value. For example:
<code>00001</code>
<date>20001231</date>
But, now, I am not sure if this is useful because long time ago I
changed the position counter to not be reseted after a each field
change.
I need to check this.

POS_4_BIT indicates that all positions are within 16 positions
of each other and can thus be stored as 2 per byte.  Position numbers are
stored as a delta from the previous position.

Here's indexing /usr/doc:


23840 files indexed.  177638538 total bytes.  19739102 total words.
Elapsed time: 00:04:42 CPU time: 00:03:09
Indexing done!
      4 bit = 843,081 (total length = 10,630,425) 12 bytes/chunk
  not 4 bit = 13,052,904 (length 83,811,498) 6 bytes/chunk

I wonder if storing the initial postion would improve that much.




*************************************************************************/





void compress_location_values(unsigned char **buf,unsigned char **flagp,int filenum,int frequency, int *posdata)
{
    unsigned char *p = *buf;
    unsigned char *flag;
    int structure = GET_STRUCTURE(posdata[0]);
    int common_structure = COMMON_STRUCTURE;
    int i;

    /* Make room for flag and init it */
    flag = p;
    *flagp = p;
    p++;

    *flag = IS_FLAG;

    /* Add file number */
    p = compress3(filenum, p);


    /* Check for special case frequency == 1 and position[0] < 128  && structure == IN_FILE */
    if(frequency == 1 && (GET_POSITION(posdata[0]) < 128) && structure == IN_FILE)
    {
        /* Remove IS_FLAG and store position in the lower 7 bits */
        /* In this way we have 0bbbbbbb in *flag 
        ** where bbbbbbb is the position and the leading 0 bit
        ** indicates that frequency is 1 and position is < 128 */
        *flag = (unsigned char) ((int)(GET_POSITION(posdata[0])));
    }
    else
    {
        /* Otherwise IS_FLAG is set */
        /* Now, let's see if all positions have the same structure to 
        ** get better compression */
        for(i=1;i<frequency;i++)
        {
            if(structure != GET_STRUCTURE(posdata[i]))
            {
                common_structure = 0;
                break;
            }
        }
        if(frequency < 16)
             (*flag) |= frequency; /* Store freequency in flag - low 4 bits */
        else                       
             p = compress3(frequency, p); /* Otherwise, leave frequency "as is" */
        /* Add structure if it is equal for all positions */
        if(common_structure)
        {
            switch(structure)
            {
                case IN_FILE:
                    *flag |= COMMON_IN_FILE; 
                     break;     
/*
                case IN_RESERVED:
                    *flag |= COMMON_IN_RESERVED; 
                     break;     
*/
                default:         
                    *p++ = (unsigned char) structure;
                    *flag |= COMMON_STRUCTURE;
                    break;
            }
        }
    }
    *buf = p;
}

void compress_location_positions(unsigned char **buf,unsigned char *flag,int frequency, int *posdata)
{
    unsigned char *p = *buf;
    int i, j;

    if((*flag) & IS_FLAG)
    { 
        (*flag) |= POS_4_BIT;

        for(i = frequency - 1; i > 0 ; i--)
        {
            posdata[i] = SET_POSDATA(GET_POSITION(posdata[i]) - GET_POSITION(posdata[i-1]),GET_STRUCTURE(posdata[i]));
            if( GET_POSITION(posdata[i]) >= 16)
                (*flag) &= ~POS_4_BIT; 
        }

        /* Always write first position "as is" */
        p = compress3(GET_POSITION(posdata[0]), p);

        /* write the position data starting at 1 */
        if((*flag) & POS_4_BIT)
        {
            for (i = 1, j = 0; i < frequency ; i++, j++)
            {
                if(j % 2)
                    p[j/2] |= (unsigned char) GET_POSITION(posdata[i]);
                else
                    p[j/2] = (unsigned char) GET_POSITION(posdata[i]) << 4;
            }
            p += ((j + 1)/2);
        }
        else
        {
            for (i = 1; i < frequency; i++)
                p = compress3(GET_POSITION(posdata[i]), p);
        }

        /* Write out the structure bytes */
        if(! (*flag & COMMON_STRUCTURE))
            for(i = 0; i < frequency; i++)
                *p++ = (unsigned char) GET_STRUCTURE(posdata[i]);

        *buf = p;
    }
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

/* 2002/01 JMRUIZ
** Added an extra byte (MAXINTCOMPSIZE+1) for each position's structure
*/

    max_size = sizeof(unsigned char) + sizeof(LOCATION *) + (((sizeof(LOCATION) / sizeof(int) + 1) + (l->frequency - 1)) * (MAXINTCOMPSIZE + sizeof(unsigned char)));


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

    compress_location_values(&p,&flag,l->filenum,l->frequency, l->posdata);
    compress_location_positions(&p,flag,l->frequency,l->posdata);



    /* Get the length of all the data */
    i = p - idx->compression_buffer;


    /* Did we underrun our buffer? */
    if (i > idx->len_compression_buffer)
        progerr("Internal error in compress_location routine");


    q = (unsigned char *) Mem_ZoneAlloc(idx->currentChunkLocZone, i);
    memcpy(q, idx->compression_buffer, i);

    return (unsigned char *) q;
}


void uncompress_location_values(unsigned char **buf,unsigned char *flag, int *filenum,int *frequency)
{
    unsigned char *p = *buf;

    *frequency = 0;

    *flag = *p++;

    if(!((*flag) & IS_FLAG))
    {
        *frequency = 1;
    }
    else
        (*frequency) |= (*flag) & 15;   /* Binary 00001111 */

    *filenum = uncompress2(&p);

    if(! (*frequency))
        *frequency = uncompress2(&p);

    *buf = p;
}

unsigned long four_bit_count = 0;
unsigned long four_bit_bytes = 0;
unsigned long not_four = 0;
unsigned long not_four_bytes = 0;
unsigned long four_bit_called = 0;
unsigned long not_four_called;


void uncompress_location_positions(unsigned char **buf, unsigned char flag, int frequency, int *posdata)
{
    int i, j, tmp;
    unsigned char *p = *buf;
    int common_structure = 0;
    int structure = 0;

    /* Check for special case frequency == 1 and position[0] < 128 and structure == IN_FILE */
    if (!(flag & IS_FLAG))
    {
        structure = IN_FILE;
        posdata[0] =  SET_POSDATA((int)(flag),structure);
    }
    else
    {
        /* Check for common structure */
        if ((tmp =(flag & COMMON_STRUCTURE)))
        {
            common_structure = COMMON_STRUCTURE;
            switch(tmp)
            {
                case COMMON_IN_FILE:
                    structure = IN_FILE;
                    break;
/*
                case COMMON_IN_RESERVED:
                    structure = IN_RESERVED;
                    break;
*/
                default:
                    structure = (int)((unsigned char) *p++);
                    break;
            }
        }

        /* First position is always "as is" */
        posdata[0] = uncompress2(&p);

        /* Check if positions where stored as two values per byte or the old "compress" style */
        if(flag & POS_4_BIT)
        {
            for (i = 1, j = 0; i < frequency; i++, j++)
            {
                if(j%2)
                    posdata[i] = p[j/2] & 0x0F;
                else
                    posdata[i] = p[j/2] >> 4;
            }
            p += ((j + 1)/2);
        }
        else
        {
            for (i = 1; i < frequency; i++)
            {
                tmp = uncompress2(&p);
                posdata[i] = tmp;
            }
        }
        /* Position were compressed incrementally. So restore them */
        for(i = 1; i < frequency; i++)
            posdata[i] += posdata[i-1];

        /* Get structure */
        for(i = 0; i < frequency; i++)
        {
            if(!common_structure)
                structure = (int)((unsigned char) *p++);

            posdata[i] = SET_POSDATA(posdata[i],structure);
        }
    }
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
            frequency;
    unsigned char flag;

    metaID = uncompress2(&p);

    uncompress_location_values(&p,&flag,&filenum,&frequency);

    loc = (LOCATION *) emalloc(sizeof(LOCATION) + (frequency - 1) * sizeof(int));

    loc->metaID = metaID;
    loc->filenum = filenum;
    loc->frequency = frequency;

    uncompress_location_positions(&p,flag,frequency,loc->posdata);

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
void SwapLocData(SWISH * sw, ENTRY *e, unsigned char *buf, int lenbuf)
{
    int     idx_swap_file;
    struct  MOD_Index *idx = sw->Index;

    /* 2002-07 jmruiz - Get de corrsponding swap file */
    /* Get the corresponding hash value to this word  */
    /* IMPORTANT!!!! - The routine being used here to compute the hash */  
    /* must be the same used to store the words */
    /* Then we must get the corresponding swap file index */
    /* Since we cannot have so many swap files (SEARCHHASHSIZE for searchhash */
    /* routine) we must scale the hash into SWAP_FILES */
    idx_swap_file = (searchhash(e->word) * (MAX_LOC_SWAP_FILES -1))/(SEARCHHASHSIZE -1);

    if (!idx->fp_loc_write[idx_swap_file])
    {
        idx->fp_loc_write[idx_swap_file] = create_tempfile(sw, F_WRITE_BINARY, "loc", &idx->swap_location_name[idx_swap_file], 0 );
    }

    compress1(lenbuf, idx->fp_loc_write[idx_swap_file], idx->swap_putc);
    if (idx->swap_write(buf, 1, lenbuf, idx->fp_loc_write[idx_swap_file]) != (unsigned int) lenbuf)
    {
        progerr("Cannot write location to swap file");
    }
}

void unSwapLocData(SWISH * sw, int idx_swap_file)
{
    unsigned char *buf;
    int     lenbuf;
    struct MOD_Index *idx = sw->Index;
    ENTRY *e;
    LOCATION *l;
    FILE *fp;

    /* Chaeck if some swap file is being used */
    if(!idx->fp_loc_write[idx_swap_file])
       return;

    /* Reopen in read mode for (for faster reads, I suppose) */
    /* Write a 0 to mark the end of locations */
    idx->swap_putc(0,idx->fp_loc_write[idx_swap_file]);
    idx->swap_close(idx->fp_loc_write[idx_swap_file]);
    idx->fp_loc_write[idx_swap_file] = NULL;
    if (!(idx->fp_loc_read[idx_swap_file] = fopen(idx->swap_location_name[idx_swap_file], F_READ_BINARY)))
        progerrno("Could not open temp file %s: ", idx->swap_location_name[idx_swap_file]);
    
    fp = idx->fp_loc_read[idx_swap_file];
    while((lenbuf = uncompress1(fp, idx->swap_getc)))
    {
        buf = (unsigned char *) Mem_ZoneAlloc(idx->totalLocZone,lenbuf);
        idx->swap_read(buf, lenbuf, 1, fp);
        e = *(ENTRY **)buf;
        /* Store the locations in reverse order - Faster */
        l = (LOCATION *) buf;
        l->next = e->allLocationList;
        e->allLocationList = l;
    }
}

/* 2002-07 jmruiz - Sorts unswaped location data by metaname, filenum */
void sortSwapLocData(SWISH * sw, ENTRY *e)
{
    int i, j, k, metaID;
    int    *pi = NULL;
    unsigned char *ptmp,
           *ptmp2,
           *compressed_data;
    LOCATION **tmploc;
    LOCATION *l, *prev=NULL, **lp;

    /* Count the locations */
    for(i = 0, l = e->allLocationList; l;i++, l = l->next);
    
    /* Very trivial case */
    if(i < 2)
        return;

    /* */
    /* Now, let's sort by metanum, offset in file */

    /* Compute array wide for sort */
    j = 2 * sizeof(int) + sizeof(void *);

    /* Compute array size */
    ptmp = (void *) emalloc(j * i);

    /* Build an array with the elements to compare
       and pointers to data */

    /* Very important to remind - data was read from the loc
    ** swap file in reverse order, so, to get data sorted
    ** by filenum we just need to use a reverse counter (i - k)
    ** as the other value for sorting (it has the same effect
    ** as filenum)
    */
    for(k=0, ptmp2 = ptmp, l = e->allLocationList ; k < i; k++, l = l->next)
    {
        pi = (int *) ptmp2;

        compressed_data = (unsigned char *)l;
        /* Jump fileoffset */
        compressed_data += sizeof(LOCATION *);

        metaID = uncompress2(&compressed_data);
        pi[0] = metaID;
        pi[1] = i-k;
        ptmp2 += 2 * sizeof(int);

        lp = (LOCATION **)ptmp2;
        *lp = l;
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
                 
}

/* 09/00 Jose Ruiz
** Gets the location data from the swap file
** Returns a memory compressed location data
*/
unsigned char *unSwapLocData_old(SWISH * sw, ENTRY *e, long pos)
{
    unsigned char *buf;
    int     i,lenbuf;
    struct MOD_Index *idx = sw->Index;


    /* Reopen in read mode for (for faster reads, I suppose) */
    if (!idx->fp_loc_read[0])
    {
        for( i = 0; i < MAX_LOC_SWAP_FILES ; i++)
        {
            idx->swap_close(idx->fp_loc_write[i]);
            idx->fp_loc_write[i] = NULL;
            if (!(idx->fp_loc_read[i] = fopen(idx->swap_location_name[i], F_READ_BINARY)))
                progerrno("Could not open temp file %s: ", idx->swap_location_name[i]);
        }
    }

    i = (int) (((int) e) % MAX_LOC_SWAP_FILES);

    idx->swap_seek(idx->fp_loc_read[i], pos, SEEK_SET);
    lenbuf = uncompress1(idx->fp_loc_read[i], idx->swap_getc);
    buf = (unsigned char *) Mem_ZoneAlloc(idx->totalLocZone,lenbuf);
    idx->swap_read(buf, lenbuf, 1, idx->fp_loc_read[i]);
    return buf;
}


/* Gets all LOCATIONs for an entry and puts them in memory */
void unSwapLocDataEntry_old(SWISH *sw,ENTRY *e)
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
        p_array[i] = (LOCATION *)unSwapLocData_old(sw,e,fileoffset);
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




