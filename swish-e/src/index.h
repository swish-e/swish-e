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


void do_index_file (SWISH *sw, FileProp *fprop);

void printMetaNames (IndexFILE *);
DOCENTRYARRAY *addsortentry (DOCENTRYARRAY *, char*);
void addentry (SWISH *, char*, int, int, int, int );
void addtofilelist (SWISH *,IndexFILE *indexf, char *filename, time_t mtime, char *title, char *summary, int start, int size, struct file ** newFileEntry);
int getfilecount (IndexFILE *);
char *getthedate(void);
int countwordstr (SWISH *, char *, int);
int parsecomment (SWISH *, char *, int, int, int, int *);
int removestops (SWISH *, int);
int getrank(SWISH *, int, int, int, int);
void printheader(INDEXDATAHEADER *, FILE *, char *, int, int, int);
void printindex(SWISH *, IndexFILE *);
void printword(SWISH *, ENTRY *, IndexFILE *);
void printworddata(SWISH *, ENTRY *, IndexFILE *);
void printhash(ENTRY **, IndexFILE *);
void printfilelist(SWISH *, IndexFILE *);
void printstopwords(IndexFILE *);
void printfileoffsets(IndexFILE *);
void printlocationlookuptables(IndexFILE *);
void printpathlookuptable(IndexFILE *);
void decompress(SWISH *, IndexFILE *);
char *ruleparse(SWISH *, char *);
void stripIgnoreFirstChars(INDEXDATAHEADER, char *);
int stripIgnoreLastChars(INDEXDATAHEADER, char *);

#define isIgnoreFirstChar(header,c) header.ignorefirstcharlookuptable[(int)((unsigned char)c)]
#define isIgnoreLastChar(header,c) header.ignorelastcharlookuptable[(int)((unsigned char)c)]
#define isBumpPositionCounterChar(header,c) header.bumpposcharslookuptable[(int)((unsigned char)c)]

unsigned char *buildFileEntry(char *, FILE *, struct docPropertyEntry **, int, int *);
struct file *readFileEntry(IndexFILE *,int);

void computehashentry(ENTRY **,ENTRY *);

void sortentry(SWISH *, IndexFILE *, ENTRY *);

int indexstring(SWISH*, char *, int, int, int, int *, int *);

void addtofwordtotals(IndexFILE *, int, int);
void addsummarytofile(IndexFILE *, int, char *);

void BuildSortedArrayOfWords(SWISH *,IndexFILE *);

