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
#include "swstring.h"
#include "compress.h"
#include "mem.h"
#include "error.h"
#include "merge.h"
#include "search.h"
#include "docprop.h"
#include "index.h"
#include "hash.h"
#include "ramdisk.h"
#include "swish_qsort.h"
#include "file.h"

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

/* Surfing the web I found this:
** it is a very simple macro that can be used in *PACKLONG* routines to 
** detect if we need to spend some cycles for [un]packing the number in a
** portable format
*/
#ifndef LITTLE_ENDIAN
static const int swish_endian_test_value = 1;
#define LITTLE_ENDIAN (*(const unsigned char *)&swish_endian_test_value)
#endif

/* 2001-05 jmruiz */
/* Routines for compressing numbers - Macros converted to routines */

/* 2002-11 jmruiz */
/* Get required size in bytes for a given compressed number */
int sizeofcompint(int number)
{
int size = 0;
    do
    {
        size++;
    } 
    while ((number >>= 7));
    return size;
}

/* Compress a number and writes it to a file */
void    compress1(int num, FILE * fp, int (*f_putc) (int, FILE *))
{
    int     _i = 0,
            _r = num;
    unsigned char _s[MAXINTCOMPSIZE];

    /* Trivial case: 0 */
    if(!_r)
    {
        f_putc(0,fp);
        return;
    }

    /* Any other case ... */
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

    /* Trivial case: 0 */
    if(!_i)
    {
        *buffer-- = 0; 
        return 0;
    }

    /* Any other case ... */
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
    unsigned char _s[MAXINTCOMPSIZE];

    /* Trivial case: 0 */
    if(!_r)
    {
        *buffer++ = 0;
        return buffer;
    }

    /* Any other case ... */
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
    unsigned long tmp = 0L;
    unsigned char *s;
    int sz_long = sizeof(unsigned long);  

    if (num && LITTLE_ENDIAN)
    {
        s = (unsigned char *) &tmp;
        while(sz_long)
            *s++ = (unsigned char) ((num >> ((--sz_long)<<3)) & 0xFF);

        return tmp;
    }
    return num;
}

/* Same routine - Packs long in buffer */
void    PACKLONG2(unsigned long num, unsigned char *s)
{
    int sz_long = sizeof(unsigned long);

    if(LITTLE_ENDIAN)
    {
        while(sz_long)
            *s++ = (unsigned char) ((num >> ((--sz_long)<<3)) & 0xFF);
    }
    else
    {
        memcpy(s,(unsigned char *)&num,sz_long);
    }
}


unsigned long UNPACKLONG(unsigned long num)
{
    int sz_long = sizeof(unsigned long);
	unsigned long tmp = 0;
    unsigned char *s = (unsigned char *) &num;

    if(LITTLE_ENDIAN)
    {
        while(sz_long)
            tmp += *s++ << ((--sz_long)<<3);
        return tmp;
    }
    return num;
}

/* Same macro - UnPacks long from buffer */
unsigned long UNPACKLONG2(unsigned char *s)
{
    int sz_long = sizeof(unsigned long);
	unsigned long tmp = 0;

    if(LITTLE_ENDIAN)
    {
        while(sz_long)
            tmp += *s++ << ((--sz_long)<<3);
    }
    else
    {
        memcpy((unsigned char *)&tmp,s,sz_long);
    }
    return tmp;
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
#define COMMON_IN_HTML_BODY  0x40  /* Binary 01000000 */
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

                case IN_BODY | IN_FILE:
                    *flag |= COMMON_IN_HTML_BODY; 
                     break;     

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

static unsigned char *compress_location(SWISH * sw, LOCATION * l)
{
    unsigned char *p,
           *q;
    int     i,
            max_size;
    unsigned char *flag;
    struct MOD_Index *idx = sw->Index;

    /* check if the work buffer is long enough */
    /* just to avoid bufferoverruns */
    /* In the worst case and integer will need MAXINTCOMPSIZE bytes */
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

                case COMMON_IN_HTML_BODY:
                    structure = IN_FILE | IN_BODY;
                    break;

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
** Compress all non yet compressed location data of an entry
*/
void    CompressCurrentLocEntry(SWISH * sw, ENTRY * e)
{
    LOCATION *l, *prev, *next, *comp;
   
    for(l = e->currentChunkLocationList,prev = NULL ; l != e->currentlocation; )
    {
        next = l->next;
        comp = (LOCATION *) compress_location(sw, l);
        if(l == e->currentChunkLocationList)
            e->currentChunkLocationList =comp;
        if(prev)
            memcpy(prev, &comp, sizeof(LOCATION *));   /* Use memcpy to avoid alignment problems */
        prev = comp;
        l = next;        
    } 
    e->currentlocation = e->currentChunkLocationList;
}


/* 2002/11 jmruiz
** Simple routine to compress worddata using zlib where available
**
** On exit returns the new size of the compressed buffer
*/
int compress_worddata(unsigned char *wdata,int wdata_size)
{
#ifndef HAVE_ZLIB
    return wdata_size;
#else
    unsigned char  *WDataBuf;     /* For compressing and uncompressing */
    uLongf          dest_size;
    int             zlib_status = 0;
    unsigned char   local_buffer[8192];/* Just to avoid emalloc/efree overhead*/


    /* Don't bother compressing smaller items */
    if ( wdata_size < MIN_WORDDATA_COMPRESS_SIZE )
    {
        return wdata_size;
    }

    /* Buffer should be +1% + a few bytes. */
    dest_size = (uLongf)(wdata_size + ( wdata_size / 100 ) + 1000);  // way more than should be needed

    /* Get an output buffer */
    if( dest_size > sizeof(local_buffer) )
        WDataBuf = (unsigned char *) emalloc((int)dest_size );
    else
        WDataBuf = local_buffer;

    zlib_status = compress2((Bytef *)WDataBuf, &dest_size, wdata, wdata_size, 9);
    if ( zlib_status != Z_OK )
        progerr("WordData Compression Error.  zlib compress2 returned: %d  Worddata size: %d compress buf size: %d", zlib_status, wdata_size, (int)dest_size);

    /* Make sure it's compressed enough -- should check that destsize is not > MAXINT */
    if ( (int)dest_size < wdata_size )
    {
        memcpy(wdata,WDataBuf,(int)dest_size);
    }
    else
    {
        dest_size = wdata_size;
    }

    if ( WDataBuf != local_buffer)
        efree(WDataBuf);
    return (int)dest_size;
#endif
}


/* 2002/11 jmruiz
** Routine to uncompress worddata
*/
void uncompress_worddata(unsigned char **buf, int *buf_size, int saved_bytes)
{
#ifdef HAVE_ZLIB
    unsigned char *new_buf;
    int             zlib_status = 0;
    uLongf          new_buf_size = (uLongf)(*buf_size + saved_bytes);

    if(! saved_bytes)     /* nothing to do */
        return;   
    new_buf= (unsigned char *) emalloc(*buf_size + saved_bytes);
    zlib_status = uncompress(new_buf, &new_buf_size, *buf, (uLongf)buf_size );
    if ( zlib_status != Z_OK )
    {
        // $$$ make sure this works ok if returning null $$$
        progwarn("Failed to uncompress Property. zlib uncompress returned: %d.  uncompressed size: %d buf_len: %d\n",
            zlib_status, new_buf_size, buf_size );
        return;
    }
    efree(*buf);
    *buf_size = (int)new_buf_size;
    *buf = new_buf;
#else
    if ( saved_bytes )
        progerr("The index was created with zlib compression.\n This version of swish was not compiled with zlib");
#endif
}


/* 2002/09 jmruiz
** This routine changes longs in worddata by shorter compressed
** numbers.
**
** Here are two reasons for using compressed numbers in worddata
** instead of longs:
**   - Compressed numbers are more portable: longs are usually 4 bytes
**     long in a 32 bit machine but in a 64 bit alpha they are 8 bytes
**     long (this a waste of space).
**   - The obvious one is that compressed numbers use less disk space
**
** BTW, Any change in worddata will also affect to dump.c, merge.c and search.c
** (getfileinfo routine).
**
**  worddata has the following format before entering the routine
**  <tfreq><metaID><nextposmetaID><data><metaID><nextposmetaID><data>...
**
**  Entering this routine nextposmetaID is the offset to next metaid
**  in bytes starting to count them from the begining of worddata.
**  It is a packed long number (sizeof(long) bytes).
**
**  Exiting this routine, nextposmetaID has changed to be the size of
**  the data block and is stored as a compressed number.
**
**  In other words, worddata has the following format:
**  <tfreq><metaID><data_len><data><metaID><data_len><data>...
**
*/
void    remove_worddata_longs(unsigned char *worddata,int *sz_worddata)
{
    unsigned char *src,*dst;   //source and dest pointers for worddata
    unsigned int metaID, tfrequency, data_len;
    unsigned long nextposmetaID;

    src = worddata;

    /* Jump over tfrequency and get first metaID */
    tfrequency = uncompress2(&src);     /* tfrequency */
    metaID = uncompress2(&src);     /* metaID */
    dst = src;

    while(1)
    {
        /* Get offset to next one */
        nextposmetaID = UNPACKLONG2(src);
        src += sizeof(long);

        /* Compute data length for this metaID */
        data_len = (int)nextposmetaID - (src - worddata);

        /* Store data_len as a compressed number */
        dst = compress3(data_len,dst);

        /* This must not happen. Anyway check it */
        if(dst > src)
            progerr("Internal error in remove_worddata_longs");

        /* dst may be smaller than src. So move the data */
        memcpy(dst,src,data_len);

        /* Increase pointers */
        src += data_len;
        dst += data_len;

        /* Check if we are at the end of the buffer */
        if ((src - worddata) == *sz_worddata)
            break;   /* End of worddata */

        /* Get next metaID */
        metaID = uncompress2(&src);
        dst = compress3(metaID,dst);
    }
    /* Adjust to new size */
    *sz_worddata = dst - worddata;
}
