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
    
** Mon May  9 14:52:21 CDT 2005
** added GPL


**
**
** 2001-06-21 jmruiz ramdisk module or "poor man ramdisk" - Initial coding
**                   to test with LOCATIONS but it can be used with other
**                   things.
**                   Writes: Only sequential write is allowed
**                   Reads: Direct and sequential reads are allowed
**                   Routines and its equivalents:
**                   
**                         ramdisk_create                         fopen
**                         ramdisk_tell                           ftell
**                         ramdisk_write                          fwrite
**                         ramdisk_read                           fwrite
**                         ramdisk_seek                           fseek
**                         ramdisk_close                          fclose
**
**
*/


#include "swish.h"
#include "mem.h"
#include "ramdisk.h"

/* 06/2001 Jose Ruiz
** Routines to store/extract location data in memory incontigous positions to save
** memory.
** The data has been compressed previously in memory.
** Returns the pointer to the memory.
*/

struct ramdisk
{
   sw_off_t cur_pos;
   sw_off_t end_pos;
   SWUINT_T n_buffers;
   SWUINT_T buf_size;
   unsigned char **buffer;
   MEM_ZONE *zone;
};


/* Create a new ramdisk - The size of the internal buffers is buf_size */
struct ramdisk *ramdisk_create(char *name, int buf_size)        // no rw64
{
struct ramdisk *rd;

        rd = (struct ramdisk *) emalloc(sizeof(struct ramdisk));
        rd->zone = Mem_ZoneCreate(name, buf_size, 0);
        rd->cur_pos = (sw_off_t)0;
        rd->end_pos = (sw_off_t)0;
        rd->n_buffers = 1;
        rd->buf_size = buf_size;
        rd->buffer = (unsigned char **)emalloc(sizeof(unsigned char *));
        rd->buffer[0] = (unsigned char *)Mem_ZoneAlloc(rd->zone, buf_size);
        return rd;
}

/* Closes / frees the memory used by the ramdisk */
int ramdisk_close(FILE *fp)                      // no rw64
{
struct ramdisk *rd = (struct ramdisk *)fp;

        Mem_ZoneFree(&rd->zone);
        efree(rd->buffer);
        efree(rd);
    return 0;
}

void add_buffer_ramdisk(struct ramdisk *rd)
{
        rd->buffer = (unsigned char **)erealloc(rd->buffer,(rd->n_buffers + 1) * sizeof(unsigned char *));
        rd->buffer[rd->n_buffers++] = (unsigned char *)Mem_ZoneAlloc(rd->zone, rd->buf_size);
}

/* Equivalent to ftell to get the position while writing to the ramdisk */
sw_off_t ramdisk_tell(FILE *fp)
{
struct ramdisk *rd = (struct ramdisk *)fp;

    return rd->cur_pos;
}


/* Writes to the ramdisk - The parameters are identical to those in fwrite */
size_t ramdisk_write(const void *buffer,size_t sz1, size_t sz2, FILE *fp)
{
SWUINT_T lenbuf=(SWUINT_T)(sz1 *sz2);
struct ramdisk *rd = (struct ramdisk *)fp;
unsigned char *buf = (unsigned char *)buffer;
SWUINT_T num_buffer,start_pos;
SWUINT_T avail;

    num_buffer = (SWUINT_T)(rd->cur_pos / (sw_off_t)rd->buf_size);
    start_pos = (SWUINT_T)(rd->cur_pos % (sw_off_t)rd->buf_size);

    avail = rd->buf_size - start_pos;
    while(avail<=(SWUINT_T)lenbuf)
    {
        if(avail)
            memcpy(rd->buffer[num_buffer]+start_pos,buf,avail);
        lenbuf -= avail;
        rd->cur_pos += (sw_off_t)avail;
        buf += avail;
        add_buffer_ramdisk(rd);
        avail = rd->buf_size;
        start_pos = 0;
        num_buffer++;
    }
    if(lenbuf)
    {
        memcpy(rd->buffer[num_buffer]+start_pos,buf,lenbuf);
        rd->cur_pos += (sw_off_t)lenbuf;
    }
    if(rd->cur_pos > rd->end_pos)
        rd->end_pos = rd->cur_pos;

    /* needs to return number of elements, not number of bytes */
    return sz2;
}

/* Equivalent to fseek */
int ramdisk_seek(FILE *fp,sw_off_t pos, int set)     // no rw64
{
struct ramdisk *rd = (struct ramdisk *)fp;

    switch(set)
    {
    case SEEK_CUR:
        pos += rd->cur_pos;
        break;
    case SEEK_END:
        pos += rd->end_pos;
        break;
    }
    if( pos > rd->end_pos )
    {
        while(rd->end_pos < pos)
        {
            ramdisk_putc(0, (FILE *)rd);
        }
    } 
    else
    {
        rd->cur_pos = pos;
    }
    return 0;
}


/* Reads from the ramdisk - The parameters are identical to those in fread */
size_t ramdisk_read(void *buf, size_t sz1, size_t sz2, FILE *fp)
{
struct ramdisk *rd = (struct ramdisk *)fp;
SWUINT_T len = (SWUINT_T) (sz1 * sz2);
unsigned char *buffer = (unsigned char *)buf;
SWUINT_T avail, num_buffer, start_pos, buffer_offset;

    if(rd->cur_pos >= rd->end_pos)
    return 0;
    if((rd->cur_pos + (sw_off_t)len) > rd->end_pos)
    {
        len = (SWUINT_T)(rd->end_pos - rd->cur_pos);
    }
    num_buffer = (SWUINT_T)(rd->cur_pos / (sw_off_t)rd->buf_size);
    start_pos = (SWUINT_T)(rd->cur_pos % (sw_off_t)rd->buf_size);

    buffer_offset = 0;

    avail = rd->buf_size - start_pos;
    while(avail < (SWUINT_T)len)
    {
        memcpy(buffer+buffer_offset,rd->buffer[num_buffer]+start_pos,avail);
        buffer_offset += avail;
        rd->cur_pos += (sw_off_t)avail;
        len -= avail;
        num_buffer++;
        start_pos=0;
        avail = rd->buf_size;
        if(num_buffer == rd->n_buffers)
            return buffer_offset;
    }
    memcpy(buffer+buffer_offset,rd->buffer[num_buffer]+start_pos,len);
    rd->cur_pos += (sw_off_t)len;
    buffer_offset += len;
    return buffer_offset;
}

int ramdisk_getc(FILE *fp)   // no rw64
{
unsigned char c;

    ramdisk_read((void *)&c, 1, 1, fp);
    return (SWINT_T) ((unsigned char)c);
}

int ramdisk_putc(int c, FILE *fp)    // no rw64
{
unsigned char tmp = (unsigned char)c;

    ramdisk_write((const void *)&tmp,1, 1, fp);
    return 1;
}
