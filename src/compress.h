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
*/

void compress1(int num, FILE *fp);
unsigned char *compress2(int num, unsigned char *buffer);
unsigned char *compress3(int num, unsigned char *buffer);

int uncompress1(FILE *fp);
int uncompress2(unsigned char **buffer);

/* Macros to make long integers portable */
#define PACKLONG(num) \
{ unsigned long _i=0L; unsigned char *_s; \
if(num) { _s=(unsigned char *) &_i; _s[0]=((num >> 24) & 0xFF); _s[1]=((num >> 16) & 0xFF); _s[2]=((num >> 8) & 0xFF); _s[3]=(num & 0xFF); } \
(num)=_i; \
}

/* Same macro - Packs long in buffer */
#define PACKLONG2(num,buffer) (unsigned char)(buffer)[0]=((num >> 24) & 0xFF); (unsigned char)(buffer)[1]=((num >> 16) & 0xFF); (unsigned char)(buffer)[2]=((num >> 8) & 0xFF); (unsigned char)(buffer)[3]=(num & 0xFF)


#define UNPACKLONG(num) \
{ unsigned long _i; unsigned char *_s; \
_i=(num);_s=(unsigned char *) &_i; \
(num)=(_s[0]<<24)+(_s[1]<<16)+(_s[2]<<8)+_s[3]; \
}

/* Same macro - UnPacks long from buffer */
#define UNPACKLONG2(num,buffer) (num)=((unsigned char)(buffer)[0]<<24)+((unsigned char)(buffer)[1]<<16)+((unsigned char)(buffer)[2]<<8)+(unsigned char)(buffer)[3]

unsigned char *compress_location(SWISH *,IndexFILE *, LOCATION *);
LOCATION *uncompress_location(SWISH *,IndexFILE *,unsigned char *);
void CompressPrevLocEntry(SWISH *,IndexFILE *,ENTRY *);
void CompressCurrentLocEntry(SWISH *,IndexFILE *,ENTRY *);
void SwapFileData(SWISH *,struct file *);
struct file *unSwapFileData(SWISH *);
long SwapLocData(SWISH *,unsigned char *,int);
unsigned char *unSwapLocData(SWISH *,long);
int get_lookup_index(struct int_lookup_st **,int ,int *);
int get_lookup_path(struct char_lookup_st **,char *);
