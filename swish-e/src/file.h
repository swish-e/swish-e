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
** added buffer size arg to grabStringValue prototype - core dumping from overrun
** SRE 2/22/00
*/

/*
** use) for easier cross-compiler (non-ANSI) porting 
** <return value> <functionname> (<arg prototypes>) );
*/


int isdirectory(char *);
int isfile(char *);
int islink(char *);
int getsize(char *);
void getdefaults(SWISH *, char *, int *, int *, int);
void checkReplaceList(SWISH *);
void checkListRegex(struct swline *list);

/*
 * Some handy routines for parsing the Configuration File
 */

void grabCmdOptions(StringList *sl, int start, struct swline **listOfWords);
int grabCmdOptionsIndexFILE(char* line, char* commandTag, IndexFILE **listOfWords, int* gotAny, int dontToIt);
void readstopwordsfile(SWISH *, IndexFILE *, char *);
void readusewordsfile(SWISH *, IndexFILE *, char *);

/* use these to open Index files (because they are binary files: Win32)  */
FILE* openIndexFILEForWrite(char *);
FILE* openIndexFILEForRead(char *);
FILE* openIndexFILEForReadAndWrite(char *);
void CreateEmptyFile(SWISH *, char *);

void indexpath(SWISH *, char *);
int parseconfline(SWISH *, StringList *);


char *read_stream(FILE *, int );

/* Get/eval properties for file  (2000-11 rasc) */
FileProp *file_properties (char *real_path, char *work_path, SWISH *sw);
void     free_file_properties (FileProp *fprop);
