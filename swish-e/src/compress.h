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

/*
** use _AP() for easier cross-compiler (non-ANSI) porting
** <return value> <functionname> _AP( (<arg prototypes>) );
*/



/* Jose Ruiz 04/00
** Now this function is a macro for better performance
** void compress _AP ((int, FILE *));
*/
#define compress1(num,fp) \
{int i,r; char s[5]; \
i=0;r=num; \
while(r) {s[i++] = r % 128;r /= 128;}\
while(--i >=0) fputc(s[i] | (i ? 128 : 0), fp);}

/* same function but this works with a memory buffer instead of file output */
#define compress2(data,buffer) {int num=data;while(num) {*buffer = num % 128; if(num!=data) *buffer|=128;num /= 128;buffer--;}}

/* same function but this works with a memory buffer instead of file output */
/* and also increases the memory pointer */
#define compress3(num,buffer) \
{int i,r; char s[5]; \
i=0;r=num; \
while(r) {s[i++] = r % 128;r /= 128;}\
while(--i >=0) *buffer++=(s[i] | (i ? 128 : 0));}

/* Jose Ruiz 04/00
** Now this function is a macro for better performance
** int uncompress _AP ((FILE *));
*/
#define uncompress1(num,fp) \
{int c;num = 0;\
do{ \
   c=(int)fgetc(fp);\
   num *= 128; num += c & 127;\
   if(!num) break;\
} while (c & 128);}

/* same function bur works with a memory buffer instead of file output */
#define uncompress2(num,buffer) {int c;num = 0;do{ c=(int)*buffer++;num*=128; num+= c & 127; if(!num)break;}while(c & 128);} 

/* same function but this works with a memory forward buffer instead of file */
/* output and also increases the memory pointer */
#define uncompress3(num,buffer) \
{int c;num = 0;\
do{ \
   c=(int)((unsigned char)*buffer++);\
   num *= 128; num += c & 127;\
   if(!num) break;\
} while (c & 128);}


unsigned char *compress_location _AP ((SWISH *,IndexFILE *, LOCATION *));
LOCATION *uncompress_location _AP ((SWISH *,IndexFILE *,unsigned char *));
void CompressPrevLocEntry _AP ((SWISH *,IndexFILE *,ENTRY *));
void CompressCurrentLocEntry _AP ((SWISH *,IndexFILE *,ENTRY *));
void SwapFileData _AP ((SWISH *,struct file *));
struct file *unSwapFileData _AP ((SWISH *));
long SwapLocData _AP ((SWISH *,unsigned char *,int));
unsigned char *unSwapLocData _AP ((SWISH *,long));
int get_lookup_index _AP ((struct int_lookup_st **,int ,int *));
int get_lookup_path _AP ((struct char_lookup_st **,char *));
