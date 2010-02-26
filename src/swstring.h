/*
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**


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
    
** Mon May  9 18:15:43 CDT 2005
** added GPL

**
** 2001-02-22 rasc   fixed macros (unsigned char)
*/

#ifndef STRING_H
#define STRING_H 1

#define CASE_SENSITIVE_ON 1
#define CASE_SENSITIVE_OFF 0

char *lstrstr (char *, char *);
char *getconfvalue (char *, char *);
int isoksuffix (char *filename, struct swline *rulelist);
char *replace (char *, char *, char *);

char *SafeStrCopy (char *,char *, int *);
void sortstring (char *);
char *mergestrings (char *,char *);


void makelookuptable (char * ,int *);
void makeallstringlookuptables (SWISH *);
/* 06/00 Jose Ruiz
** Macros iswordchar, isvowel
*/
#define iswordchar(header,c) header.wordcharslookuptable[tolower((unsigned char)(c))]
#define isvowel(sw,c) sw->isvowellookuptable[tolower((unsigned char)(c))]
/* #define isindexchar(header,c) header.indexcharslookuptable[c] indexchars stuff removed */

/* Functions for comparing integers for qsort */
int icomp2 (const void *,const void *);

/* 06/00 Jose Ruiz 
** Function to parse a line into a StringList
*/
StringList *parse_line (char *);

/* 06/00
** Function to free memory used by a StringList
*/
void freeStringList (StringList *);


int isnumstring (unsigned char*);
void remove_newlines (char*);
void remove_tags (char*);

unsigned char *bin2string(unsigned char *,int);

char *strtolower (char *str);
#define makeItLow(a)    strtolower ((a)) /* map old name to new $$$ */

char *str_skip_ws (char *s);
void str_trim_ws(char *string);
char charDecode_C_Escape (char *s, char **se);

/* ISO-Routines */

unsigned char char_ISO_normalize (unsigned char c);
char *str_ISO_normalize (char *s);

unsigned char *StringListToString(StringList *sl,int n);

int BuildTranslateChars (int trlookup[], unsigned char *from, unsigned char *to);
unsigned char *TranslateChars (int trlookup[], unsigned char *s);

char *str_basename (char *path);
char *cstr_basename (char *path);
char *cstr_dirname (char *path);


char *estrdup (char *str);
char *estrndup (char *str, size_t n);
char *estrredup (char *s1, char *s2);

const char *comma_long( SWUINT_T u );

/* Make life easy for now */
#include "swregex.h"


#endif /* STRING_H */
