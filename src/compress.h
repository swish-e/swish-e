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

void compress1(int num, FILE *fp, int (*f_putc)(int , FILE *));
// unsigned char *compress2(int num, unsigned char *buffer);
unsigned char *compress3(int num, unsigned char *buffer);

int uncompress1(FILE *fp, int (*f_getc)(FILE *fp));
int uncompress2(unsigned char **buffer);


unsigned long PACKLONG(unsigned long num);
void PACKLONG2(unsigned long num, unsigned char *buffer);

unsigned long UNPACKLONG(unsigned long num);
unsigned long UNPACKLONG2(unsigned char *buffer);

unsigned char *compress_location(SWISH *,IndexFILE *, LOCATION *);
void compress_location_values(unsigned char **buf,unsigned char **flagp,int filenum,int frequency, int *position);
void compress_location_positions(unsigned char **buf,unsigned char *flag,int frequency, int *position);

LOCATION *uncompress_location(SWISH *,IndexFILE *,unsigned char *);
void uncompress_location_values(unsigned char **buf,unsigned char *flag, int *filenum,int *frequency);
void uncompress_location_positions(unsigned char **buf, unsigned char flag, int frequency, int *position);

void CompressCurrentLocEntry(SWISH *,IndexFILE *,ENTRY *);

long SwapLocData(SWISH *,ENTRY *,unsigned char *,int);
unsigned char *unSwapLocData(SWISH *,ENTRY *,long);
void unSwapLocDataEntry(SWISH *,ENTRY *);


#define MAXINTCOMPSIZE 5

