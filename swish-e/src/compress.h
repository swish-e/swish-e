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

/* Jose Ruiz 04/00
** Now these functions are macros for better performance
*/
#define compress1(num,fp) {register int _i,_r; unsigned char _s[5]; _i=0;_r=num; while(_r) {_s[_i++] = _r & 127;_r >>= 7;} while(--_i >=0) fputc(_s[_i] | (_i ? 128 : 0), fp);}

/* same function but this works with a memory buffer instead of file output */
#define compress2(num,buffer) {register int _i=num;while(_i) {*buffer = _i & 127; if(_i != num) *buffer|=128;_i >>= 7;buffer--;}}

/* same function but this works with a memory buffer instead of file output */
/* and also increases the memory pointer */
#define compress3(num,buffer) {register int _i,_r; unsigned char _s[5]; _i=0;_r=num; while(_r) {_s[_i++] = _r & 127;_r >>= 7;} while(--_i >=0) *buffer++=(_s[_i] | (_i ? 128 : 0));}

#define uncompress1(num,fp) {register int _c;num = 0; do{ _c=(int)fgetc(fp); num <<= 7; num |= _c & 127; if(!num) break; } while (_c & 128);}

/* same function but this works with a memory forward buffer instead of file */
/* output and also increases the memory pointer */
#define uncompress2(num,buffer) {register int _c;num = 0; do{ _c=(int)((unsigned char)*buffer++); num <<= 7; num |= _c & 127; if(!num) break; } while (_c & 128);}

/* Macros to make long integers portable */
#define PACKLONG(num) \
{ unsigned long _i=0L; unsigned char *_s; \
if(num) { _s=(unsigned char *) &_i; _s[0]=((num >> 24) & 0xFF); _s[1]=((num >> 16) & 0xFF); _s[2]=((num >> 8) & 0xFF); _s[3]=(num & 0xFF); } \
(num)=_i; \
}

/* Same macro - Packs long in buffer */
#define PACKLONG2(num,buffer) (buffer)[0]=((num >> 24) & 0xFF); (buffer)[1]=((num >> 16) & 0xFF); (buffer)[2]=((num >> 8) & 0xFF); (buffer)[3]=(num & 0xFF)


#define UNPACKLONG(num) \
{ unsigned long _i; unsigned char *_s; \
_i=(num);_s=(unsigned char *) &_i; \
(num)=(_s[0]<<24)+(_s[1]<<16)+(_s[2]<<8)+_s[3]; \
}

/* Same macro - UnPacks long from buffer */
#define UNPACKLONG2(num,buffer) (num)=((buffer)[0]<<24)+((buffer)[1]<<16)+((buffer)[2]<<8)+(buffer)[3]

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
