/*
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
**
** 2001-06-21 jmruiz ramdisk module or "poor man ramdisk" - Initial coding
**                   to test with LOCATIONS but it can be used with other
**                   things.
**                   Writes: Only sequential write is allowed
**                   Reads: Direct and sequential reads are allowed
**                   Routines and its equivalents:
**                   
**                         ramdisk_create                         fopen
**                         ramdisk_tell_write, ramdisk_tell_read  ftell
**                         ramdisk_write                          fwrite
**                         ramdisk_read                           fwrite
**                         ramdisk_seek_read                      fseek
**                         ramdisk_close                          fclose
**
**
*/


#include "swish.h"
#include "mem.h"
#include "ramdisk.h"

/* 06/2001 Jose Ruiz
** Routines to store/extract location data in memory incontigous positions to sa
ve
** memory.
** The data has been compressed previously in memory.
** Returns the pointer to the memory.
*/

struct ramdisk
{
   unsigned long cur_write_pos;
   unsigned long cur_read_pos;
   unsigned int n_buffers;
   unsigned int buf_size;
   unsigned char **buffer;
};


/* Create a new ramdisk - The size of the internal buffers is buf_size */
struct ramdisk *ramdisk_create(int buf_size)
{
struct ramdisk *rd;
        rd = (struct ramdisk *) emalloc(sizeof(struct ramdisk));
        rd->cur_write_pos = 0;
        rd->cur_read_pos = 0;
        rd->n_buffers = 1;
        rd->buf_size = buf_size;
        rd->buffer = (unsigned char **)emalloc(sizeof(unsigned char *));
        rd->buffer[0] = (unsigned char *)emalloc(buf_size);
        return rd;
}

/* Closes / frees the memory used by the ramdisk */
int ramdisk_close(FILE *fp)
{
struct ramdisk *rd = (struct ramdisk *)fp;
unsigned int i;
    for(i=0;i<rd->n_buffers;i++)
        efree(rd->buffer[i]);
    efree(rd);
	return 0;
}

void add_buffer_ramdisk(struct ramdisk *rd)
{
        rd->buffer = (unsigned char **)erealloc(rd->buffer,(rd->n_buffers + 1) * sizeof(unsigned char *));
        rd->buffer[rd->n_buffers++] = (unsigned char *)emalloc(rd->buf_size);
}

/* Equivalent to ftell to get the position while writing to the ramdisk */
long ramdisk_tell_write(FILE *fp)
{
struct ramdisk *rd = (struct ramdisk *)fp;
    return rd->cur_write_pos;
}

/* Equivalent to ftell to get the position while reading from the ramdisk */
long ramdisk_tell_read(FILE *fp)
{
struct ramdisk *rd = (struct ramdisk *)fp;
    return rd->cur_read_pos;
}

/* Writes to the ramdisk - The parameters are identical to those in fwrite */
size_t ramdisk_write(const void *buffer,size_t sz1, size_t sz2, FILE *fp)
{
unsigned int lenbuf=(unsigned int)(sz1 *sz2);
struct ramdisk *rd = (struct ramdisk *)fp;
unsigned char *buf = (unsigned char *)buffer;
unsigned int num_buffer,start_pos,tmplenbuf = lenbuf;
unsigned int avail;

    num_buffer = rd->cur_write_pos / rd->buf_size;
    start_pos = rd->cur_write_pos % rd->buf_size;

    avail = rd->buf_size - start_pos;
    while(avail<=(unsigned int)lenbuf)
    {
        if(avail)
            memcpy(rd->buffer[num_buffer]+start_pos,buf,avail);
        lenbuf -= avail;
        rd->cur_write_pos += avail;
		buf += avail;
        add_buffer_ramdisk(rd);
        avail = rd->buf_size;
        start_pos = 0;
        num_buffer++;
    }
    if(lenbuf)
    {
        memcpy(rd->buffer[num_buffer]+start_pos,buf,lenbuf);
        rd->cur_write_pos += lenbuf;
    }
    return (int) tmplenbuf;
}

/* Equivalent to fseek while reading */
int ramdisk_seek_read(FILE *fp,long pos, int set)
{
struct ramdisk *rd = (struct ramdisk *)fp;
    rd->cur_read_pos = pos;
    return pos;
}

/* Reads from the ramdisk - The parameters are identical to those in fread */
size_t ramdisk_read(void *buf, size_t sz1, size_t sz2, FILE *fp)
{
struct ramdisk *rd = (struct ramdisk *)fp;
unsigned int len = (unsigned int) (sz1 *sz2);
unsigned char *buffer = (unsigned char *)buf;
unsigned int avail, num_buffer, start_pos, buffer_offset;

    num_buffer = rd->cur_read_pos / rd->buf_size;
    start_pos = rd->cur_read_pos % rd->buf_size;

    buffer_offset = 0;

    avail = rd->buf_size - start_pos;
    while(avail < (unsigned int)len)
    {
        memcpy(buffer+buffer_offset,rd->buffer[num_buffer]+start_pos,avail);
        buffer_offset += avail;
        rd->cur_read_pos += avail;
        len -= avail;
        num_buffer++;
        start_pos=0;
		avail = rd->buf_size;
        if(num_buffer == rd->n_buffers)
            return buffer_offset;
    }
    memcpy(buffer+buffer_offset,rd->buffer[num_buffer]+start_pos,len);
    rd->cur_read_pos += len;
    buffer_offset += len;
    return buffer_offset;
}

int ramdisk_getc(FILE *fp)
{
unsigned char c;
    ramdisk_read((void *)&c, 1, 1, fp);
    return (int) ((unsigned char)c);
}

int ramdisk_putc(int c, FILE *fp)
{
unsigned char tmp = (unsigned char)c;
    ramdisk_write((const void *)&tmp,1, 1, fp);
    return 1;
}
