/*
$Id$
**
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


void do_index_file (SWISH *sw, FileProp *fprop);

DOCENTRYARRAY *addsortentry (DOCENTRYARRAY *, char*);
void addentry (SWISH *, char*, int, int, int, int );
void addtofilelist (SWISH *,IndexFILE *indexf, char *filename, time_t mtime, char *title, char *summary, int start, int size, struct file ** newFileEntry);
int getfilecount (IndexFILE *);
int countwordstr (SWISH *, char *, int);
int removestops (SWISH *);
int getrank(SWISH *, int, int, int, int, int);
void write_file_list(SWISH *, IndexFILE *);
void write_sorted_index(SWISH *, IndexFILE *);
void decompress(SWISH *, IndexFILE *);
char *ruleparse(SWISH *, char *);
void stripIgnoreFirstChars(INDEXDATAHEADER *, char *);
void stripIgnoreLastChars(INDEXDATAHEADER *, char *);

#define isIgnoreFirstChar(header,c) (header)->ignorefirstcharlookuptable[(int)((unsigned char)c)]
#define isIgnoreLastChar(header,c) (header)->ignorelastcharlookuptable[(int)((unsigned char)c)]
#define isBumpPositionCounterChar(header,c) (header)->bumpposcharslookuptable[(int)((unsigned char)c)]

unsigned char *buildFileEntry(char *, struct docPropertyEntry **, int, int *);
struct file *readFileEntry(SWISH *, IndexFILE *,int);

void computehashentry(ENTRY **,ENTRY *);

void sort_words(SWISH *, IndexFILE *);
void sortentry(SWISH *, IndexFILE *, ENTRY *);

int indexstring(SWISH*, char *, int, int, int, int *, int *);

void addtofwordtotals(IndexFILE *, int, int);
void addsummarytofile(IndexFILE *, int, char *);

void BuildSortedArrayOfWords(SWISH *,IndexFILE *);



void PrintHeaderLookupTable (int ID, int table[], int table_size, FILE *fp);

