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

char *BuildErrorString _AP ((char *,int *,char *,char *));
void progerr _AP ((char *));

#define RC_OK 0
#define ERROR_BASE RC_OK

#define INDEX_FILE_NOT_FOUND ERROR_BASE-1
#define UNKNOWN_INDEX_FILE_FORMAT ERROR_BASE-2
#define NO_WORDS_IN_SEARCH ERROR_BASE-3
#define WORDS_TOO_COMMON ERROR_BASE-4
#define INDEX_FILE_IS_EMPTY ERROR_BASE-5
#define UNKNOWN_PROPERTY_NAME_IN_SEARCH_DISPLAY ERROR_BASE-6
#define UNKNOWN_PROPERTY_NAME_IN_SEARCH_SORT ERROR_BASE-7
#define UNKNOWN_METANAME ERROR_BASE-8
#define UNIQUE_WILDCARD_NOT_ALLOWED_IN_WORD ERROR_BASE-9
#define WORD_NOT_FOUND ERROR_BASE-10
#define SWISH_LISTRESULTS_EOF ERROR_BASE-11
#define INVALID_SWISH_HANDLE ERROR_BASE-12

void progerr(char *);
char *getErrorString(int);
