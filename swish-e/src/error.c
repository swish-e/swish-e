/*
$Id$
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
    
    Swish-e includes a library for searching with a well-defined API. The library
    is named libswish-e.
    
    Linking libswish-e statically or dynamically with other modules is making a
    combined work based on Swish-e.  Thus, the terms and conditions of the GNU
    General Public License cover the whole combination.

    As a special exception, the copyright holders of Swish-e give you
    permission to link Swish-e with independent modules that communicate with
    Swish-e solely through the libswish-e API interface, regardless of the license
    terms of these independent modules, and to copy and distribute the
    resulting combined work under terms of your choice, provided that
    every copy of the combined work is accompanied by a complete copy of
    the source code of Swish-e (the version of Swish-e used to produce the
    combined work), being distributed under the terms of the GNU General
    Public License plus this exception.  An independent module is a module
    which is not derived from or based on Swish-e.

    Note that people who make modified versions of Swish-e are not obligated
    to grant this special exception for their modified versions; it is
    their choice whether to do so.  The GNU General Public License gives
    permission to release a modified version without this exception; this
    exception also makes it possible to release a modified version which
    carries forward this exception.
    
** Mon May  9 15:51:39 CDT 2005
** added GPL
** if this really is re-written in 2001, we should remove the HP copyright??


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


/* Allow overriding swish-e's old behavior of errors to stdout */

FILE *error_handle = NULL;

void set_error_handle( FILE *where )
{
    error_handle = where;
}

void SwishErrorsToStderr( void )
{
    error_handle = stderr;
}


void progerr(char *msgfmt,...)
{
  va_list args;

  if ( !error_handle )
      error_handle = stdout;

  va_start (args,msgfmt);
  fprintf  (error_handle, "err: ");
  vfprintf (error_handle, msgfmt, args);
  fprintf  (error_handle, "\n.\n");
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

  if ( !error_handle )
      error_handle = stdout;

  va_start (args,msgfmt);
  fprintf  (error_handle, "err: ");
  vfprintf (error_handle, msgfmt, args);
  fprintf  (error_handle, "%s", strerror(errno));
  fprintf  (error_handle, "\n.\n");
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

void reset_lasterror(SWISH *sw)
{
    sw->lasterror = RC_OK;
    sw->lasterrorstr[0] = '\0';
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
 


/* only print a warning (also to error_handle) and return */
/* might want to have an enum level WARN_INFO, WARN_ERROR, WARN_CRIT, WARN_DEBUG */
void progwarn(char *msgfmt,...)
{
  va_list args;

  if ( !error_handle )
      error_handle = stdout;


  va_start (args,msgfmt);
  fprintf  (error_handle, "\nWarning: ");
  vfprintf (error_handle, msgfmt, args);
  fprintf  (error_handle, "\n");
  va_end   (args);
 }

/* only print a warning (also to error_handle) and return */
/* might want to have an enum level WARN_INFO, WARN_ERROR, WARN_CRIT, WARN_DEBUG */
/* includes text of errno at end of message */ 
void progwarnno(char *msgfmt,...)
{
  va_list args;

  if ( !error_handle )
      error_handle = stdout;

  va_start (args,msgfmt);
  fprintf  (error_handle, "\nWarning: ");
  vfprintf (error_handle, msgfmt, args);
  fprintf  (error_handle, "%s", strerror(errno));
  fprintf  (error_handle, "\n");
  va_end   (args);
 }

typedef struct
{
    int     critical;   /* If true the calling code needs to call SwishClose */
    int     error_num;
    char    *message_string;
} error_msg_map;

/* See errors.h to the correspondant numerical value */
static error_msg_map swishErrors[]={
    { 0, RC_OK,                                    "" },
    { 0, NO_WORDS_IN_SEARCH,                       "No search words specified" },
    { 0, WORDS_TOO_COMMON,                         "All search words too common to be useful" },
    { 0, UNKNOWN_PROPERTY_NAME_IN_SEARCH_DISPLAY,  "Unknown property name in display properties" },
    { 0, UNKNOWN_PROPERTY_NAME_IN_SEARCH_SORT,     "Unknown property name to sort by" },
    { 0, INVALID_PROPERTY_TYPE,                    "Invalid property type" },
    { 0, UNKNOWN_METANAME,                         "Unknown metaname" },
    { 0, UNIQUE_WILDCARD_NOT_ALLOWED_IN_WORD,      "Single wildcard not allowed as word" },
    { 0, WILDCARD_NOT_ALLOWED_WITHIN_WORD,         "Wildcard not allowed within a word" },
    { 0, WORD_NOT_FOUND,                           "Word not found" },
    { 0, SEARCH_WORD_TOO_BIG,                      "Search word exceeded maxwordlimit setting" },
    { 0, QUERY_SYNTAX_ERROR,                       "Syntax error in query (missing end quote or unbalanced parenthesis?)" },
    { 0, PROP_LIMIT_ERROR,                         "Failed to setup limit by property"},
    { 0, SWISH_LISTRESULTS_EOF,                    "No more results" },
    { 0, HEADER_READ_ERROR,                        "Index Header Error", },
    { 1, INDEX_FILE_NOT_FOUND,                     "Could not open index file" },
    { 1, UNKNOWN_INDEX_FILE_FORMAT,                "Unknown index file format" },
    { 1, INDEX_FILE_IS_EMPTY,                      "Index file(s) is empty" },
    { 1, INDEX_FILE_ERROR,                         "Index file error" },
    { 1, INVALID_SWISH_HANDLE,                     "Invalid swish handle" },
    { 1, INVALID_RESULTS_HANDLE,                   "Invalid results object" },
};


/*****************************************************************
* SwishError
*
*   Pass:
*       SWISH *sw
*
*   Returns:
*       value of the last error number, or zero
*
******************************************************************/

int     SwishError(SWISH * sw)
{
    if (!sw)
        return INVALID_SWISH_HANDLE;
    return (sw->lasterror);
}



/*****************************************************************
* SwishErrorString
*
*   Pass:
*       SWISH *sw
*
*   Returns:
*       pointer to string of generic error message related to
*       the last error number
*
******************************************************************/


char   *SwishErrorString(SWISH *sw)
{
    return getErrorString(sw ? sw->lasterror : INVALID_SWISH_HANDLE);
}



/*****************************************************************
* SwishLastErrorMsg
*
*   Pass:
*       SWISH *sw
*
*   Returns:
*       pointer to the string comment of the last error message, if any
*
******************************************************************/


char   *SwishLastErrorMsg(SWISH *sw )
{
    return sw->lasterrorstr;
}





/*****************************************************************
* getErrorString
*
*   Pass:
*       error number
*
*   Returns:
*       value of the last error number, or zero
*
******************************************************************/


char *getErrorString(int number)
{
    int i;
    static char message[50];
    
    for (i = 0; i < (int)(sizeof(swishErrors) / sizeof(swishErrors[0])); i++)
        if ( number == swishErrors[i].error_num )
            return swishErrors[i].message_string;

    sprintf( message, "Invalid error number '%d'", number );
    return( message );
}

/*****************************************************************
* SwishCriticalError
*
*   This returns true if the last error was critical and means that
*   the swish object should be destroyed
*
*   Pass:
*       *sw
*
*   Returns:
*       true if the current sw->lasterror is a critical error
*       or if the number is invalid or the sw is null
*
******************************************************************/


int SwishCriticalError(SWISH *sw)
{
    int i;

    if ( !sw )
        return 1;
    
    for (i = 0; i < (int)(sizeof(swishErrors) / sizeof(swishErrors[0])); i++)
        if ( sw->lasterror == swishErrors[i].error_num )
            return swishErrors[i].critical;

    return 1;
}




/*****************************************************************
* SwishAbortLastError
*
*   Aborts with the error message type, and the optional comment message
*
*   Pass:
*       SWISH *sw
*
*   Returns:
*       nope
*
******************************************************************/

void SwishAbortLastError(SWISH *sw)
{
    if ( sw->lasterror < 0 )
    {
        if ( *(SwishLastErrorMsg( sw )) )
            progerr( "%s: %s", SwishErrorString( sw ), SwishLastErrorMsg( sw ) );
        else
            progerr( "%s", SwishErrorString( sw ) );
    }

    progerr("Swish aborted with non-negative lasterror");
}


