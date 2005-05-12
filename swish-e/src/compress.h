/*
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**
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
    
** Mon May  9 18:19:34 CDT 2005
** added GPL


*/

int sizeofcompint(int number);
void compress1(int num, FILE *fp, int (*f_putc)(int , FILE *));
/* unsigned char *compress2(int num, unsigned char *buffer);*/
unsigned char *compress3(int num, unsigned char *buffer);

int uncompress1(FILE *fp, int (*f_getc)(FILE *fp));
int uncompress2(unsigned char **buffer);


unsigned long PACKLONG(unsigned long num);
void PACKLONG2(unsigned long num, unsigned char *buffer);

unsigned long UNPACKLONG(unsigned long num);
unsigned long UNPACKLONG2(unsigned char *buffer);


sw_off_t PACKFILEOFFSET(sw_off_t num);
void PACKFILEOFFSET2(sw_off_t num, unsigned char *buffer);

sw_off_t UNPACKFILEOFFSET(sw_off_t num);
sw_off_t UNPACKLONGFILEOFFSET2(unsigned char *buffer);

void compress_location_values(unsigned char **buf,unsigned char **flagp,int filenum,int frequency, unsigned int *posdata);
void compress_location_positions(unsigned char **buf,unsigned char *flag,int frequency, unsigned int *posdata);

void uncompress_location_values(unsigned char **buf,unsigned char *flag, int *filenum,int *frequency);
void uncompress_location_positions(unsigned char **buf, unsigned char flag, int frequency, unsigned int *posdata);

void CompressCurrentLocEntry(SWISH *, ENTRY *);

int compress_worddata(unsigned char *, int, int );
void uncompress_worddata(unsigned char **,int *, int);
void    remove_worddata_longs(unsigned char *,int *);

/* Here is the worst case size for a compressed number 
** MAXINTCOMPSIZE stands for MAXimum INTeger COMPressed SIZE
**
** There are many places in the code in which we allocate
** space for a compressed number. In the worst case this size is 5
** for 32 bit number, 10 for a 64 bit number.
**
** The way this compression works is reserving the first bit 
** in each byte to store a flag. The flag is set in all bytes
** except for the last one.
** This only gives 7 bits per byte to store the number.
**
** For example, to store 1000 (binary 1111101000) we will get:
** 
** 1st byte    2th byte
** 10000111    01101000
** ^           ^
** |           |
** |           Flag to indicate that this is tha last byte
** |
** Flag set to indicate that more bytes follow this one
**
** So, to compress a 32 bit number we need 5 bytes and for
** a 64 bit number we will use 10 bytes for the worst case
*/
#define MAXINTCOMPSIZE (((sizeof(int) * 8) / 7) + (((sizeof(int) * 8) % 7) ? 1 : 0))

