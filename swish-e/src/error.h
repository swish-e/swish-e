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
    
** Mon May  9 18:19:34 CDT 2005
** added GPL



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
    WILDCARD_NOT_ALLOWED_AT_WORD_START,
    WORD_NOT_FOUND,
    SWISH_LISTRESULTS_EOF,
    HEADER_READ_ERROR,
    INVALID_SWISH_HANDLE,
    INVALID_RESULTS_HANDLE,
    SEARCH_WORD_TOO_BIG,
    QUERY_SYNTAX_ERROR,
    PROP_LIMIT_ERROR,
    WILDCARD_NOT_ALLOWED_WITHIN_WORD
};    
#endif

