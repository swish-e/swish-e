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
**
** 2001-02-22 rasc   fixed macros (unsigned char)
*/


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
void remove_controls (char*);
void remove_tags (char*);

unsigned char *bin2string(unsigned char *,int);

#ifdef _WIN32
#define strncasecmp	strnicmp
#endif




char *strtolower (char *str);
#define makeItLow(a)    strtolower ((a)) /* map old name to new $$$ */

char *str_skip_ws (char *s);
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

void split_path( unsigned char *path, unsigned char **directory, unsigned char **file );

char *estrdup (char *str);
char *estrndup (char *str, size_t n);
char *estrredup (char *s1, char *s2);


int match_regex_list( char *str, regex_list *regex );
char *process_regex_list( char *str, regex_list *regex, int *matched );

void free_regex_list( regex_list **reg_list );
void add_regular_expression( regex_list **reg_list, char *pattern, char *replace, int cflags, int global, int negate );

