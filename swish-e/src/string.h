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

/* 06/00 Jose Ruiz
** Structure  StringList. Stores words up to a number of n
*/
typedef struct  {
        int n;
        char **word;
} StringList;

char *lstrstr _AP ((char *, char *));
char *getword _AP ((char *, int *skiplen));
char *getconfvalue _AP ((char *, char *));
int isoksuffix _AP ((char *filename, struct swline *rulelist));
char *replace _AP ((char *, char *, char *));
void makeItLow _AP ((char *str));
int matchARegex _AP ((char *str, char *pattern));
char *SafeStrCopy _AP ((char *,char *, int *));
void sortstring _AP ((char *));
char *mergestrings _AP ((char *,char *));

char *matchAndChange  _AP ((char *str, char *pattern, char *subs));
char *replaceWild _AP ((char* fileName, char* pattern, char* subs));

void makelookuptable _AP ((char * ,int *));
void makeallstringlookuptables _AP ((SWISH *));
/* 06/00 Jose Ruiz
** Macros iswordchar, isvowel
*/
#define iswordchar(header,c) header.wordcharslookuptable[tolower(c)]
#define isvowel(sw,c) sw->isvowellookuptable[tolower(c)]

/* Functions for comparing integers for qsort */
int icomp _AP ((const void *,const void *));
int icomp2 _AP ((const void *,const void *));

/* 06/00 Jose Ruiz 
** Function to parse a line into a StringList
*/
StringList *parse_line _AP ((char *));

/* 06/00
** Function to free memory used by a StringList
*/
void freeStringList _AP ((StringList *));

char *parsetag _AP ((char *, char *, int ));

int isnumstring _AP ((unsigned char*));
void remove_newlines _AP ((char*));
void remove_tags _AP ((char*));

#ifdef _WIN32
#define strncasecmp	strnicmp
#endif

