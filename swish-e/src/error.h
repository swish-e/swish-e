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
** 2001-02-12 rasc   some parts rewritten (progerr uses vargs, now)
*/


#ifndef __HasSeenModule_Error
#define __HasSeenModule_Error		1


void set_error_handle( FILE *where );
void SwishErrorsToStderr( void );

void progerr (char *msgfmt, ...);
void progerrno (char *msgfmt, ...);

void set_progerr(int errornum, SWISH *sw, char *msgfmt,...);
void set_progerrno(int errornum, SWISH *sw, char *msgfmt,...);


void progwarn (char *msgfmt, ...);
void progwarnno (char *msgfmt, ...);


char *getErrorString(int);
int  SwishError(SWISH * sw);
char *SwishErrorString(SWISH *sw);
char *SwishLastErrorMsg(SWISH *sw);
void SwishAbortLastError(SWISH *sw);
int SwishCriticalError(SWISH *sw);
void reset_lasterror(SWISH *sw);


char *fuzzy_mode_to_string( FuzzyIndexType mode );
FuzzyIndexType set_fuzzy_mode( char *param );


#define RC_OK 0

enum {
    INDEX_FILE_NOT_FOUND = -255,
    UNKNOWN_INDEX_FILE_FORMAT,
    NO_WORDS_IN_SEARCH,
    WORDS_TOO_COMMON,
    INDEX_FILE_IS_EMPTY,
    INDEX_FILE_ERROR,
    UNKNOWN_PROPERTY_NAME_IN_SEARCH_DISPLAY,
    UNKNOWN_PROPERTY_NAME_IN_SEARCH_SORT,
    INVALID_PROPERTY_TYPE,
    UNKNOWN_METANAME,
    UNIQUE_WILDCARD_NOT_ALLOWED_IN_WORD,
    WORD_NOT_FOUND,
    SWISH_LISTRESULTS_EOF,
    HEADER_READ_ERROR,
    INVALID_SWISH_HANDLE,
    INVALID_RESULTS_HANDLE,
    SEARCH_WORD_TOO_BIG,
    QUERY_SYNTAX_ERROR,
    PROP_LIMIT_ERROR
};    
#endif

