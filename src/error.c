/*
$id:$
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
** 2001-02-12 rasc   rewritten (progerr uses vargs, now)
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "error.h"



/*
  -- print programm error message  (like printf)
  -- exit (1)
  -- 2001-02-12 rasc   rewritten to use va_args,
  --                     get rid of BuildErrorString
*/


void progerr(char *msgfmt,...)
{
  va_list args;

  va_start (args,msgfmt);
  fprintf  (stdout, "err: ");
  vfprintf (stdout, msgfmt, args);
  fprintf  (stdout, "\n.\n");
  va_end   (args);
  exit(1);
 }



/* See errors.h to the correspondant numerical value */
static char *swishErrors[]={
"",                                                          /* RC_OK */
"Index file not found",          /*INDEX_FILE_NOT_FOUND */
"Unknown index file format",          /* UNKNOWN_INDEX_FILE_FORMAT */
"No words in search",          /* NO_WORDS_IN_SEARCH */
"Words too common",          /* WORDS_TOO_COMMON */
"Index file is empty",          /* INDEX_FILE_IS_EMPTY */
"Unknown property name in display properties", /* UNKNOWN_PROPERTY_NAME_IN_SEARCH_DISPLAY */
"Unknown property name to sort by",          /* UNKNOWN_PROPERTY_NAME_IN_SEARCH_SORT */
"Unknown metaname",          /* UNKNOWN_METANAME */
"Single wildcard not allowed as word", /* UNIQUE_WILDCARD_NOT_ALLOWED_IN_WORD */
"Word not found",          /* WORD_NOT_FOUND */
"No more results",          /* SWISH_LISTRESULTS_EOF */
"Invalid swish handle",          /* INVALID_SWISH_HANDLE */
NULL};



char *getErrorString(int number)
{
int i;
	number=abs(number);
	/* To avoid buffer overruns lets count the strings */
	for(i=0;swishErrors[i];i++);
	if(number>=i) return(swishErrors[0]);

	return(swishErrors[number]);
}
