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
** use _AP() for easier cross-compiler (non-ANSI) porting 
** <return value> <functionname> _AP( (<arg prototypes>) );
*/


int isdirectory _AP ((char *));
int isfile _AP ((char *));
int islink _AP ((char *));
int getsize _AP ((char *));
void getdefaults _AP ((SWISH *, char *, int *, int *, int));
void checkReplaceList _AP ((SWISH *));
void checkListRegex _AP ((struct swline *list));

/*
 * Some handy routines for parsing the Configuration File
 */

int grabYesNoField _AP ((char* line, char* commandTag, int* yesNoValue));
char *grabStringValueField _AP ((char* line, char* commandTag));
int grabIntValueField _AP ((char* line, char* commandTag, int* singleValue, int dontToIt));
int grabCmdOptionsMega _AP ((char* line, char* commandTag, struct swline **listOfWords, int* gotAny, int dontToIt));
int grabCmdOptions _AP ((char* line, char* commandTag, struct swline **listOfWords));
int grabCmdOptionsIndexFILE _AP ((char* line, char* commandTag, IndexFILE **listOfWords, int* gotAny, int dontToIt));
void readstopwordsfile _AP ((SWISH *, IndexFILE *, char *));
void readusewordsfile _AP ((SWISH *, IndexFILE *, char *));

/* use these to open Index files (because they are binary files: Win32)  */
FILE* openIndexFILEForWrite _AP ((char *));
FILE* openIndexFILEForRead _AP ((char *));
FILE* openIndexFILEForReadAndWrite _AP ((char *));
void CreateEmptyFile _AP ((SWISH *, char *));

void indexpath _AP((SWISH *, char *));
int parseconfline _AP((SWISH *, char *));


char *read_stream _AP((FILE *, int ));

/* Get/eval properties for file  (2000-11 rasc) */
FileProp *file_properties (char *real_path, char *work_path, SWISH *sw);
void     free_file_properties (FileProp *fprop);
