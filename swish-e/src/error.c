/*
$Id$
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

#include "swish.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "error.h"
#include <errno.h>


/*
  -- print program error message  (like printf)
  -- exit (1)
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


/*
  -- print program error message  (like printf)
  -- includes text of errno at end of message
  -- exit (1)
*/

void progerrno(char *msgfmt,...)
{
  va_list args;

  va_start (args,msgfmt);
  fprintf  (stdout, "err: ");
  vfprintf (stdout, msgfmt, args);
  fprintf  (stdout, "%s", strerror(errno));
  fprintf  (stdout, "\n.\n");
  va_end   (args);
  exit(1);
 }

/********** These are an attempt to prevent aborting in the library *********/

void set_progerr(int errornum, SWISH *sw, char *msgfmt,...)
{
  va_list args;

  sw->lasterror = errornum;

  va_start (args,msgfmt);
  vsnprintf (sw->lasterrorstr, MAX_ERROR_STRING_LEN, msgfmt, args);
  va_end   (args);
}



void set_progerrno(int errornum, SWISH *sw, char *msgfmt,...)
{
  va_list args;
  char *errstr = strerror(errno);

  sw->lasterror = errornum;

  va_start (args,msgfmt);
  vsnprintf (sw->lasterrorstr, MAX_ERROR_STRING_LEN - strlen( errstr ), msgfmt, args);
  strcat( sw->lasterrorstr, errstr );
  va_end   (args);
}
 


/* only print a warning (also to stdout) and return */
/* might want to have an enum level WARN_INFO, WARN_ERROR, WARN_CRIT, WARN_DEBUG */
void progwarn(char *msgfmt,...)
{
  va_list args;

  va_start (args,msgfmt);
  fprintf  (stdout, "\nWarning: ");
  vfprintf (stdout, msgfmt, args);
  fprintf  (stdout, "\n");
  va_end   (args);
 }

/* only print a warning (also to stdout) and return */
/* might want to have an enum level WARN_INFO, WARN_ERROR, WARN_CRIT, WARN_DEBUG */
/* includes text of errno at end of message */ 
void progwarnno(char *msgfmt,...)
{
  va_list args;

  va_start (args,msgfmt);
  fprintf  (stdout, "\nWarning: ");
  vfprintf (stdout, msgfmt, args);
  fprintf  (stdout, "%s", strerror(errno));
  fprintf  (stdout, "\n");
  va_end   (args);
 }



/* See errors.h to the correspondant numerical value */
static char *swishErrors[]={
"",                                                         /* RC_OK */
"Could not open index file",                                /*INDEX_FILE_NOT_FOUND */
"Unknown index file format",                                /* UNKNOWN_INDEX_FILE_FORMAT */
"No search words specified",                                /* NO_WORDS_IN_SEARCH */
"All search words too common to be useful",                 /* WORDS_TOO_COMMON */
"Index file(s) is empty",                                   /* INDEX_FILE_IS_EMPTY */
"Unknown property name in display properties",              /* UNKNOWN_PROPERTY_NAME_IN_SEARCH_DISPLAY */
"Unknown property name to sort by",                         /* UNKNOWN_PROPERTY_NAME_IN_SEARCH_SORT */
"Unknown metaname",                                         /* UNKNOWN_METANAME */
"Single wildcard not allowed as word",                      /* UNIQUE_WILDCARD_NOT_ALLOWED_IN_WORD */
"Word not found",                                           /* WORD_NOT_FOUND */
"No more results",                                          /* SWISH_LISTRESULTS_EOF */
"Invalid swish handle",                                     /* INVALID_SWISH_HANDLE */
"Search word exceeded maxwordlimit setting",                /* SEARCH_WORD_TOO_BIG */
"Syntax error in query (missing end quote or unbalanced parenthesis?)",	/* QUERY_SYNTAX_ERROR */
NULL};



char *getErrorString(int number)
{
int i;
	number=abs(number);
	/* To avoid buffer overruns lets count the strings */
	for(i=0;swishErrors[i];i++);
	if ( number>=i ) return( "Unknown Error Number" );

	return(swishErrors[number]);
}
